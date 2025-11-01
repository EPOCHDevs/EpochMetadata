#pragma once
//
// Created by dewe on 10/1/22.
//
#include <candles.h>

#include "arrow/array.h"
#include <epochflow/transforms/core/itransform.h>
#include "indicators.h"
#include "stdexcept"
#include "string"
#include "unordered_map"
#include "vector"
#include <cmath>
#include <epoch_core/macros.h>
#include <mutex>

namespace epochflow::transform {

template <bool IsIndicator> class TulipModelImpl final : public ITransform {
public:
  using InfoType =
      std::conditional_t<IsIndicator, ti_indicator_info, tc_candle_info>;
  explicit TulipModelImpl(const TransformConfiguration &config);
  ~TulipModelImpl() override = default;

  std::pair<std::vector<std::string>, std::vector<std::vector<double>>>
  MakeEmptyOutputVector(int64_t length) const;

  epoch_frame::DataFrame
  TransformData(const epoch_frame::DataFrame &bars) const override;

protected:
  std::vector<std::string> m_required_bar_inputs;
  const InfoType *m_info;

  epoch_frame::DataFrame
  MakeDataFrame(const std::pair<std::vector<std::string>,
                                std::vector<std::vector<double>>> &data,
                epoch_frame::IndexPtr const &index) const;
};

extern template class TulipModelImpl<true>;
extern template class TulipModelImpl<false>;

} // namespace epochflow::transform