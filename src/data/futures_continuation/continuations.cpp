//
// Created by dewe on 6/18/23.
//
#include "continuations.h"
#include "adjustments/adjustments.h"
#include "epoch_core/macros.h"
#include "epoch_frame/factory/array_factory.h"
#include "epoch_frame/factory/index_factory.h"
#include "epoch_frame/frame_or_series.h"
#include "roll_method/first_of_month.h"
#include "roll_method/liquidity_based.h"
#include "vector"
#include <cstddef>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/series.h>
#include <tbb/parallel_for_each.h>

namespace epoch_script::data {
std::unique_ptr<futures::RolloverMethodBase>
MakeRolloverMethod(epoch_core::RolloverType rolloverType, int offset) {
  switch (rolloverType) {
  case epoch_core::RolloverType::FirstOfMonth:
    return std::make_unique<futures::FirstOfMonthRollMethod>(offset);
  case epoch_core::RolloverType::LastTradingDay:
    return std::make_unique<futures::LastTradingDayMethod>(offset);
  case epoch_core::RolloverType::LiquidityBased:
    return std::make_unique<futures::LiquidityBasedMethod>(offset);
  default:
    break;
  }
  ThrowExceptionFromStream("Invalid epoch_core::RolloverType");
}

std::unique_ptr<AdjustmentMethodBase>
MakeAdjustmentMethod(AdjustmentType adjustmentType) {
  switch (adjustmentType) {
  case epoch_core::AdjustmentType::BackwardPanamaCanal:
    return std::make_unique<BackwardPanamaMethod>();
  case epoch_core::AdjustmentType::BackwardRatio:
    return std::make_unique<BackwardRatioMethod>();
  case epoch_core::AdjustmentType::ForwardPanamaCanal:
    return std::make_unique<ForwardPanamaMethod>();
  case epoch_core::AdjustmentType::ForwardRatio:
    return std::make_unique<ForwardRatioMethod>();
  default:
    ThrowExceptionFromStream("Invalid epoch_core::AdjustmentType");
  }
}

