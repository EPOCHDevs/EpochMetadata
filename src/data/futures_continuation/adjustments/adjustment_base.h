#pragma once
//
// Created by dewe on 12/4/23.
//
#include "data/futures_continuation/constructed_bars.h"
#include "epoch_frame/aliases.h"
#include "epoch_script/core/bar_attribute.h"
#include "initializer_list"
#include "vector"
#include <epoch_core/enum_wrapper.h>

CREATE_ENUM(AdjustmentType, BackwardPanamaCanal, BackwardRatio,
            ForwardPanamaCanal, ForwardRatio);

namespace epoch_script::futures {
struct AdjustmentMethodBase {
  constexpr static std::initializer_list<epoch_script::BarAttribute::Type>
      g_adjustedAttributeType{epoch_script::BarAttribute::Type::Open,
                              epoch_script::BarAttribute::Type::High,
                              epoch_script::BarAttribute::Type::Low,
                              epoch_script::BarAttribute::Type::Close};
  constexpr static std::initializer_list<epoch_script::BarAttribute::Type>
      g_unAdjustedAttributeType{
          epoch_script::BarAttribute::Type::OpenInterest,
          epoch_script::BarAttribute::Type::Volume,
          epoch_script::BarAttribute::Type::Contract};

  virtual epoch_frame::DataFrame
  AdjustContracts(FuturesConstructedBars const &unAdjustedFrontBarData,
                  FuturesConstructedBars const &unAdjustedBackBarData,
                  std::vector<int64_t> const &rollIndexes) = 0;

  // Function to construct the adjusted table
  static arrow::TablePtr
  ConstructAdjustedTable(FuturesConstructedBars &bars,
                         const FuturesConstructedBars &unAdjustedFrontBarData);

  // Function to prepare the bars container
  static FuturesConstructedBars PrepareBarsContainer(int64_t nRows);

  virtual epoch_core::AdjustmentType GetType() const = 0;

  // Function to calculate roll index ranges
  static std::vector<std::pair<int64_t, int64_t>>
  CalculateRollIndexRanges(const std::vector<int64_t> &rollIndexes,
                           int64_t nRows);

  virtual ~AdjustmentMethodBase() = default;
};
} // namespace epoch_script::futures