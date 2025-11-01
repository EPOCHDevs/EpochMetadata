//
// Created by dewe on 9/11/24.
//
#include <epochflow/strategy/registration.h>
#include "../core/doc_deserialization_helper.h"
#include <epochflow/core/glaze_custom_types.h>
#include <epochflow/strategy/data_options.h>
#include <epochflow/strategy/enums.h>
#include <epochflow/strategy/generic_function.h>
#include <epochflow/strategy/metadata.h>
#include <epochflow/strategy/registry.h>
#include <epochflow/strategy/strategy_config.h>
#include <epochflow/transforms/core/registration.h>
#include "transforms/compiler/ast_compiler.h"
#include <unordered_map>

namespace epochflow::strategy
{
  struct AIGeneratedAlgorithmMetaData
  {
    std::string id;
    std::string name;
    std::string description;
    epoch_core::TradeSignalType algorithm_type;
    std::string prompt;
    std::string timestamp;
    PythonSource source;
    std::vector<std::string> tags;
  };

  struct AIGeneratedStrategyMetaData
  {
    std::string id;
    std::string name;
    std::string description;
    DataOption assets;
    std::optional<GenericFunction> trade_signal;
    std::optional<GenericFunction> position_sizer;
    std::optional<GenericFunction> stop_loss;
    std::optional<GenericFunction> take_profit;
    epoch_core::TradeSignalType category;
  };

  template <typename T>
  std::vector<T>
  LoadMetaDataT(std::vector<std::string> const &aiGeneratedAlgorithms)
  {
    std::vector<T> configs;
    configs.reserve(aiGeneratedAlgorithms.size());

    for (auto const &config : aiGeneratedAlgorithms)
    {
      const auto doc = glz::read_json<T>(config);
      if (!doc)
      {
        SPDLOG_ERROR("Failed to parse JSON buffer:\n{}", glz::format_error(doc));
        continue;
      }
      configs.emplace_back(doc.value());
    }
    return configs;
  }

  void RegisterStrategyMetadata(
      FileLoaderInterface const &loader,
      std::vector<std::string> const &aiGeneratedAlgorithms,
      std::vector<std::string> const &aiGeneratedStrategies)
  {
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

    std::unordered_map<std::string, int> duplicateIdCount;
    auto aiGenerated =
        LoadMetaDataT<AIGeneratedAlgorithmMetaData>(aiGeneratedAlgorithms);
    for (auto [i, config] : std::views::enumerate(aiGenerated))
    {
      // PythonSource has already been compiled during deserialization
      if (config.source.GetCompilationResult().empty())
      {
        SPDLOG_ERROR("{}: Failed to compile Python source - empty result", config.name);
        continue;
      }

      if (duplicateIdCount.contains(config.id))
      {
        ++duplicateIdCount[config.id];
        config.id = config.id + "_" + std::to_string(duplicateIdCount[config.id]);
      }
      else
      {
        duplicateIdCount[config.id] = 0;
      }

      // Extract options from compiled result
      MetaDataOptionList options;
      // TODO: Extract exposed options from compiled AlgorithmNodes

      // Determine if timeframe is required (check if any node has explicit timeframe)
      bool requiresTimeframe = true;
      for (const auto &node : config.source.GetCompilationResult())
      {
        if (node.timeframe.has_value())
        {
          requiresTimeframe = false;
          break;
        }
      }

      TradeSignalMetaData trade_signal_metadata{.id = config.id,
                                                .name = config.name,
                                                .options = options,
                                                .desc = config.description,
                                                .requiresTimeframe =
                                                    requiresTimeframe,
                                                .source = config.source,
                                                .tags = config.tags};

      trade_signal::Registry::GetInstance().Register(trade_signal_metadata);
    }

    std::vector<StrategyTemplate> templates;
    auto aiGeneratedStrategiesT =
        LoadMetaDataT<AIGeneratedStrategyMetaData>(aiGeneratedStrategies);
    duplicateIdCount.clear();
    for (auto [i, config] : std::views::enumerate(aiGeneratedStrategiesT))
    {

      if (!config.trade_signal)
      {
        SPDLOG_ERROR("Failed to convert {} trade signal", config.id);
        continue;
      }

      if (duplicateIdCount.contains(config.id))
      {
        ++duplicateIdCount[config.id];
        config.id = config.id + "_" + std::to_string(duplicateIdCount[config.id]);
      }
      else
      {
        duplicateIdCount[config.id] = 0;
      }

      auto tradeSignalId = config.trade_signal->type.value();
      auto optionalSavedStrategy =
          trade_signal::Registry::GetInstance().GetMetaData(tradeSignalId);
      if (!optionalSavedStrategy)
      {
        SPDLOG_ERROR("Failed to find trade signal: {} in registry",
                     tradeSignalId);
        continue;
      }
      const auto savedStrategy = optionalSavedStrategy.value().get();
      StrategyConfig strategyConfig{.name = config.name,
                                    .description = config.description,
                                    .data = config.assets,
                                    .trade_signal = config.trade_signal.value(),
                                    .position_sizer = config.position_sizer,
                                    .take_profit = config.take_profit,
                                    .stop_loss = config.stop_loss};

      StrategyTemplate strategy{.id = config.id,
                                .strategy = strategyConfig,
                                .category = config.category};

      // Use the PythonSource from the saved strategy (already compiled)
      strategy.strategy.trade_signal.source = savedStrategy.source;
      strategy_templates::Registry::GetInstance().Register(strategy);
    }
  }

} // namespace epochflow::strategy