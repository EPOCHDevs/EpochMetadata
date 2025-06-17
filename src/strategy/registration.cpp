//
// Created by dewe on 9/11/24.
//
#include "epoch_metadata/strategy/registration.h"
#include "../doc_deserialization_helper.h"
#include "epoch_metadata/glaze_custom_types.h"
#include "epoch_metadata/strategy/enums.h"
#include "epoch_metadata/strategy/metadata.h"
#include "epoch_metadata/strategy/registry.h"
#include "epoch_metadata/strategy/strategy_config.h"
#include "epoch_metadata/strategy/ui_graph.h"
#include "epoch_metadata/transforms/registration.h"

#include "epoch_metadata/strategy/validation.h"

namespace epoch_metadata::strategy {
struct AIGeneratedAlgorithmMetaData {

  std::string name;
  std::string description;
  epoch_core::TradeSignalType algorithm_type;
  std::string prompt;
  std::string timestamp;
  UIData blueprint;
};

std::vector<AIGeneratedAlgorithmMetaData> LoadAIGeneratedAlgorithmMetaData(
    std::vector<std::string> const &aiGeneratedAlgorithms) {
  std::vector<AIGeneratedAlgorithmMetaData> configs;
  configs.reserve(aiGeneratedAlgorithms.size());

  for (auto const &config : aiGeneratedAlgorithms) {
    const auto doc = glz::read_json<AIGeneratedAlgorithmMetaData>(config);
    if (!doc) {
      SPDLOG_ERROR("Failed to parse JSON buffer:\n{}", glz::format_error(doc));
      continue;
    }
    configs.emplace_back(doc.value());
  }
  return configs;
}

void RegisterStrategyMetadata(
    FileLoaderInterface const &loader,
    std::vector<std::string> const &aiGeneratedAlgorithms) {
  transforms::RegisterTransformMetadata(loader);
  // TODO ADD FILTERS/SCREENER

  futures_continuation::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmBaseMetaData>(loader, "futures_continuation"));
  commission::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmBaseMetaData>(loader, "commission"));
  slippage::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmBaseMetaData>(loader, "slippage"));
  position_sizer::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmMetaData>(loader, "position_sizer"));
  stop_loss::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmMetaData>(loader, "stop_loss"));
  take_profit::Registry::GetInstance().Register(
      LoadFromFile<AlgorithmMetaData>(loader, "take_profit"));
  trade_signal::Registry::GetInstance().Register(
      LoadFromFile<TradeSignalMetaData>(loader, "trade_signals"));

  auto aiGenerated = LoadAIGeneratedAlgorithmMetaData(aiGeneratedAlgorithms);
  std::vector<StrategyTemplate> templates;
  templates.reserve(aiGenerated.size());

  for (auto const &[i, config] : std::views::enumerate(aiGenerated)) {
    auto converted = CreateAlgorithmMetaData(config.blueprint);
    if (!converted) {
      SPDLOG_ERROR("Failed to convert trade signal: {}",
                   FormatValidationIssues(converted.error()));
      continue;
    }

    bool requiresTimeframe = true;
    for (auto const &node : config.blueprint.nodes) {
      if (node.timeframe) {
        requiresTimeframe = false;
        break;
      }
    }

    TradeSignalMetaData trade_signal_metadata{
        .id = "ai_generated_" + std::to_string(i),
        .name = config.name,
        .options = converted->options,
        .desc = config.description,
        .requiresTimeframe = requiresTimeframe,
        .type = config.algorithm_type,
        .data = config.blueprint,
        .tags = {"ai_generated", "algorithm"}};

    trade_signal::Registry::GetInstance().Register(trade_signal_metadata);
    // strategy_templates::Registry::GetInstance().Register(
    //     {std::to_string(i), strategy, config.trade_signal_metadata.type});
  }
}

} // namespace epoch_metadata::strategy