  // For Intraday resample to 1D before applying continuation
epoch_frame::DataFrame
FuturesContinuation::BuildBars(const epoch_frame::DataFrame &df) const {
  using namespace epoch_frame;

  if (df.empty()) {
    return df;
  }

  const auto contractKey =
      epoch_script::EpochStratifyXConstants::instance().CONTRACT();
  const auto openKey =
      epoch_script::EpochStratifyXConstants::instance().OPEN();
  const auto highKey =
      epoch_script::EpochStratifyXConstants::instance().HIGH();
  const auto lowKey = epoch_script::EpochStratifyXConstants::instance().LOW();
  const auto closeKey =
      epoch_script::EpochStratifyXConstants::instance().CLOSE();
  const auto volumeKey =
      epoch_script::EpochStratifyXConstants::instance().VOLUME();
  const auto openInterestKey =
      epoch_script::EpochStratifyXConstants::instance().OPEN_INTEREST();

  std::vector<std::string> groupKey;
  groupKey.reserve(df.num_rows());

  // throws internally if wrong cast
  const auto timestampView = df.index()->array().to_timestamp_view();
  const auto contractView =
      df[contractKey].contiguous_array().to_view<std::string>();

  for (auto const &[timestamp, contract] :
       std::views::zip(*timestampView, *contractView)) {
    auto contractStr = *contract;
    groupKey.emplace_back(
        std::format("{}#{}", contractStr,
                    asset::ContractInfo::GetDecade(std::string(
                        contractStr.end() - 2, contractStr.end()))));
  }

  std::vector<std::pair<std::string, DataFrame>> groups;
  for (auto const &[key, sub_group] :
       df.group_by_apply(factory::array::make_array(groupKey)).groups()) {
    auto keyStr = key.repr();
    keyStr = keyStr.substr(0, keyStr.find('#'));
    groups.emplace_back(keyStr, df.iloc(Array{sub_group}));
  }

  if (groups.size() == 1) {
    return df;
  }

  std::ranges::sort(groups, [&](auto const &lhs, auto const &rhs) {
    return GetContractExpiration(lhs.first) < GetContractExpiration(rhs.first);
  });

  auto orderedTimestamps = df.index()->array().unique().to_timestamp_view();
  std::vector<int64_t> rolloverPoints;
  rolloverPoints.reserve(df.num_rows());

  FuturesConstructedBars frontBarData{};
  frontBarData.reserve(df.num_rows());

  FuturesConstructedBars backBarData;
  backBarData.reserve(df.num_rows());

  // Capture the bar data for the given contract at the current timestamp.
  // Note: we must **not** use the timestamp of the `contractData` row because
  // it might belong to a different trading day (e.g. when the back contract
  // has not started trading yet). Instead, we use the timeline's current
  // timestamp so that the `front` and `back` legs are aligned on the same
  // axis. This behaviour is asserted in the unit‑tests.
  auto emplace_data = [&](DataFrame const &contractData,
                          std::string const &frontContract, int64_t timelineTs,
                          FuturesConstructedBars &result) {
    result.o.emplace_back(contractData.iloc(0, openKey).as_double());
    result.h.emplace_back(contractData.iloc(0, highKey).as_double());
    result.l.emplace_back(contractData.iloc(0, lowKey).as_double());
    result.c.emplace_back(contractData.iloc(0, closeKey).as_double());
    result.v.emplace_back(contractData.iloc(0, volumeKey).as_double());
    result.oi.emplace_back(contractData.iloc(0, openInterestKey).as_double());
    result.t.emplace_back(timelineTs);
    result.s.emplace_back(frontContract);
  };

  // We'll track an index that increments over valid groups only
  size_t currentFrontContractIndex{0};
  size_t rowIndex{0};

  const auto isLastContract = [&] {
    return currentFrontContractIndex == (groups.size() - 1);
  };

  auto get_data = [&](const DataFrame &sub, const Scalar &ts) {
    if (sub.empty()) {
      SPDLOG_DEBUG("Empty DataFrame provided to get_data");
      return DataFrame{};
    }

    const auto idx = sub.index()->searchsorted(ts, SearchSortedSide::Left);
    if (idx >= sub.num_rows()) {
      // Debug information to understand the issue
      SPDLOG_DEBUG("Timestamp not found in contract data. Timestamp: {}, First "
                   "timestamp: {}, Last timestamp: {}, idx: {}, num_rows: {}",
                   ts.repr(), sub.index()->at(0).repr(),
                   sub.index()->at(-1).repr(), idx, sub.num_rows());

      // Instead of asserting, return empty DataFrame to indicate no data found
      return DataFrame{};
    }

    return sub.iloc({idx, idx + 1});
  };

  std::optional<epoch_frame::Date> lastDecisionDay;
  for (const auto &currentTimestamp : *orderedTimestamps) {
    const Scalar currentTimestampScalar(
        arrow::TimestampScalar{*currentTimestamp, orderedTimestamps->type()});
    const Date currentDate = currentTimestampScalar.to_datetime().date();

    const bool newDay = !lastDecisionDay || (currentDate != *lastDecisionDay);
    lastDecisionDay = currentDate;

    // 1. Roll over any contracts that have already expired *before* we touch
    //    their data. This can advance across multiple contracts if the input
    //    timeline has gaps (e.g. missing trading days).
    while (newDay && !isLastContract() &&
           currentTimestampScalar >
               groups[currentFrontContractIndex].second.index()->at(-1)) {
      if (rolloverPoints.empty() ||
          static_cast<size_t>(rolloverPoints.back()) != rowIndex)
        rolloverPoints.push_back(rowIndex);
      ++currentFrontContractIndex;
    }

    // After the forced‑roll loop we can safely access the (new) front group
    // because its last trading day is >= current timestamp.
    const auto &activeGroup = groups[currentFrontContractIndex].second;

    if (newDay && currentTimestampScalar < activeGroup.index()->at(0)) {
      // Timeline date lies before this contract starts trading – skip it.
      SPDLOG_DEBUG("Skipping current timestamp({}) because it is before the "
                   "current trading day({}) of the contract({}).",
                   currentTimestampScalar.repr(),
                   activeGroup.index()->at(0).repr(),
                   groups[currentFrontContractIndex].first);
      continue;
    }

    auto makeRolloverData = [&] {
      const auto frontData = get_data(activeGroup, currentTimestampScalar);
      RolloverMethodBase::Input in{.frontData = frontData,
                                   .backData = frontData,
                                   .currentDate = currentDate};
      if (!isLastContract()) {
        // Avoid forward‑bias: if the next contract has not started trading
        // yet (its earliest timestamp is *after* the timeline's current ts),
        // reuse the frontData as a placeholder. This prevents us from
        // injecting future prices into today's back leg.
        if (const auto &nextGroup =
                groups[currentFrontContractIndex + 1].second;
            currentTimestampScalar >= nextGroup.index()->at(0)) {
          in.backData = get_data(nextGroup, currentTimestampScalar);
        }
      }
      return in;
    };

    auto currentRollOverData = makeRolloverData();
    if (currentRollOverData.backData.empty() ||
        currentRollOverData.frontData.empty()) {
      SPDLOG_DEBUG("Skipping timestamp {} due to missing front or back data",
                   currentTimestampScalar.repr());
      continue;
    }

    // 2. Strategy‑based roll (e.g. volume, last‑trading‑day offset, etc.)
    if (newDay && !isLastContract() &&
        m_rolloverMethod->IsRollDate(currentRollOverData)) {
      if (rolloverPoints.empty() ||
          static_cast<size_t>(rolloverPoints.back()) != rowIndex)
        rolloverPoints.push_back(rowIndex);
      ++currentFrontContractIndex;
      currentRollOverData = makeRolloverData();
    }

    if (currentRollOverData.backData.empty() ||
        currentRollOverData.frontData.empty()) {
      SPDLOG_DEBUG(
          "Skipping timestamp {} after roll due to missing front or back data",
          currentTimestampScalar.repr());
      continue;
    }

    auto frontContract = groups[currentFrontContractIndex].first;
    auto backContractSymbol = isLastContract()
                                  ? frontContract
                                  : groups[currentFrontContractIndex + 1].first;

    const int64_t currentTsVal = currentTimestampScalar.timestamp().value;
    emplace_data(currentRollOverData.frontData, frontContract, currentTsVal,
                 frontBarData);
    emplace_data(currentRollOverData.backData, backContractSymbol, currentTsVal,
                 backBarData);
    ++rowIndex;
  }

  // If there's a legitimate chance that no roll is needed, turn this into a
  // warning or remove it. If your business logic insists that at least one roll
  // must happen, keep the assertion.
  if (rolloverPoints.empty()) {
    // Either change this to a warning or retain the assertion if it's
    // required.
    SPDLOG_WARN(
        "No rollover points found. Possibly a single-contract scenario.");
    return make_dataframe(
        factory::index::make_datetime_index(frontBarData.t, "", "UTC"),
        {factory::array::make_array(frontBarData.o),
         factory::array::make_array(frontBarData.h),
         factory::array::make_array(frontBarData.l),
         factory::array::make_array(frontBarData.c),
         factory::array::make_array(frontBarData.v),
         factory::array::make_array(frontBarData.oi),
         factory::array::make_array(frontBarData.s)},
        {openKey, highKey, lowKey, closeKey, volumeKey, openInterestKey,
         contractKey});
  }

  return m_adjustmentMethod->AdjustContracts(frontBarData, backBarData,
                                             rolloverPoints);
}

FuturesContinuationConstructor::FuturesContinuationConstructor(
    IFuturesContinuationPtr input)
    : m_barConstructors(std::move(input)) {}

AssetDataFrameMap FuturesContinuationConstructor::Build(
    AssetDataFrameMap const &inputData) const {
  AssetDataFrameMap result;
  std::mutex mtx;
  std::vector<std::optional<std::pair<asset::Asset, epoch_frame::DataFrame>>>
      items(inputData.size());

  tbb::parallel_for_each(
      inputData.begin(), inputData.end(), [&](auto const &item) {
        if (!item.first.IsFuturesContract()) {
          return;
        }

        auto continuation = item.first.MakeFuturesContinuation();
        auto df = m_barConstructors->BuildBars(item.second);
        if (df.empty()) {
          SPDLOG_WARN("No bars built for {}", continuation.ToString());
          return;
        }

        std::lock_guard lck{mtx};
        result[continuation] = df;
      });

  return result;
}

} // namespace epoch_script::data