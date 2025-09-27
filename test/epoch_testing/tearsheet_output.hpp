#ifndef TEARSHEET_OUTPUT_HPP
#define TEARSHEET_OUTPUT_HPP

#include "epoch_testing/transform_tester_base.hpp"
#include <sstream>
#include <iomanip>

#include "epoch_protos/common.pb.h"
#include "epoch_protos/table_def.pb.h"
#include "epoch_protos/tearsheet.pb.h"
#include "yaml-cpp/yaml.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

namespace YAML {
    template <> struct convert<epoch_proto::Scalar> {
        static bool decode(const Node &node, epoch_proto::Scalar &rhs) {
            if (node.IsNull()) {
                rhs.set_null_value(epoch_proto::NULL_VALUE);
                return true;
            }

            if (node.IsScalar()) {
                std::string value = node.as<std::string>();

                // Try to parse as boolean first
                if (value == "true" || value == "false") {
                    rhs.set_boolean_value(value == "true");
                    return true;
                }

                // Try to parse as number
                try {
                    if (value.find('.') != std::string::npos) {
                        // Decimal number
                        rhs.set_decimal_value(std::stod(value));
                        return true;
                    } else {
                        // Integer
                        rhs.set_integer_value(std::stoll(value));
                        return true;
                    }
                } catch (...) {
                    // If number parsing fails, treat as string
                    rhs.set_string_value(value);
                    return true;
                }
            }

            // Handle direct typed values
            if (node.Tag() == "!string") {
                rhs.set_string_value(node.as<std::string>());
                return true;
            } else if (node.Tag() == "!int") {
                rhs.set_integer_value(node.as<int64_t>());
                return true;
            } else if (node.Tag() == "!float") {
                rhs.set_decimal_value(node.as<double>());
                return true;
            } else if (node.Tag() == "!bool") {
                rhs.set_boolean_value(node.as<bool>());
                return true;
            }

            // For other node types, try basic conversion
            try {
                if (node.IsScalar()) {
                    if (node.as<std::string>() == "null") {
                        rhs.set_null_value(epoch_proto::NULL_VALUE);
                        return true;
                    }
                    rhs.set_string_value(node.as<std::string>());
                    return true;
                }
            } catch (...) {
                return false;
            }

            return false;
        }
    };

    template <> struct convert<epoch_proto::CardData> {
        static bool decode(const Node &node, epoch_proto::CardData &rhs) {
            if (!node.IsMap()) return false;

            if (node["title"]) {
                rhs.set_title(node["title"].as<std::string>());
            }

            if (node["value"]) {
                epoch_proto::Scalar* scalar = rhs.mutable_value();
                if (!convert<epoch_proto::Scalar>::decode(node["value"], *scalar)) {
                    return false;
                }
            }

            if (node["type"]) {
                std::string typeStr = node["type"].as<std::string>();
                epoch_proto::EpochFolioType type;
                if (epoch_proto::EpochFolioType_Parse(typeStr, &type)) {
                    rhs.set_type(type);
                } else {
                    // Default to TypeDecimal for numeric values
                    rhs.set_type(epoch_proto::TypeDecimal);
                }
            }

            if (node["group"]) {
                rhs.set_group(node["group"].as<uint64_t>());
            }

            return true;
        }
    };

    template <> struct convert<epoch_proto::CardDef> {
        static bool decode(const Node &node, epoch_proto::CardDef &rhs) {
            if (!node.IsMap()) return false;

            if (node["category"]) {
                rhs.set_category(node["category"].as<std::string>());
            }

            if (node["data"]) {
                if (node["data"].IsSequence()) {
                    for (const auto& cardNode : node["data"]) {
                        epoch_proto::CardData* cardData = rhs.add_data();
                        if (!convert<epoch_proto::CardData>::decode(cardNode, *cardData)) {
                            return false;
                        }
                    }
                }
            }

            if (node["group_size"]) {
                rhs.set_group_size(node["group_size"].as<uint64_t>());
            }

            if (node["type"]) {
                std::string typeStr = node["type"].as<std::string>();
                epoch_proto::EpochFolioDashboardWidget type;
                if (epoch_proto::EpochFolioDashboardWidget_Parse(typeStr, &type)) {
                    rhs.set_type(type);
                }
            }

            return true;
        }
    };

    template <> struct convert<epoch_proto::CardDefList> {
        static bool decode(const Node &node, epoch_proto::CardDefList &rhs) {
            if (node.IsSequence()) {
                // Handle array of CardDef directly
                for (const auto& cardDefNode : node) {
                    epoch_proto::CardDef* cardDef = rhs.add_cards();
                    if (!convert<epoch_proto::CardDef>::decode(cardDefNode, *cardDef)) {
                        return false;
                    }
                }
                return true;
            } else if (node.IsMap() && node["cards"]) {
                // Handle object with "cards" field
                if (node["cards"].IsSequence()) {
                    for (const auto& cardDefNode : node["cards"]) {
                        epoch_proto::CardDef* cardDef = rhs.add_cards();
                        if (!convert<epoch_proto::CardDef>::decode(cardDefNode, *cardDef)) {
                            return false;
                        }
                    }
                }
                return true;
            }
            return false;
        }
    };

    template <> struct convert<epoch_proto::TearSheet> {
        static bool decode(const Node &node, epoch_proto::TearSheet &rhs) {
            if (!node.IsMap()) return false;

            if (node["cards"]) {
                epoch_proto::CardDefList* cardList = rhs.mutable_cards();
                if (!convert<epoch_proto::CardDefList>::decode(node["cards"], *cardList)) {
                    return false;
                }
            }

            // For now, we'll focus on cards since that's what our tests use
            // Charts and tables can be added later if needed

            return true;
        }
    };
}

