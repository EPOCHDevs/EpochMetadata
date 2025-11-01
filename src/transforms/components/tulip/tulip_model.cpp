//
// Created by dewe on 1/10/23.
//
#include "tulip_model.h"
#include "boost/algorithm/algorithm.hpp"
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <epochflow/core/bar_attribute.h>
#include <epochflow/transforms/core/metadata.h>

namespace epochflow::transform {
template <bool IsIndicator>
TulipModelImpl<IsIndicator>::TulipModelImpl(
    const TransformConfiguration &config)
    : ITransform(config) {
  auto C = EpochStratifyXConstants::instance();
  if constexpr (IsIndicator) {
    // Handle crossunder as crossover with swapped inputs
    std::string transformName = config.GetTransformName();
    if (transformName == "crossunder") {
      transformName = "crossover";
    }
    m_info = ti_find_indicator(transformName.c_str());
    AssertFromFormat(m_info != nullptr, "TIError: indicator fn `{}` not found",
                     transformName);
    static std::unordered_map<std::string, std::string> keyMapping = {
        {"close", C.CLOSE()},
        {"high", C.HIGH()},
        {"low", C.LOW()},
        {"open", C.OPEN()},
        {"volume", C.VOLUME()}};

    for (auto const &strView :
         std::span{m_info->input_names, static_cast<size_t>(m_info->inputs)}) {
      std::string str{strView};
      if (keyMapping.contains(str)) {
        m_required_bar_inputs.push_back(keyMapping.at(str));
      }
    }
  } else {
    m_info = tc_find_candle(config.GetTransformName().c_str());
    AssertFromFormat(m_info != nullptr, "TIError: candle fn `{}` not found",
                     config.GetTransformName());
    m_required_bar_inputs =
        std::vector<std::string>{C.OPEN(), C.HIGH(), C.LOW(), C.CLOSE()};
  }
}

template <bool IsIndicator>
std::pair<std::vector<std::string>, std::vector<std::vector<double>>>
TulipModelImpl<IsIndicator>::MakeEmptyOutputVector(int64_t length) const {
  const auto outputMetaData = GetOutputMetaData();
  std::vector<std::string> keys(outputMetaData.size());
  std::vector<std::vector<double>> output(outputMetaData.size());

  std::transform(outputMetaData.begin(), outputMetaData.end(), keys.begin(),
                 output.begin(), [&](const auto &metaData, std::string &key) {
                   key = GetOutputId(metaData.id);
                   return std::vector<double>(length, IsIndicator ? NAN : 0);
                 });
  return std::pair{keys, output};
}

template <bool IsIndicator>
epoch_frame::DataFrame TulipModelImpl<IsIndicator>::MakeDataFrame(
    const std::pair<std::vector<std::string>, std::vector<std::vector<double>>>
        &outputPair,
    epoch_frame::IndexPtr const &index) const {
  std::vector<std::string> columns;
  columns.reserve(outputPair.first.size());

  arrow::ChunkedArrayVector arrayList;
  arrayList.reserve(outputPair.first.size());

  for (auto const &[k, v, outputMetadata] : std::views::zip(
           outputPair.first, outputPair.second, m_config.GetOutputs())) {
    columns.emplace_back(k);
    auto arr = epoch_frame::factory::array::make_array(v);
    if (outputMetadata.type == epoch_core::IODataType::Boolean) {
      arr = epoch_frame::AssertArrayResultIsOk(
          arrow::compute::Cast(arr, arrow::boolean()));
    }
    arrayList.emplace_back(arr);
  }

  return epoch_frame::make_dataframe(index, arrayList, columns);
}

template <bool IsIndicator>
epoch_frame::DataFrame TulipModelImpl<IsIndicator>::TransformData(
    const epoch_frame::DataFrame &bars) const {
  auto inputKeys = m_required_bar_inputs;
  for (auto const &key : GetInputIds()) {
    inputKeys.push_back(key);
  }

  // Swap inputs for crossunder to implement it as crossover with reversed inputs
  if (GetName() == "crossunder" && inputKeys.size() >= 2) {
    std::swap(inputKeys[inputKeys.size() - 2], inputKeys[inputKeys.size() - 1]);
  }

  const int64_t length = bars.num_rows();
  const epoch_frame::IndexPtr index = bars.index();

  std::vector<const double *> inputs(inputKeys.size());
  std::vector<epoch_frame::Array> casted(inputKeys.size());

  std::transform(
      inputKeys.begin(), inputKeys.end(), casted.begin(), inputs.begin(),
      [&bars](const std::string &key, epoch_frame::Array &currentArray) {
        currentArray = bars[key].contiguous_array();
        if (currentArray.type()->id() != arrow::Type::DOUBLE) {
          currentArray = currentArray.cast(arrow::float64());
        }

        const auto rawArray = currentArray.to_view<double>();
        return rawArray->raw_values();
      });

  std::vector<double> options [[maybe_unused]];
  int64_t start = 0;
  if constexpr (IsIndicator) {
    for (auto const &optionMetaData : GetOptionsMetaData()) {
      options.push_back(GetOption(optionMetaData.id).GetNumericValue());
    }
    start = m_info->start(options.data());
    if (start < 0) {
      auto outputPair = MakeEmptyOutputVector(length);
      return MakeDataFrame(outputPair, index);
    }
  }

  const auto outputLength = std::max(0, static_cast<int>(length - start));
  auto outputPair = MakeEmptyOutputVector(outputLength);
  if (outputLength == 0) {
    return MakeDataFrame(outputPair, index->iloc({0, 0}));
  }

  int returnCode{};
  if constexpr (!IsIndicator) {
    tc_config cfg{.period = static_cast<int>(GetOption("period").GetInteger()),
                  .body_none = GetOption("body_none").GetNumericValue(),
                  .body_short = GetOption("body_short").GetNumericValue(),
                  .body_long = GetOption("body_long").GetNumericValue(),
                  .wick_none = GetOption("wick_none").GetNumericValue(),
                  .wick_long = GetOption("wick_long").GetNumericValue(),
                  .near = GetOption("near").GetNumericValue()};

    auto outcnd = tc_result_new();
    returnCode = tc_run(m_info->pattern, length, inputs.data(), &cfg, outcnd);
    const auto size = tc_result_count(outcnd);

    for (int i = 0; i < size; ++i) {
      const tc_hit &h = tc_result_get(outcnd, i);
      outputPair.second[0][h.index] = 1;
    }
    tc_result_free(outcnd);
  } else {
    std::vector<double *> outputsPtr(outputPair.second.size(), nullptr);

    std::ranges::transform(
        outputPair.second, outputsPtr.begin(),
        [&](std::vector<double> &v) -> double * { return v.data(); });

    returnCode = m_info->indicator(length, inputs.data(), options.data(),
                                   outputsPtr.data());
  }

  if (returnCode == TI_OKAY) {
    return MakeDataFrame(outputPair, index->iloc({start}));
  }
  std::stringstream ss;
  ss << "Error computing technical Indicator:\n";
  ss << *this;
  throw std::runtime_error(ss.str());
}

template class TulipModelImpl<true>;
template class TulipModelImpl<false>;

} // namespace epochflow::transform