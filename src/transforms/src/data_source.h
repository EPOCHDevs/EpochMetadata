//
// Created by adesola on 6/1/25.
//

#pragma once
#include "epoch_metadata/bar_attribute.h"
#include "epoch_metadata/transforms/itransform.h"
#include <unordered_set>

namespace epoch_metadata::transform {
class DataSourceTransform final : public ITransform {
public:
  explicit DataSourceTransform(const TransformConfiguration &config)
      : ITransform(config) {
    for (auto const &outputMetaData : config.GetOutputs()) {
      m_replacements[outputMetaData.id] = config.GetOutputId(outputMetaData.id);
    }
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &bars) const override {
    return bars.rename(m_replacements);
  }

private:
  std::unordered_map<std::string, std::string> m_replacements;
  inline static const std::unordered_set<std::string> m_allowedInputIds{
      epoch_metadata::BarsConstants::instance().all.begin(),
      epoch_metadata::BarsConstants::instance().all.end()};
};

} // namespace epoch_metadata::transform