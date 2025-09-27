//
// Created by dewe on 9/21/24.
//

#pragma once
#include "epoch_core/macros.h"
#include "epoch_metadata/transforms/itransform.h"

CREATE_ENUM(EqualityOperator, GreaterThan, GreaterThanOrEquals, LessThan,
            LessThanOrEquals, Equals, NotEquals);

namespace epoch_metadata::transform {

template <epoch_core::EqualityOperator sign>
struct EqualityTransform : ITransform {
  explicit EqualityTransform(const TransformConfiguration &config)
      : ITransform(config) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series lhs = bars[GetInputId(epoch_metadata::ARG0)];
    epoch_frame::Series rhs = bars[GetInputId(epoch_metadata::ARG1)];

    if constexpr (sign == epoch_core::EqualityOperator::GreaterThanOrEquals) {
      lhs = lhs >= rhs;
    } else if constexpr (sign ==
                         epoch_core::EqualityOperator::LessThanOrEquals) {
      lhs = lhs <= rhs;
                         } else if constexpr (sign == epoch_core::EqualityOperator::Equals) {
                           lhs = lhs == rhs;
                         } else if constexpr (sign == epoch_core::EqualityOperator::NotEquals) {
                           lhs = lhs != rhs;
                         } else if constexpr (sign == epoch_core::EqualityOperator::GreaterThan) {
                           lhs = lhs > rhs;
                         } else if constexpr (sign == epoch_core::EqualityOperator::LessThan) {
                           lhs = lhs < rhs;
                         }
    return MakeResult(lhs);
  }
};

using VectorGT = EqualityTransform<epoch_core::EqualityOperator::GreaterThan>;
using VectorGTE =
    EqualityTransform<epoch_core::EqualityOperator::GreaterThanOrEquals>;
using VectorLT = EqualityTransform<epoch_core::EqualityOperator::LessThan>;
using VectorLTE =
    EqualityTransform<epoch_core::EqualityOperator::LessThanOrEquals>;
using VectorEQ = EqualityTransform<epoch_core::EqualityOperator::Equals>;
using VectorNEQ = EqualityTransform<epoch_core::EqualityOperator::NotEquals>;

enum class ValueCompareType { Highest, Lowest, Previous };

template <epoch_core::EqualityOperator sign, ValueCompareType type>
class ValueCompare : public ITransform {
public:
  explicit ValueCompare(const TransformConfiguration &config)
      : ITransform(config),
        m_lookback(config.GetOptionValue("periods").GetInteger()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    epoch_frame::Series current = bars[GetInputId()];
    epoch_frame::Series rhs;
    if constexpr (type == ValueCompareType::Highest) {
      rhs = current.rolling_agg({.window_size = m_lookback}).max();
    } else if constexpr (type == ValueCompareType::Lowest) {
      rhs = current.rolling_agg({.window_size = m_lookback}).min();
    } else if constexpr (type == ValueCompareType::Previous) {
      rhs = current.shift(m_lookback);
    }

    epoch_frame::Series result;
    if constexpr (sign == epoch_core::EqualityOperator::GreaterThan) {
      result = current > rhs;
    } else if constexpr (sign ==
                         epoch_core::EqualityOperator::GreaterThanOrEquals) {
      result = current >= rhs;
    } else if constexpr (sign == epoch_core::EqualityOperator::LessThan) {
      result = current < rhs;
    } else if constexpr (sign ==
                         epoch_core::EqualityOperator::LessThanOrEquals) {
      result = current <= rhs;
    } else if constexpr (sign == epoch_core::EqualityOperator::Equals) {
      result = current == rhs;
    } else if constexpr (sign == epoch_core::EqualityOperator::NotEquals) {
      result = current != rhs;
    }

    return MakeResult(result);
  }

private:
  int m_lookback;
};

using GreaterThanHighest =
    ValueCompare<epoch_core::EqualityOperator::GreaterThan,
                 ValueCompareType::Highest>;
using GreaterThanOrEqualsHighest =
    ValueCompare<epoch_core::EqualityOperator::GreaterThanOrEquals,
                 ValueCompareType::Highest>;
using EqualsHighest = ValueCompare<epoch_core::EqualityOperator::Equals,
                                   ValueCompareType::Highest>;
using NotEqualsHighest = ValueCompare<epoch_core::EqualityOperator::NotEquals,
                                      ValueCompareType::Highest>;
using LessThanHighest = ValueCompare<epoch_core::EqualityOperator::LessThan,
                                     ValueCompareType::Highest>;
using LessThanOrEqualsHighest =
    ValueCompare<epoch_core::EqualityOperator::LessThanOrEquals,
                 ValueCompareType::Highest>;

using GreaterThanLowest =
    ValueCompare<epoch_core::EqualityOperator::GreaterThan,
                 ValueCompareType::Lowest>;
using GreaterThanOrEqualsLowest =
    ValueCompare<epoch_core::EqualityOperator::GreaterThanOrEquals,
                 ValueCompareType::Lowest>;
using EqualsLowest = ValueCompare<epoch_core::EqualityOperator::Equals,
                                  ValueCompareType::Lowest>;
using NotEqualsLowest = ValueCompare<epoch_core::EqualityOperator::NotEquals,
                                     ValueCompareType::Lowest>;
using LessThanLowest = ValueCompare<epoch_core::EqualityOperator::LessThan,
                                    ValueCompareType::Lowest>;
using LessThanOrEqualsLowest =
    ValueCompare<epoch_core::EqualityOperator::LessThanOrEquals,
                 ValueCompareType::Lowest>;

using GreaterThanPrevious =
    ValueCompare<epoch_core::EqualityOperator::GreaterThan,
                 ValueCompareType::Previous>;
using GreaterThanOrEqualsPrevious =
    ValueCompare<epoch_core::EqualityOperator::GreaterThanOrEquals,
                 ValueCompareType::Previous>;
using EqualsPrevious = ValueCompare<epoch_core::EqualityOperator::Equals,
                                    ValueCompareType::Previous>;
using NotEqualsPrevious = ValueCompare<epoch_core::EqualityOperator::NotEquals,
                                       ValueCompareType::Previous>;
using LessThanPrevious = ValueCompare<epoch_core::EqualityOperator::LessThan,
                                      ValueCompareType::Previous>;
using LessThanOrEqualsPrevious =
    ValueCompare<epoch_core::EqualityOperator::LessThanOrEquals,
                 ValueCompareType::Previous>;

} // namespace epoch_metadata::transform