template <class ProtoBufType>
struct ProtoBufTypeMatcher : Catch::Matchers::MatcherGenericBase {
    ProtoBufTypeMatcher(ProtoBufType const& range):
        self{ range }
    {}

    bool match(ProtoBufType const& other) const {
        return self.SerializeAsString() == other.SerializeAsString();
    }

    std::string describe() const override {
        return self.DebugString();
    }

private:
    ProtoBufType const& self;
};


namespace epoch {
namespace test {

struct Card {
    std::string title;
    Value value;

    bool equals(const Card& other) const {
        return title == other.title && value == other.value;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << "  - " << title << ": ";
        if (std::holds_alternative<double>(value)) {
            ss << std::get<double>(value);
        } else if (std::holds_alternative<bool>(value)) {
            ss << (std::get<bool>(value) ? "true" : "false");
        } else {
            ss << std::get<std::string>(value);
        }
        return ss.str();
    }
};

struct ChartData {
    std::string name;
    std::vector<double> values;
    std::vector<std::pair<std::string, double>> points;

    bool equals(const ChartData& other) const {
        return name == other.name && values == other.values && points == other.points;
    }
};

struct Chart {
    std::string type;
    std::string title;
    std::vector<ChartData> data;
    std::vector<std::string> categories;
    int bins = 0;

    bool equals(const Chart& other) const {
        return type == other.type &&
               title == other.title &&
               data.size() == other.data.size() &&
               std::equal(data.begin(), data.end(), other.data.begin(),
                   [](const ChartData& a, const ChartData& b) { return a.equals(b); }) &&
               categories == other.categories &&
               bins == other.bins;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << "Chart[" << type << "]: " << title;
        if (bins > 0) {
            ss << " (bins=" << bins << ")";
        }
        if (!categories.empty()) {
            ss << "\n    Categories: ";
            for (const auto& cat : categories) {
                ss << cat << " ";
            }
        }
        for (const auto& d : data) {
            ss << "\n    Data[" << d.name << "]: ";
            if (!d.values.empty()) {
                for (auto v : d.values) ss << v << " ";
            }
            if (!d.points.empty()) {
                for (const auto& [x, y] : d.points) {
                    ss << "(" << x << "," << y << ") ";
                }
            }
        }
        return ss.str();
    }
};

struct TableColumn {
    std::string name;
    std::string type;

    bool equals(const TableColumn& other) const {
        return name == other.name && type == other.type;
    }
};

struct TableRow {
    std::vector<Value> values;

    bool equals(const TableRow& other) const {
        return values == other.values;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << "    [";
        bool first = true;
        for (const auto& val : values) {
            if (!first) ss << ", ";
            first = false;
            if (std::holds_alternative<double>(val)) {
                ss << std::get<double>(val);
            } else if (std::holds_alternative<bool>(val)) {
                ss << (std::get<bool>(val) ? "true" : "false");
            } else {
                ss << "\"" << std::get<std::string>(val) << "\"";
            }
        }
        ss << "]";
        return ss.str();
    }
};

struct ReportTable {
    std::string title;
    std::string type = "table";
    std::vector<TableColumn> columns;
    std::vector<TableRow> rows;

    bool equals(const ReportTable& other) const {
        if (title != other.title || type != other.type) return false;
        if (columns.size() != other.columns.size()) return false;
        if (rows.size() != other.rows.size()) return false;

        for (size_t i = 0; i < columns.size(); ++i) {
            if (!columns[i].equals(other.columns[i])) return false;
        }

        for (size_t i = 0; i < rows.size(); ++i) {
            if (!rows[i].equals(other.rows[i])) return false;
        }

        return true;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << "Table: " << title << " (" << type << ")\n";
        ss << "  Columns: ";
        for (const auto& col : columns) {
            ss << col.name << "(" << col.type << ") ";
        }
        ss << "\n  Rows:\n";
        for (const auto& row : rows) {
            ss << row.toString() << "\n";
        }
        return ss.str();
    }
};

class TearsheetOutput : public IOutputType {
public:
    std::vector<Card> cards;
    std::vector<Chart> charts;
    std::vector<ReportTable> tables;

    // Store the protobuf for direct comparison
    epoch_proto::TearSheet protoTearsheet;

    TearsheetOutput() = default;

    std::string getType() const override { return "tearsheet"; }

    bool equals(const IOutputType& other) const override {
        auto* otherTearsheet = dynamic_cast<const TearsheetOutput*>(&other);
        if (!otherTearsheet) return false;

        // Simple protobuf comparison using SerializeAsString
        return protoTearsheet.SerializeAsString() == otherTearsheet->protoTearsheet.SerializeAsString();
    }

    std::string toString() const override {
        std::stringstream ss;
        ss << "Tearsheet Output (Protobuf):\n";
        ss << protoTearsheet.DebugString();
        return ss.str();
    }

    static std::unique_ptr<IOutputType> fromYAML(const YAML::Node& node) {
        auto output = std::make_unique<TearsheetOutput>();

        // Use node.as<T>() to automatically invoke the YAML::convert
        output->protoTearsheet = node.as<epoch_proto::TearSheet>();

        return output;
    }
};

inline void registerTearsheetType() {
    OutputTypeRegistry::instance().registerType("tearsheet",
        [](const YAML::Node& node) { return TearsheetOutput::fromYAML(node); });
}

} // namespace test
} // namespace epoch

#endif // TEARSHEET_OUTPUT_HPP