//
// Created by adesola on 12/28/24.
//
#include "execution_node.h"
#include "epoch_core/macros.h"
#include "epoch_frame/aliases.h"
#include <tbb/parallel_for_each.h>
#include <tbb/concurrent_vector.h>
#include <arrow/type_fwd.h>
#include <epoch_frame/common.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/index.h>
#include <epoch_script/core/time_frame.h>
#include <epoch_script/transforms/core/sessions_utils.h>
#include <unordered_map>
#include <vector>

// TODO: Watch out for throwing excePtion in these functions -> causes deadlock
namespace epoch_script::runtime {
// Best-effort intraday detection from timeframe string (e.g., 1Min, 5Min, 1H)
static inline bool IsIntradayString(std::string const &tf) {
  if (tf.size() < 2)
    return false;
  if (tf.ends_with("Min"))
    return true;
  if (!tf.empty() && tf.back() == 'H')
    return true;
  return false;
}

// Delegate to shared utils (UTC-aware)
static inline epoch_frame::DataFrame
SliceBySession(epoch_frame::DataFrame const &df,
               epoch_frame::SessionRange const &range) {
  return epoch_script::transform::sessions_utils::SliceBySessionUTC(df,
                                                                      range);
}
void ApplyDefaultTransform(
    const epoch_script::transform::ITransformBase &transformer,
    ExecutionContext &msg) {
  auto timeframe = transformer.GetTimeframe().ToString();
  auto name = transformer.GetName() + " " + transformer.GetId();

  // Enforce intradayOnly if metadata requests it
  try {
    const auto meta =
        transformer.GetConfiguration().GetTransformDefinition().GetMetadata();
    if (meta.intradayOnly && !IsIntradayString(timeframe)) {
      SPDLOG_WARN("Transform {} marked intradayOnly but timeframe {} is not "
                  "intraday. Skipping.",
                  name, timeframe);
      for (auto const &asset_id : msg.cache->GetAssetIDs()) {
        try {
          msg.cache->StoreTransformOutput(asset_id, transformer,
                                          epoch_frame::DataFrame{});
        } catch (std::exception const &exp) {
          msg.logger->log(std::format(
              "Asset: {}, Transform: {}, Error: {}.", asset_id,
              transformer.GetConfiguration().GetId(), exp.what()));
        }
      }
      return;
    }
  } catch (...) {
    // If metadata structure doesn't contain the flag yet, ignore
  }

  // Lambda for processing a single asset
  auto processAsset = [&](auto const &asset_id) {
    try {
      auto result = msg.cache->GatherInputs(asset_id, transformer);
      result = transformer.GetConfiguration()
                       .GetTransformDefinition()
                       .GetMetadata()
                       .allowNullInputs
                   ? result
                   : result.drop_null();

      // Apply session slicing if required by metadata and session is resolvable
      bool requiresSession = false;
      std::optional<epoch_frame::SessionRange> sessionRange =
          transformer.GetConfiguration().GetSessionRange();
      // Heuristic: if an explicit session range isn't set, consider option
      // presence
      if (!sessionRange) {
        try {
          requiresSession =
              transformer.GetConfiguration().GetOptions().contains("session");
        } catch (...) {
          requiresSession = false;
        }
      } else {
        requiresSession = true;
      }
      if (requiresSession) {
        if (sessionRange) {
          result = SliceBySession(result, *sessionRange);
        } else {
          SPDLOG_WARN(
              "Transform {} requiresSession but no session range was resolved.",
              name);
        }
      }

      if (!result.empty()) {
        result = transformer.TransformData(result);
      } else {
        SPDLOG_WARN(
            "Asset({}): Empty DataFrame provided to {}. Skipping transform",
            asset_id, name);
        result = epoch_frame::DataFrame{}; // Empty result, cache manager will
        // handle
      }

      msg.cache->StoreTransformOutput(asset_id, transformer, result);
    } catch (std::exception const &exp) {
      const auto error =
          std::format("Asset: {}, Transform: {}, Error: {}.", asset_id,
                      transformer.GetConfiguration().GetId(), exp.what());
      msg.logger->log(error);
    }
  };

  // Parallel per-asset processing using TBB
  const auto& asset_ids = msg.cache->GetAssetIDs();
  tbb::parallel_for_each(asset_ids.begin(), asset_ids.end(), processAsset);
}

void ApplyCrossSectionTransform(
    const epoch_script::transform::ITransformBase &transformer,
    ExecutionContext &msg) {
  // Build input list across all symbols in timeframe
  auto timeframe = transformer.GetTimeframe().ToString();
  auto inputId = transformer.GetInputId();
  auto outputId = transformer.GetOutputId();
  const auto &asset_ids = msg.cache->GetAssetIDs();

  // Enforce intradayOnly if metadata requests it
  try {
    const auto meta =
        transformer.GetConfiguration().GetTransformDefinition().GetMetadata();
    if (meta.intradayOnly && !IsIntradayString(timeframe)) {
      SPDLOG_WARN("Cross-sectional transform {} marked intradayOnly but "
                  "timeframe {} is not intraday. Skipping.",
                  transformer.GetConfiguration().GetId(), timeframe);
      for (auto const &asset_id : asset_ids) {
        try {
          msg.cache->StoreTransformOutput(asset_id, transformer,
                                          epoch_frame::DataFrame{});
        } catch (std::exception const &exp) {
          msg.logger->log(std::format(
              "Asset: {}, Transform: {}, Error: {}.", asset_id,
              transformer.GetConfiguration().GetId(), exp.what()));
        }
      }
      return;
    }
  } catch (...) {
  }

  std::vector<epoch_frame::FrameOrSeries> inputPerAsset;
  inputPerAsset.reserve(asset_ids.size());

  // Single transform call on vector of DataFrames
  try {
    // Parallel input gathering with thread-safe vector
    tbb::concurrent_vector<epoch_frame::FrameOrSeries> concurrentInputs;

    tbb::parallel_for_each(asset_ids.begin(), asset_ids.end(), [&](auto const &asset_id) {
      auto assetDataFrame =
          msg.cache->GatherInputs(asset_id, transformer).drop_null();
      // Apply session slicing if required
      bool requiresSession = false;
      std::optional<epoch_frame::SessionRange> sessionRange =
          transformer.GetConfiguration().GetSessionRange();
      if (!sessionRange) {
        try {
          requiresSession =
              transformer.GetConfiguration().GetOptions().contains("session");
        } catch (...) {
          requiresSession = false;
        }
      } else {
        requiresSession = true;
      }
      if (requiresSession) {
        if (sessionRange) {
          assetDataFrame = SliceBySession(assetDataFrame, *sessionRange);
        } else {
          SPDLOG_WARN("Cross-sectional transform {} requiresSession but no "
                      "session range was resolved.",
                      transformer.GetConfiguration().GetId());
        }
      }
      auto inputSeries = assetDataFrame[inputId].rename(asset_id);
      concurrentInputs.push_back(inputSeries);
    });

    // Copy to regular vector for concat
    inputPerAsset.assign(concurrentInputs.begin(), concurrentInputs.end());

    auto inputDataFrame =
        epoch_frame::concat(
            epoch_frame::ConcatOptions{.frames = inputPerAsset,
                                       .joinType = epoch_frame::JoinType::Outer,
                                       .axis = epoch_frame::AxisType::Column})
            .drop_null();

    epoch_frame::DataFrame crossResult =
        inputDataFrame.empty() ? epoch_frame::DataFrame{}
                               // Empty result, cache manager will handle
                               : transformer.TransformData(inputDataFrame);

    SPDLOG_DEBUG("CROSS-SECTIONAL DEBUG - Transform: {}, Output ID: {}",
                 transformer.GetId(), outputId);
    SPDLOG_DEBUG("CROSS-SECTIONAL DEBUG - crossResult num_cols: {}, num_rows: {}",
                 crossResult.num_cols(), crossResult.num_rows());
    SPDLOG_DEBUG("CROSS-SECTIONAL DEBUG - crossResult contains(outputId={}): {}",
                 outputId, crossResult.contains(outputId));
    if (!crossResult.empty()) {
      std::string colNames;
      for (const auto& col : crossResult.column_names()) {
        if (!colNames.empty()) colNames += ", ";
        colNames += col;
      }
      SPDLOG_DEBUG("CROSS-SECTIONAL DEBUG - crossResult column names: {}", colNames);
    }

    if (crossResult.num_cols() == 1 && crossResult.contains(outputId)) {
      // broadcast single column cross-sectional result across all assets
      SPDLOG_DEBUG("CROSS-SECTIONAL DEBUG - Broadcasting single column {} to all {} assets",
                   outputId, asset_ids.size());
      for (auto &asset_id : asset_ids) {
        msg.cache->StoreTransformOutput(asset_id, transformer, crossResult);
      }
    } else {
      SPDLOG_DEBUG("CROSS-SECTIONAL DEBUG - Distributing multi-column result by asset ID");
      for (auto const &asset_id : asset_ids) {
        epoch_frame::DataFrame assetResult;
        if (crossResult.contains(asset_id)) {
          SPDLOG_DEBUG("CROSS-SECTIONAL DEBUG - Asset {} found in crossResult, extracting column", asset_id);
          assetResult = crossResult[asset_id].to_frame(outputId);
        } else {
          SPDLOG_DEBUG("CROSS-SECTIONAL DEBUG - Asset {} NOT found in crossResult (empty result)", asset_id);
        }
        msg.cache->StoreTransformOutput(asset_id, transformer, assetResult);
      }
    }

  } catch (std::exception const &exp) {
    auto error =
        std::format("Transform : {}", transformer.GetConfiguration().GetId());
    const auto exception = std::format("{}\n{}", exp.what(), error);
    msg.logger->log(exception);
  }
}
} // namespace epoch_script::runtime