//
// Created by dewe on 9/11/24.
//
#include "epoch_metadata/strategy/registration.h"
#include "epoch_metadata/transforms/registration.h"
#include "../doc_deserialization_helper.h"
#include "epoch_metadata/strategy/enums.h"
#include "epoch_metadata/strategy/metadata.h"
#include "epoch_metadata/strategy/registry.h"
#include "epoch_metadata/strategy/strategy_config.h"
#include "epoch_metadata/glaze_custom_types.h"


namespace epoch_metadata::strategy {
    struct AIGeneratedStrategyMetaData {
        std::string name;
        std::string description;
        epoch_core::TradeSignalType algorithm_type;
        std::string source_algorithm;
        std::string generated_at;
        StrategyConfig strategy;
        TradeSignalMetaData trade_signal_metadata;
    };

    std::vector<AIGeneratedStrategyMetaData> LoadAIGeneratedStrategyMetaData(std::vector<std::string> const& aiGeneratedStrategies) {
        std::vector<AIGeneratedStrategyMetaData> configs;
        configs.reserve(aiGeneratedStrategies.size());

        for (auto const& config: aiGeneratedStrategies) {
            const auto doc = glz::read_json<AIGeneratedStrategyMetaData>(config);
            if (!doc) {
                SPDLOG_ERROR("Failed to parse JSON buffer:\n{}", glz::format_error(doc));
                continue;
            }
            configs.emplace_back(doc.value());
        }
        return configs;
    }

void RegisterStrategyMetadata(FileLoaderInterface const &loader, std::vector<std::string> const& aiGeneratedStrategies) {
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
            LoadFromFile<AlgorithmMetaData>(loader, "exit_signals"));
        take_profit::Registry::GetInstance().Register(
            LoadFromFile<AlgorithmMetaData>(loader, "exit_signals"));
        trade_signal::Registry::GetInstance().Register(
            LoadFromFile<TradeSignalMetaData>(loader, "trade_signals"));

        auto aiGenerated = LoadAIGeneratedStrategyMetaData(aiGeneratedStrategies);
        std::vector<StrategyTemplate> templates;
        templates.reserve(aiGenerated.size());

        for (auto const& [i, config]: std::views::enumerate(aiGenerated)) {
            trade_signal::Registry::GetInstance().Register(config.trade_signal_metadata);
            strategy_templates::Registry::GetInstance().Register({
                std::to_string(i),
                 config.strategy,
                config.trade_signal_metadata.type
            });
        }
    }

} // namespace epoch_metadata::strategy