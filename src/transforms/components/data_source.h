//
// Created by adesola on 6/1/25.
//

#pragma once
#include <epochflow/core/bar_attribute.h>
#include <epochflow/transforms/core/itransform.h>
#include <unordered_set>

namespace epochflow::transform {
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
      epochflow::BarsConstants::instance().all.begin(),
      epochflow::BarsConstants::instance().all.end()};
};

} // namespace epochflow::transform