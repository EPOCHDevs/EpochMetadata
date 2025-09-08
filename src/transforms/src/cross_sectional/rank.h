#pragma once
//
// Created by dewe on 4/14/23.
//
#include "epoch_frame/array.h"
#include "epoch_metadata/transforms/itransform.h"
#include <cstdint>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_metadata/constants.h>

namespace epoch_metadata::transform {
/**
 * @brief Cross-sectional returns operation
 *
 * This transform calculates cross-sectional returns across multiple assets.
 * It computes the mean percentage change across all assets (columns) at each
 * time point, then calculates the cumulative product of these mean returns
 * plus 1.
 *
 * The operation is performed per time step across all assets, and the result
 * is broadcasted back to each asset in the output.
 *
 * Input: DataFrame containing percentage changes for multiple assets
 * Output: DataFrame containing cumulative cross-sectional returns
 */

template <bool ascending = true, bool is_percentile = false>
struct CrossSectionalRankOperation final : ITransform {

  static constexpr auto cmp =
      std::conditional_t<ascending, std::less<double>, std::greater<double>>{};

  explicit CrossSectionalRankOperation(const TransformConfiguration &config)
      : ITransform(config),
        k(static_cast<size_t>(config.GetOptionValue("k").GetInteger())) {
    if constexpr (is_percentile) {
      AssertFromFormat(k > 0 && k <= 100,
                       "k must be between 0 and 100(inclusive)");
    } else {
      AssertFromFormat(k > 0, "k must be greater than 0");
    }
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(const epoch_frame::DataFrame &scores) const override {
    using namespace epoch_frame;
    auto k_ = GetK(scores.num_cols());
    std::vector<std::size_t> idx(scores.num_cols());
    std::iota(idx.begin(), idx.end(), 0);

    return scores.apply(
        [&](Array const &array) {
          auto ranks = idx;
          auto data = array.to_view<double>();
          auto comp = [&](std::size_t a, std::size_t b) {
            auto lhs = data->Value(int64_t(a));
            auto rhs = data->Value(int64_t(b));
            return cmp(lhs, rhs);
          };

          std::nth_element(ranks.begin(), ranks.begin() + k_ - 1, ranks.end(),
                           comp);

          std::vector<bool> mask(data->length(), false);
          for (size_t i = 0; i < k_; ++i) {
            mask[ranks[i]] = true;
          }
          return Array::FromVector(mask);
        },
        AxisType::Row);
  }

private:
  size_t k;

  constexpr auto GetK(size_t n) const {
    size_t k_{k};
    if constexpr (is_percentile) {
      k_ = static_cast<size_t>(std::ceil((k / 100.0) * n));
    }
    return std::clamp(k_, 1UL, n);
  }
};

using CrossSectionalTopKOperation = CrossSectionalRankOperation<false, false>;
using CrossSectionalBottomKOperation = CrossSectionalRankOperation<true, false>;
using CrossSectionalTopKPercentileOperation =
    CrossSectionalRankOperation<false, true>;
using CrossSectionalBottomKPercentileOperation =
    CrossSectionalRankOperation<true, true>;

} // namespace epoch_metadata::transform
