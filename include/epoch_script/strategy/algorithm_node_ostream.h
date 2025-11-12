#pragma once

#include "metadata.h"
#include <iostream>
#include <sstream>

namespace epoch_script::strategy {

// Helper to print InputMapping
inline std::string inputMappingToString(const InputMapping& inputs) {
    if (inputs.empty()) return "{}";

    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, values] : inputs) {
        if (!first) oss << ", ";
        first = false;
        oss << "\"" << key << "\": [";
        bool firstVal = true;
        for (const auto& val : values) {
            if (!firstVal) oss << ", ";
            firstVal = false;
            oss << "\"" << val << "\"";
        }
        oss << "]";
    }
    oss << "}";
    return oss.str();
}

// Helper to print MetaDataArgDefinitionMapping
inline std::string optionsToString(const epoch_script::MetaDataArgDefinitionMapping& options) {
    if (options.empty()) return "{}";

    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : options) {
        if (!first) oss << ", ";
        first = false;
        oss << "\"" << key << "\": ";

        // Use the ToString() method which handles all variant types
        try {
            oss << "\"" << value.ToString() << "\"";
        } catch (...) {
            oss << "<error>";
        }
    }
    oss << "}";
    return oss.str();
}

// Helper to print SessionVariant
inline std::string sessionToString(const std::optional<SessionVariant>& session) {
    if (!session) return "null";

    return std::visit([](const auto& s) -> std::string {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return "\"" + s + "\"";
        } else {
            return "<session>";
        }
    }, *session);
}

// operator<< for AlgorithmNode
inline std::ostream& operator<<(std::ostream& os, const AlgorithmNode& node) {
    os << "AlgorithmNode{\n";
    os << "  type: \"" << node.type << "\",\n";
    os << "  id: \"" << node.id << "\",\n";
    os << "  options: " << optionsToString(node.options) << ",\n";
    os << "  inputs: " << inputMappingToString(node.inputs) << ",\n";

    if (node.timeframe) {
        os << "  timeframe: \"" << node.timeframe->ToString() << "\",\n";
    } else {
        os << "  timeframe: null,\n";
    }

    os << "  session: " << sessionToString(node.session) << "\n";
    os << "}";

    return os;
}

// operator<< for vector<AlgorithmNode> (CompilationResult)
inline std::ostream& operator<<(std::ostream& os, const std::vector<AlgorithmNode>& nodes) {
    os << "[\n";
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (i > 0) os << ",\n";
        os << "  " << nodes[i];
    }
    os << "\n]";
    return os;
}

} // namespace epoch_script::strategy
