//
// Created by dewe on 9/10/24.
//
#include "epoch_metadata/strategy/metadata.h"
#include "doc_deserialization_helper.h"
#include "epoch_metadata/metadata_options.h"
#include "epoch_metadata/strategy/algorithm_validator.h"
#include "epoch_metadata/strategy/ui_graph.h"
#include "epoch_metadata/strategy/validation.h"
#include "epoch_metadata/transforms/registry.h"
#include <epoch_core/macros.h>
#include <glaze/core/reflect.hpp>
#include <glaze/json/json_concepts.hpp>
#include <glaze/json/json_t.hpp>
#include <glaze/json/write.hpp>
#include <string>

using namespace epoch_metadata;
using namespace epoch_metadata::strategy;

namespace YAML {
bool convert<SessionVariant>::decode(YAML::Node const &node,
                                     SessionVariant &metadata) {
  if (node.IsScalar()) {
    auto session = node.as<std::string>();
    metadata = epoch_core::SessionTypeWrapper::FromString(session);
  } else if (node["start"] && node["end"]) {
    auto start = node["start"].as<std::string>();
    auto end = node["end"].as<std::string>();
    metadata = epoch_frame::SessionRange{epoch_metadata::TimeFromString(start),
                                         epoch_metadata::TimeFromString(end)};
  } else {
    throw std::runtime_error("Invalid session variant, must be a scalar or a "
                             "map with start and end keys, not " +
                             YAML::Dump(node));
  }
  return true;
}

bool convert<AlgorithmNode>::decode(YAML::Node const &node,
                                    AlgorithmNode &metadata) {

  metadata.type = node["type"].as<std::string>();
  metadata.id = node["id"].as<std::string>(metadata.type);

  auto expectedTransform =
      transforms::ITransformRegistry::GetInstance().GetMetaData(metadata.type);
  if (!expectedTransform) {
    throw std::runtime_error("Unknown transform type: " + metadata.type);
  }

  const auto &transform = expectedTransform->get();
  auto options = node["options"];
  if (!options && transform.options.size() > 0) {
    throw std::runtime_error(
        fmt::format("Missing options for transform {}", metadata.type));
  }

  for (auto const &option : transform.options) {
    auto arg = options[option.id];
    if (option.isRequired && !arg) {
      throw std::runtime_error("Missing required option: " + option.id +
                               " for transform " + metadata.type);
    }

    auto serialized = YAML::Dump(arg);
    if (serialized.starts_with(".")) {
      metadata.options.emplace(
          option.id,
          MetaDataOptionDefinition{MetaDataArgRef{serialized.substr(1)}});
    } else {
      metadata.options.emplace(option.id,
                               CreateMetaDataArgDefinition(arg, option));
    }
    options.remove(option.id);
  }

  if (options && options.size() != 0) {
    throw std::runtime_error("Unknown options: " + Dump(options));
  }

  auto nodeInputs = node["inputs"];
  for (auto const &input : transform.inputs) {
    auto inputs = nodeInputs[input.id];
    if (!inputs) {
      continue;
    }

    if (input.allowMultipleConnections) {
      AssertFromFormat(inputs.IsSequence(), "Input {} is not a sequence",
                       input.id);
      metadata.inputs[input.id] = inputs.as<std::vector<std::string>>();
    } else {
      AssertFromFormat(inputs.IsScalar(), "Input {} is not a scalar", input.id);
      metadata.inputs[input.id] = std::vector{inputs.as<std::string>()};
    }
  }

  // If this transform requires a timeframe/session, set a default session
  // to signal session context to clients. Use SessionType by default so
  // clients can override with custom ranges in the UI if desired.
  if (auto sessionNode = node["session"]) {
    auto session = sessionNode.as<SessionVariant>();
    AssertFromStream(transform.requiresTimeFrame,
                     "requiresTimeFrame is required for session");
    metadata.session = session;
  }

  return true;
}

bool convert<AlgorithmBaseMetaData>::decode(YAML::Node const &node,
                                            AlgorithmBaseMetaData &metadata) {
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
                                        AlgorithmMetaData &metadata) {
  metadata.id = node["id"].as<std::string>();
  metadata.name = node["name"].as<std::string>("");
  metadata.options =
      node["options"].as<MetaDataOptionList>(MetaDataOptionList{});
  metadata.desc = MakeDescLink(node["desc"].as<std::string>(""));
  metadata.isGroup = node["isGroup"].as<bool>(false);
  metadata.requiresTimeframe = node["requiresTimeframe"].as<bool>(true);
  metadata.tags =
      node["tags"].as<std::vector<std::string>>(std::vector<std::string>{});
  return true;
}
} // namespace YAML