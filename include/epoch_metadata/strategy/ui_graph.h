#pragma once
#include "metadata.h"
#include "ui_data.h"

namespace epoch_metadata::strategy {
    std::expected<PartialTradeSignalMetaData, std::string>
CreateAlgorithmMetaData(const UIData &data);

    std::expected<UIData, std::string>
    CreateUIData(const PartialTradeSignalMetaData &data);

    // Layout function using Graphviz dot algorithm with defaults
    std::expected<UIData, std::string> AlignHorizontally(const UIData &data);
} // namespace epoch_stratifyx::server