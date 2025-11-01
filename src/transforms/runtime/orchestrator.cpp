//
// Created by adesola on 12/28/24.
//
#include "orchestrator.h"
#include "execution/intermediate_storage.h"
#include <boost/container_hash/hash.hpp>
#include <epoch_script/transforms/core/registration.h>
#include <format>
#include <spdlog/spdlog.h>

#include <epoch_script/transforms/core/transform_registry.h>
#include "epoch_protos/tearsheet.pb.h"

#include <tbb/parallel_for_each.h>
namespace {
  // Helper to check if transform metadata indicates it's a reporter
  bool IsReporterTransform(const epoch_script::transform::ITransformBase& transform) {
    const auto metadata = transform.GetConfiguration().GetTransformDefinition().GetMetadata();
    return metadata.category == epoch_core::TransformCategory::Reporter;
  }

}

namespace epoch_script::runtime {
DataFlowRuntimeOrchestrator::DataFlowRuntimeOrchestrator(
    std::vector<std::string> asset_ids,
    ITransformManagerPtr transformManager,
    IIntermediateStoragePtr cacheManager, ILoggerPtr logger)
    : m_asset_ids(std::move(asset_ids)) {

  if (cacheManager) {
    m_executionContext.cache = std::move(cacheManager);
  } else {
    m_executionContext.cache = std::make_unique<IntermediateResultStorage>();
  }

  if (logger) {
    m_executionContext.logger = std::move(logger);
  } else {
    m_executionContext.logger = std::make_unique<Logger>();
  }

  // Build transform instances from configurations (validates ordering)
  auto transforms = transformManager->BuildTransforms();
  SPDLOG_DEBUG("BuildTransforms returned {} transforms", transforms.size());

  // Track unique IDs to prevent actual duplicates
  std::unordered_set<std::string> usedIds;

  for (auto& transform : transforms) {
    SPDLOG_DEBUG("Processing transform {}", (transform ? "non-null" : "NULL"));
    const std::string uniqueId = transform->GetId();
    SPDLOG_DEBUG("Transform ID = '{}'", uniqueId);
    if (usedIds.contains(uniqueId)) {
      throw std::runtime_error(
          std::format("Duplicate transform id: {}", uniqueId));
    }
    usedIds.insert(uniqueId);

    SPDLOG_DEBUG("Registering Transform {} ({}), Unique ID: {}",
                 transform->GetName(), transform->GetId(), uniqueId);
    RegisterTransform(std::move(transform));
  }
}

std::vector<DataFlowRuntimeOrchestrator::TransformExecutionNode *>
DataFlowRuntimeOrchestrator::ResolveInputDependencies(
    const epoch_script::strategy::InputMapping &inputs) const {
  std::vector<TransformExecutionNode *> result;
  std::ranges::for_each(inputs | std::views::values,
                        [&](std::vector<std::string> const &inputList) {
                          for (const auto &s : inputList) {
                            auto it = m_outputHandleToNode.find(s);
                            if (it == m_outputHandleToNode.end()) {
                              throw std::runtime_error(std::format(
                                  "Handle {} was not previously hashed.", s));
                            }
                            result.emplace_back(it->second);
                          }
                        });
  return result;
}

void DataFlowRuntimeOrchestrator::RegisterTransform(
    std::unique_ptr<epoch_script::transform::ITransformBase> transform) {
  auto& transformRef = *transform;
  auto node = CreateTransformNode(transformRef);
  auto inputs = transformRef.GetInputIds();

  // Store the transform before creating node
  m_transforms.push_back(std::move(transform));

  if (inputs.empty()) {
    m_independentNodes.emplace_back(std::move(node));
    return;
  }

  // Resolve input dependencies - find the nodes that produce the required handles
  std::vector<TransformExecutionNode*> handles;
  for (const auto& inputHandle : inputs) {
    auto it = m_outputHandleToNode.find(inputHandle);
    if (it == m_outputHandleToNode.end()) {
      throw std::runtime_error(std::format(
          "Handle {} was not previously hashed.", inputHandle));
    }
    handles.push_back(it->second);
  }

  for (auto &handle : handles) {
    make_edge(*handle, *node);
  }
  m_dependentNodes.emplace_back(std::move(node));
}

TimeFrameAssetDataFrameMap
DataFlowRuntimeOrchestrator::ExecutePipeline(TimeFrameAssetDataFrameMap data) {
  // Initialize cache with input data
  m_executionContext.cache->InitializeBaseData(std::move(data),
                                         {m_asset_ids.begin(), m_asset_ids.end()});
  // Set up shared data
  m_executionContext.logger->clear();

  // Use TBB flow graph for parallel execution
  SPDLOG_INFO("Executing transform graph ({} transforms)", m_transforms.size());

  // Trigger independent nodes (nodes with no dependencies)
  for (const auto& node : m_independentNodes) {
    node->try_put(tbb::flow::continue_msg());
  }

  // Wait for all nodes to complete
  m_graph.wait_for_all();

  // Check for errors after execution
  const auto error = m_executionContext.logger->str();
  if (!error.empty()) {
    SPDLOG_ERROR("Transform pipeline failed with errors: {}", error);
    throw std::runtime_error(std::format("Transform pipeline failed: {}", error));
  }

  // Cache reports from reporter transforms
  for (const auto& transform : m_transforms) {
    CacheSelectorFromTransform(*transform);
    if (IsReporterTransform(*transform)) {
      CacheReportFromTransform(*transform);
    }
  }

  SPDLOG_INFO("Transform pipeline completed successfully");

  // Build final output from cache
  auto result = m_executionContext.cache->BuildFinalOutput();

#ifndef NDEBUG
  // Log final output sizes for alignment debugging
  SPDLOG_DEBUG("FLOW DEBUG - Transform pipeline completed with {} timeframes", result.size());
  for (const auto& [timeframe, assetMap] : result) {
    for (const auto& [asset_id, dataframe] : assetMap) {
      SPDLOG_INFO("FLOW DEBUG - Output data: {} {} has {} rows",
                   timeframe, asset_id, dataframe.num_rows());
    }
  }
#endif

  // Clean up shared data
  m_executionContext.logger->clear();

  return result;
}

std::function<void(execution_context_t)> DataFlowRuntimeOrchestrator::CreateExecutionFunction(
    const epoch_script::transform::ITransformBase &transform) {
  // Check if this transform is cross-sectional from its metadata
  bool isCrossSectional = transform.GetConfiguration().IsCrossSectional();

  if (isCrossSectional) {
    SPDLOG_DEBUG("Creating cross-sectional execution node for transform '{}'", transform.GetId());
    return MakeExecutionNode<true>(transform, m_executionContext);
  } else {
    return MakeExecutionNode<false>(transform, m_executionContext);
  }
}

DataFlowRuntimeOrchestrator::TransformNodePtr DataFlowRuntimeOrchestrator::CreateTransformNode(
    epoch_script::transform::ITransformBase& transform) {
  auto body = CreateExecutionFunction(transform);
  m_executionFunctions.push_back(body);

  const std::string transformId = transform.GetId();

  // Unlimited concurrency - TBB enforces dependencies through graph edges
  auto node = std::make_unique<TransformExecutionNode>(m_graph, tbb::flow::unlimited, body);
  SPDLOG_DEBUG("Created transform node '{}' (dependencies enforced by TBB graph)", transformId);

  // Register transform with cache (stores metadata for later queries)
  m_executionContext.cache->RegisterTransform(transform);

  auto outputs = transform.GetOutputMetaData();
  SPDLOG_DEBUG("Transform {} has {} output(s)", transformId, outputs.size());
  for (auto const &outputMetadata : outputs) {
    // safer to use transform interface to get output id due to overrides
    auto outputId = transform.GetOutputId(outputMetadata.id);
    SPDLOG_DEBUG("Registering output {} for transform {} (metadata.id={})",
                 outputId, transformId, outputMetadata.id);
    m_outputHandleToNode.insert_or_assign(outputId, node.get());
  }
  SPDLOG_DEBUG("Total handles registered so far: {}", m_outputHandleToNode.size());

  return node;
}


void DataFlowRuntimeOrchestrator::CacheReportFromTransform(
    const epoch_script::transform::ITransformBase& transform) const {
  const std::string transformId = transform.GetId();

  try {
    auto report = transform.GetTearSheet();

    // Validate report before caching
    if (report.ByteSizeLong() == 0) {
      SPDLOG_WARN("Transform {} produced empty report", transformId);
      return;
    }

    // For multi-asset scenarios, cache the report for each asset
    // Reporter transforms typically generate aggregate statistics that apply to all assets
    // Parallel report caching with mutex protection
    tbb::parallel_for_each(m_asset_ids.begin(), m_asset_ids.end(), [&](const auto& asset) {
      {
        std::lock_guard<std::mutex> lock(m_reportCacheMutex);

        // Check if we already have a report for this asset
        if (m_reportCache.contains(asset)) {
          SPDLOG_DEBUG("Merging report from transform {} with existing report for asset {}",
                       transformId, asset.GetSymbolStr());

          // Merge the new report with the existing one
          auto& existingReport = m_reportCache[asset];
          MergeReportInPlace(existingReport, report, transformId);

          SPDLOG_DEBUG("Successfully merged report from transform {} into existing report for asset {} (final size: {} bytes)",
                       transformId, asset.GetSymbolStr(), existingReport.ByteSizeLong());
        } else {
          SPDLOG_DEBUG("Cached first report from transform {} for asset {} ({} bytes)",
                 transformId, asset.GetSymbolStr(), report.ByteSizeLong());
          m_reportCache.emplace(asset, report);
        }
      }
      // Note: Selector caching is handled by CacheSelectorFromTransform() to avoid duplication
    });

  } catch (const std::exception& e) {
    SPDLOG_WARN("Failed to cache report from transform {}: {}", transformId, e.what());
  }
}

void DataFlowRuntimeOrchestrator::MergeReportInPlace(
    epoch_proto::TearSheet& existing,
    const epoch_proto::TearSheet& newReport,
    const std::string& sourceTransformId) {

  try {
    // Use protobuf's built-in MergeFrom method for efficient merging
    // This will:
    // - Merge repeated fields (cards, charts, tables) by appending
    // - Merge singular fields by overwriting with new values
    // - Handle all nested message merging automatically
    size_t originalSize [[maybe_unused]] = existing.ByteSizeLong();
    size_t newSize [[maybe_unused]] = newReport.ByteSizeLong();

    existing.MergeFrom(newReport);

    size_t mergedSize [[maybe_unused]] = existing.ByteSizeLong();
    SPDLOG_DEBUG("Report merge completed: {} + {} = {} bytes (from transform {})",
                 originalSize, newSize, mergedSize, sourceTransformId);

    // Log details about what was merged
    if (newReport.has_cards() && newReport.cards().cards_size() > 0) {
      SPDLOG_DEBUG("Merged {} cards from transform {}",
                   newReport.cards().cards_size(), sourceTransformId);
    }
    if (newReport.has_charts() && newReport.charts().charts_size() > 0) {
      SPDLOG_DEBUG("Merged {} charts from transform {}",
                   newReport.charts().charts_size(), sourceTransformId);
    }
    if (newReport.has_tables() && newReport.tables().tables_size() > 0) {
      SPDLOG_DEBUG("Merged {} tables from transform {}",
                   newReport.tables().tables_size(), sourceTransformId);
    }

  } catch (const std::exception& e) {
    SPDLOG_ERROR("Failed to merge report from transform {}: {}", sourceTransformId, e.what());
    throw;
  }
}

AssetReportMap DataFlowRuntimeOrchestrator::GetGeneratedReports() const {
  return m_reportCache;
}

void DataFlowRuntimeOrchestrator::CacheSelectorFromTransform(
    const epoch_script::transform::ITransformBase& transform) const {
  const std::string transformId = transform.GetId();

  try {
    auto selectorData = transform.GetSelectorData();

    // Validate selector data before caching
    if (selectorData.title.empty() || selectorData.schemas.empty()) {
      return;
    }

    // For multi-asset scenarios, cache the selector for each asset
    // Selector transforms typically generate UI metadata that applies to all assets
    // Parallel selector caching with mutex protection
    tbb::parallel_for_each(m_asset_ids.begin(), m_asset_ids.end(), [&](const auto& asset) {
      std::lock_guard<std::mutex> lock(m_selectorCacheMutex);

      // Check if we already have selectors for this asset
      if (m_selectorCache.contains(asset)) {
        // Append to existing vector of selectors
        m_selectorCache[asset].push_back(selectorData);
        SPDLOG_DEBUG("Appended selector from transform {} for asset {} (title: '{}', {} schemas, total selectors: {})",
               transformId, asset.GetSymbolStr(), selectorData.title,
               selectorData.schemas.size(), m_selectorCache[asset].size());
      } else {
        // Create new vector with this selector
        std::vector<epoch_script::transform::SelectorData> newVector;
        newVector.push_back(selectorData);
        m_selectorCache.emplace(asset, std::move(newVector));
        SPDLOG_DEBUG("Cached first selector from transform {} for asset {} (title: '{}', {} schemas)",
               transformId, asset.GetSymbolStr(), selectorData.title,
               selectorData.schemas.size());
      }
    });

  } catch (const std::exception& e) {
    SPDLOG_WARN("Failed to cache selector from transform {}: {}", transformId, e.what());
  }
}

AssetSelectorMap DataFlowRuntimeOrchestrator::GetGeneratedSelectors() const {
  return m_selectorCache;
}

} // namespace epoch_script::runtime