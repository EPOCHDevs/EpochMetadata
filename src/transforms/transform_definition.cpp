//
// Created by adesola on 9/26/24.
//
#include <epochflow/core/time_frame.h>
#include <epochflow/transforms/core/registry.h>
#include <epochflow/transforms/core/transform_registry.h>
#include <unordered_map>
#include <utility>
#include <yaml-cpp/node/node.h>

using namespace epochflow::strategy;
using namespace epochflow;

namespace YAML {
template <> struct convert<std::optional<epochflow::TimeFrame>> {
  static bool decode(Node const &node,
                     std::optional<epochflow::TimeFrame> &timeframe) {
    if (!node.IsDefined()) {
      timeframe = std::nullopt;
      return true;
    }
    if (auto offset = node.as<epoch_frame::DateOffsetHandlerPtr>()) {
      timeframe = TimeFrame(std::move(offset));
      return true;
    }
    return false;
  }
};
} // namespace YAML

namespace epochflow {

TransformDefinition::TransformDefinition(YAML::Node const &argsNode)
    : TransformDefinition(
          argsNode.as<epochflow::strategy::AlgorithmNode>(),
          argsNode["timeframe"].as<std::optional<epochflow::TimeFrame>>(
              std::nullopt)) {}

epochflow::TimeFrame
GetTimeFrame(std::string const &id,
             std::optional<epochflow::TimeFrame> offset,
             std::optional<epochflow::TimeFrame> fallbackTimeframe) {
  if (offset) {
    return offset.value();
  }
  AssertFromStream(fallbackTimeframe, "Timeframe is required for " << id);
  return std::move(fallbackTimeframe.value());
}

TransformDefinition::TransformDefinition(
    AlgorithmNode const &algorithm, std::optional<TimeFrame> fallbackTimeframe)
    : m_data({
          .type = algorithm.type,
          .id = algorithm.id,
          .timeframe = GetTimeFrame(algorithm.id, algorithm.timeframe,
                                    std::move(fallbackTimeframe))
      }) {

  auto metaDataPtr =
      epochflow::transforms::ITransformRegistry::GetInstance().GetMetaData(
          m_data.type);
  AssertFromStream(metaDataPtr, "Invalid Transform: " << m_data.type);
  m_data.metaData = *metaDataPtr;

  for (auto const &arg : m_data.metaData.options) {
    if (!algorithm.options.contains(arg.id)) {
      AssertFalseFromStream(arg.isRequired,
                            "missing option: " << arg.id << " for "
                                               << m_data.type << " .");
    }
  }
  m_data.options = algorithm.options;

  m_data.inputs = algorithm.inputs;
  int connectedInput = 0;
  for (auto const &input : m_data.metaData.inputs) {
    if (!algorithm.inputs.contains(input.id)) {
      AssertFromStream(m_data.metaData.atLeastOneInputRequired,
                       m_data.metaData.id << " is missing input(" << input.id
                                          << ").");
    } else {
      ++connectedInput;
    }
  }

  if (connectedInput == 0 && m_data.metaData.atLeastOneInputRequired &&
      !algorithm.inputs.empty()) {
    ThrowExceptionFromStream("Found no inputs for "
                             << m_data.metaData.id
                             << ", but at least 1 input was required.");
  }

  if (algorithm.session) {
    if (std::holds_alternative<epoch_frame::SessionRange>(*algorithm.session)) {
      auto sessionRange =
          std::get<epoch_frame::SessionRange>(algorithm.session.value());
      AssertFromStream(sessionRange.start <= sessionRange.end,
                       "Invalid session range: " << sessionRange.start << " > "
                                                  << sessionRange.end);
    }
    else {
      m_data.sessionRange =
        kSessionRegistry.at(std::get<epoch_core::SessionType>(algorithm.session.value()));
    }
  }

}
} // namespace epochflow