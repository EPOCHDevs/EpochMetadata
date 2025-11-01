//
// Created by dewe on 9/10/24.
//
#include <epoch_script/strategy/metadata.h>
#include "../core/doc_deserialization_helper.h"
#include <epoch_script/core/metadata_options.h>
#include <epoch_script/transforms/core/registry.h>
#include <epoch_script/transforms/core/metadata.h>
#include <epoch_script/transforms/core/registration.h>
#include <epoch_script/strategy/registration.h>
#include "transforms/compiler/ast_compiler.h"
#include <epoch_core/macros.h>
#include <glaze/core/reflect.hpp>
#include <glaze/json/json_concepts.hpp>
#include <glaze/json/generic.hpp>
#include <glaze/json/write.hpp>
#include <string>
#include <unordered_set>
#include <ranges>
#include <algorithm>

using namespace epoch_script;
using namespace epoch_script::strategy;
using epoch_core::BaseDataTimeFrame;
using epoch_core::EpochOffsetType;

// Helper function to determine base timeframe from compilation result
static std::optional<BaseDataTimeFrame>
GetBaseTimeFrameFromCompilationResult(const std::vector<AlgorithmNode> &compilationResult)
{
  std::unordered_set<EpochOffsetType> types;

  for (const auto &node : compilationResult)
  {
    // Check if node type requires intraday data or has a session
    if (epoch_script::transforms::kIntradayOnlyIds.contains(node.type) ||
        node.session)
    {
      types.emplace(EpochOffsetType::Minute);
      continue;
    }

    // Check node's timeframe
    if (!node.timeframe)
      continue;

    types.emplace(node.timeframe->GetOffset()->type());
  }

  if (types.empty())
  {
    return std::nullopt;
  }

  return std::ranges::any_of(types, [](EpochOffsetType t)
                             { return epoch_script::IsIntraday(t); })
             ? BaseDataTimeFrame::Minute
             : BaseDataTimeFrame::EOD;
}

// PythonSource constructor implementation
PythonSource::PythonSource(std::string src) : source_(std::move(src))
{
  if (source_.empty())
  {
    return;
  }

  // Compile Python source to get algorithm nodes
  epoch_script::AlgorithmAstCompiler compiler;
  compilationResult_ = compiler.compile(source_);
  m_executor_count = compiler.getExecutorCount();

  // Determine base timeframe from compilation result
  baseTimeframe_ = GetBaseTimeFrameFromCompilationResult(compilationResult_);

  // Set isIntraday flag based on baseTimeframe
  if (baseTimeframe_.has_value())
  {
    isIntraday_ = (baseTimeframe_.value() == epoch_core::BaseDataTimeFrame::Minute);
  }
}

namespace YAML
{
  bool convert<SessionVariant>::decode(YAML::Node const &node,
                                       SessionVariant &metadata)
  {
    if (node.IsScalar())
    {
      auto session = node.as<std::string>();
      metadata = epoch_core::SessionTypeWrapper::FromString(session);
    }
    else if (node["start"] && node["end"])
    {
      auto start = node["start"].as<std::string>();
      auto end = node["end"].as<std::string>();
      metadata = epoch_frame::SessionRange{epoch_script::TimeFromString(start),
                                           epoch_script::TimeFromString(end)};
    }
    else
    {
      throw std::runtime_error("Invalid session variant, must be a scalar or a "
                               "map with start and end keys, not " +
                               YAML::Dump(node));
    }
    return true;
  }

  bool convert<AlgorithmNode>::decode(YAML::Node const &node,
                                      AlgorithmNode &metadata)
  {

    metadata.type = node["type"].as<std::string>();
    metadata.id = node["id"].as<std::string>(metadata.type);

    auto expectedTransform =
        transforms::ITransformRegistry::GetInstance().GetMetaData(metadata.type);
    if (!expectedTransform)
    {
      throw std::runtime_error("Unknown transform type: " + metadata.type);
    }

    const auto &transform = expectedTransform->get();
    auto options = node["options"];
    if (!options && transform.options.size() > 0)
    {
      throw std::runtime_error(
          fmt::format("Missing options for transform {}", metadata.type));
    }

    for (auto const &option : transform.options)
    {
      auto arg = options[option.id];
      if (option.isRequired && !arg)
      {
        throw std::runtime_error("Missing required option: " + option.id +
                                 " for transform " + metadata.type);
      }

      auto serialized = YAML::Dump(arg);
      if (serialized.starts_with("."))
      {
        metadata.options.emplace(
            option.id,
            MetaDataOptionDefinition{MetaDataArgRef{serialized.substr(1)}});
      }
      else
      {
        metadata.options.emplace(option.id,
                                 CreateMetaDataArgDefinition(arg, option));
      }
      options.remove(option.id);
    }

    if (options && options.size() != 0)
    {
      throw std::runtime_error("Unknown options: " + Dump(options));
    }

    auto nodeInputs = node["inputs"];
    for (auto const &input : transform.inputs)
    {
      auto inputs = nodeInputs[input.id];
      if (!inputs)
      {
        continue;
      }

      if (input.allowMultipleConnections)
      {
        AssertFromFormat(inputs.IsSequence(), "Input {} is not a sequence",
                         input.id);
        metadata.inputs[input.id] = inputs.as<std::vector<std::string>>();
      }
      else
      {
        AssertFromFormat(inputs.IsScalar(), "Input {} is not a scalar", input.id);
        metadata.inputs[input.id] = std::vector{inputs.as<std::string>()};
      }
    }

    // If this transform requires a timeframe/session, set a default session
    // to signal session context to clients. Use SessionType by default so
    // clients can override with custom ranges in the UI if desired.
    if (auto sessionNode = node["session"])
    {
      auto session = sessionNode.as<SessionVariant>();
      AssertFromStream(transform.requiresTimeFrame,
                       "requiresTimeFrame is required for session");
      metadata.session = session;
    }

    return true;
  }

  bool convert<AlgorithmBaseMetaData>::decode(YAML::Node const &node,
                                              AlgorithmBaseMetaData &metadata)
  {
    metadata.id = node["id"].as<std::string>();
    metadata.name = node["name"].as<std::string>("");
    metadata.options =
        node["options"].as<MetaDataOptionList>(MetaDataOptionList{});
    metadata.desc = MakeDescLink(node["desc"].as<std::string>(""));
    metadata.tags =
        node["tags"].as<std::vector<std::string>>(std::vector<std::string>{});
    return true;
  }

  bool convert<AlgorithmMetaData>::decode(YAML::Node const &node,
                                          AlgorithmMetaData &metadata)
  {
    metadata.id = node["id"].as<std::string>();
    metadata.name = node["name"].as<std::string>("");
    metadata.options =
        node["options"].as<MetaDataOptionList>(MetaDataOptionList{});
    metadata.desc = MakeDescLink(node["desc"].as<std::string>(""));
    metadata.requiresTimeframe = node["requiresTimeframe"].as<bool>(true);
    metadata.tags =
        node["tags"].as<std::vector<std::string>>(std::vector<std::string>{});
    return true;
  }
} // namespace YAML