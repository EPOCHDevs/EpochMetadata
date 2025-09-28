#ifndef TEARSHEET_OUTPUT_HPP
#define TEARSHEET_OUTPUT_HPP

#include "epoch_testing/transform_tester_base.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

#include "epoch_protos/common.pb.h"
#include "epoch_protos/table_def.pb.h"
#include "epoch_protos/tearsheet.pb.h"
#include "yaml-cpp/yaml.h"
#include <google/protobuf/util/message_differencer.h>

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
                // First check if this is a timestamp that YAML parsed as a date
                // YAML-cpp parses ISO timestamps like "2023-01-01T09:30:00" and converts them
                // We need to preserve the original string format
                std::string value;

                try {
                    // Try to get the raw scalar value
                    value = node.Scalar();
                } catch (...) {
                    // Fallback to as<string> which may truncate timestamps
                    value = node.as<std::string>();
                }

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

    template <> struct convert<epoch_proto::TableRow> {
        static bool decode(const Node &node, epoch_proto::TableRow &rhs) {
            if (!node.IsSequence()) return false;

            for (const auto& valueNode : node) {
                epoch_proto::Scalar* scalar = rhs.add_values();
                if (!convert<epoch_proto::Scalar>::decode(valueNode, *scalar)) {
                    return false;
                }
            }
            return true;
        }
    };

    template <> struct convert<epoch_proto::TableData> {
        static bool decode(const Node &node, epoch_proto::TableData &rhs) {
            if (!node.IsMap()) return false;

            if (node["rows"]) {
                if (node["rows"].IsSequence()) {
                    for (const auto& rowNode : node["rows"]) {
                        epoch_proto::TableRow* row = rhs.add_rows();
                        if (!convert<epoch_proto::TableRow>::decode(rowNode, *row)) {
                            return false;
                        }
                    }
                }
            }
            return true;
        }
    };

    template <> struct convert<epoch_proto::ColumnDef> {
        static bool decode(const Node &node, epoch_proto::ColumnDef &rhs) {
            if (!node.IsMap()) return false;

            if (node["name"]) {
                rhs.set_name(node["name"].as<std::string>());
            }
            if (node["type"]) {
                std::string typeStr = node["type"].as<std::string>();
                epoch_proto::EpochFolioType type;
                if (epoch_proto::EpochFolioType_Parse(typeStr, &type)) {
                    rhs.set_type(type);
                }
            }
            return true;
        }
    };

    template <> struct convert<epoch_proto::Table> {
        static bool decode(const Node &node, epoch_proto::Table &rhs) {
            if (!node.IsMap()) return false;

            if (node["title"]) {
                rhs.set_title(node["title"].as<std::string>());
            }
            if (node["category"]) {
                rhs.set_category(node["category"].as<std::string>());
            }
            if (node["type"]) {
                std::string typeStr = node["type"].as<std::string>();
                epoch_proto::EpochFolioDashboardWidget type;
                if (epoch_proto::EpochFolioDashboardWidget_Parse(typeStr, &type)) {
                    rhs.set_type(type);
                }
            }
            if (node["columns"]) {
                if (node["columns"].IsSequence()) {
                    for (const auto& colNode : node["columns"]) {
                        epoch_proto::ColumnDef* col = rhs.add_columns();
                        if (!convert<epoch_proto::ColumnDef>::decode(colNode, *col)) {
                            return false;
                        }
                    }
                }
            }
            if (node["data"]) {
                epoch_proto::TableData* data = rhs.mutable_data();
                if (!convert<epoch_proto::TableData>::decode(node["data"], *data)) {
                    return false;
                }
            }
            return true;
        }
    };

    template <> struct convert<epoch_proto::TableList> {
        static bool decode(const Node &node, epoch_proto::TableList &rhs) {
            if (node.IsSequence()) {
                // Handle array of Table directly
                for (const auto& tableNode : node) {
                    epoch_proto::Table* table = rhs.add_tables();
                    if (!convert<epoch_proto::Table>::decode(tableNode, *table)) {
                        return false;
                    }
                }
                return true;
            } else if (node.IsMap() && node["tables"]) {
                // Handle object with "tables" field
                if (node["tables"].IsSequence()) {
                    for (const auto& tableNode : node["tables"]) {
                        epoch_proto::Table* table = rhs.add_tables();
                        if (!convert<epoch_proto::Table>::decode(tableNode, *table)) {
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

            if (node["tables"]) {
                epoch_proto::TableList* tableList = rhs.mutable_tables();
                if (!convert<epoch_proto::TableList>::decode(node["tables"], *tableList)) {
                    return false;
                }
            }

            // Charts can be added later if needed

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
        // Avoid using DebugString() which can crash if descriptors aren't loaded
        return "ProtoBuf object (serialized size: " +
               std::to_string(self.ByteSizeLong()) + " bytes)";
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

        // Manual comparison of tearsheet contents to avoid protobuf serialization issues
        bool result = compareTearsheets(protoTearsheet, otherTearsheet->protoTearsheet);

        // Debug output if needed for troubleshooting
        if (!result) {

            // Expected tearsheet
            std::cerr << "\n=== EXPECTED ===" << std::endl;
            if (otherTearsheet->protoTearsheet.has_cards() && otherTearsheet->protoTearsheet.cards().cards_size() > 0) {
                std::cerr << "Cards: " << otherTearsheet->protoTearsheet.cards().cards_size() << " cards" << std::endl;
                for (int i = 0; i < otherTearsheet->protoTearsheet.cards().cards_size() && i < 3; ++i) {
                    auto& card = otherTearsheet->protoTearsheet.cards().cards(i);
                    std::cerr << "  Card " << i << ": category='" << card.category()
                              << "', group_size=" << card.group_size()
                              << ", data_count=" << card.data_size() << std::endl;
                    for (int j = 0; j < card.data_size() && j < 3; ++j) {
                        auto& data = card.data(j);
                        std::cerr << "    Data " << j << ": title='" << data.title()
                                  << "', group=" << data.group()
                                  << ", type=" << data.type();
                        if (data.has_value()) {
                            if (data.value().has_decimal_value()) {
                                std::cerr << ", value=" << data.value().decimal_value() << " (decimal)";
                            } else if (data.value().has_integer_value()) {
                                std::cerr << ", value=" << data.value().integer_value() << " (integer)";
                            } else if (data.value().has_boolean_value()) {
                                std::cerr << ", value=" << data.value().boolean_value() << " (boolean)";
                            } else if (data.value().has_string_value()) {
                                std::cerr << ", value='" << data.value().string_value() << "' (string)";
                            }
                        }
                        std::cerr << std::endl;
                    }
                }
            }
            if (otherTearsheet->protoTearsheet.has_tables()) {
                std::cerr << "Tables: " << otherTearsheet->protoTearsheet.tables().tables_size() << " tables" << std::endl;
            }
            if (otherTearsheet->protoTearsheet.has_charts()) {
                std::cerr << "Charts: " << otherTearsheet->protoTearsheet.charts().charts_size() << " charts" << std::endl;
            }

            // Actual tearsheet
            std::cerr << "\n=== ACTUAL ===" << std::endl;
            if (protoTearsheet.has_cards() && protoTearsheet.cards().cards_size() > 0) {
                std::cerr << "Cards: " << protoTearsheet.cards().cards_size() << " cards" << std::endl;
                for (int i = 0; i < protoTearsheet.cards().cards_size() && i < 3; ++i) {
                    auto& card = protoTearsheet.cards().cards(i);
                    std::cerr << "  Card " << i << ": category='" << card.category()
                              << "', group_size=" << card.group_size()
                              << ", data_count=" << card.data_size() << std::endl;
                    for (int j = 0; j < card.data_size() && j < 3; ++j) {
                        auto& data = card.data(j);
                        std::cerr << "    Data " << j << ": title='" << data.title()
                                  << "', group=" << data.group()
                                  << ", type=" << data.type();
                        if (data.has_value()) {
                            if (data.value().has_decimal_value()) {
                                std::cerr << ", value=" << data.value().decimal_value() << " (decimal)";
                            } else if (data.value().has_integer_value()) {
                                std::cerr << ", value=" << data.value().integer_value() << " (integer)";
                            } else if (data.value().has_boolean_value()) {
                                std::cerr << ", value=" << data.value().boolean_value() << " (boolean)";
                            } else if (data.value().has_string_value()) {
                                std::cerr << ", value='" << data.value().string_value() << "' (string)";
                            }
                        }
                        std::cerr << std::endl;
                    }
                }
            } else {
                std::cerr << "No cards section" << std::endl;
            }
            if (protoTearsheet.has_tables()) {
                std::cerr << "Tables: " << protoTearsheet.tables().tables_size() << " tables" << std::endl;
            }
            if (protoTearsheet.has_charts()) {
                std::cerr << "Charts: " << protoTearsheet.charts().charts_size() << " charts" << std::endl;
            }

            std::cerr << "================================================\n" << std::endl;
        }

        return result;
    }

private:
    bool compareTearsheets(const epoch_proto::TearSheet& a, const epoch_proto::TearSheet& b) const {
        // Compare cards section - treat no cards and empty cards list as equivalent
        bool a_has_cards = a.has_cards() && a.cards().cards_size() > 0;
        bool b_has_cards = b.has_cards() && b.cards().cards_size() > 0;

        if (a_has_cards != b_has_cards) return false;
        if (a_has_cards && !compareCardLists(a.cards(), b.cards())) return false;

        // Compare tables section
        if (a.has_tables() != b.has_tables()) return false;
        if (a.has_tables() && !compareTableLists(a.tables(), b.tables())) return false;

        // Compare charts section (if needed)
        if (a.has_charts() != b.has_charts()) return false;

        return true;
    }

    bool compareCardLists(const epoch_proto::CardDefList& a, const epoch_proto::CardDefList& b) const {
        if (a.cards_size() != b.cards_size()) return false;

        for (int i = 0; i < a.cards_size(); ++i) {
            if (!compareCards(a.cards(i), b.cards(i))) return false;
        }
        return true;
    }

    bool compareCards(const epoch_proto::CardDef& a, const epoch_proto::CardDef& b) const {
        if (a.type() != b.type()) return false;
        if (a.category() != b.category()) return false;
        if (a.group_size() != b.group_size()) return false;
        if (a.data_size() != b.data_size()) return false;

        for (int i = 0; i < a.data_size(); ++i) {
            if (!compareCardData(a.data(i), b.data(i))) return false;
        }
        return true;
    }

    bool compareCardData(const epoch_proto::CardData& a, const epoch_proto::CardData& b) const {
        if (a.title() != b.title()) return false;
        if (a.type() != b.type()) return false;
        if (a.group() != b.group()) return false;

        // Compare values with tolerance for floating point
        if (a.has_value() != b.has_value()) return false;
        if (a.has_value()) {
            return compareScalars(a.value(), b.value());
        }
        return true;
    }

    bool compareTableLists(const epoch_proto::TableList& a, const epoch_proto::TableList& b) const {
        if (a.tables_size() != b.tables_size()) return false;

        for (int i = 0; i < a.tables_size(); ++i) {
            if (!compareTables(a.tables(i), b.tables(i))) return false;
        }
        return true;
    }

    bool compareTables(const epoch_proto::Table& a, const epoch_proto::Table& b) const {
        if (a.title() != b.title()) return false;
        if (a.category() != b.category()) return false;
        if (a.type() != b.type()) return false;
        if (a.columns_size() != b.columns_size()) return false;

        // Compare columns
        for (int i = 0; i < a.columns_size(); ++i) {
            if (!compareColumns(a.columns(i), b.columns(i))) return false;
        }

        // Compare table data
        if (a.has_data() != b.has_data()) return false;
        if (a.has_data()) {
            if (!compareTableData(a.data(), b.data())) return false;
        }
        return true;
    }

    bool compareColumns(const epoch_proto::ColumnDef& a, const epoch_proto::ColumnDef& b) const {
        return a.name() == b.name() && a.type() == b.type();
    }

    bool compareTableData(const epoch_proto::TableData& a, const epoch_proto::TableData& b) const {
        if (a.rows_size() != b.rows_size()) return false;

        for (int i = 0; i < a.rows_size(); ++i) {
            if (!compareTableRows(a.rows(i), b.rows(i))) return false;
        }
        return true;
    }

    bool compareTableRows(const epoch_proto::TableRow& a, const epoch_proto::TableRow& b) const {
        if (a.values_size() != b.values_size()) return false;

        for (int i = 0; i < a.values_size(); ++i) {
            if (!compareScalars(a.values(i), b.values(i))) return false;
        }
        return true;
    }

    bool compareScalars(const epoch_proto::Scalar& a, const epoch_proto::Scalar& b) const {
        // Check that both have same type of value
        if (a.has_decimal_value() != b.has_decimal_value()) return false;
        if (a.has_integer_value() != b.has_integer_value()) return false;
        if (a.has_boolean_value() != b.has_boolean_value()) return false;
        if (a.has_string_value() != b.has_string_value()) return false;
        if (a.has_null_value() != b.has_null_value()) return false;

        // Compare values with appropriate tolerance
        if (a.has_decimal_value()) {
            // Use small epsilon for floating point comparison
            const double epsilon = 1e-9;
            return std::abs(a.decimal_value() - b.decimal_value()) < epsilon;
        }
        if (a.has_integer_value()) {
            return a.integer_value() == b.integer_value();
        }
        if (a.has_boolean_value()) {
            return a.boolean_value() == b.boolean_value();
        }
        if (a.has_string_value()) {
            return a.string_value() == b.string_value();
        }
        if (a.has_null_value()) {
            return true; // Both are null
        }

        return true; // Both have no value
    }

    void printScalar(const epoch_proto::Scalar& scalar) const {
        if (scalar.has_decimal_value()) {
            std::cerr << scalar.decimal_value() << " (decimal)" << std::endl;
        } else if (scalar.has_integer_value()) {
            std::cerr << scalar.integer_value() << " (integer)" << std::endl;
        } else if (scalar.has_boolean_value()) {
            std::cerr << (scalar.boolean_value() ? "true" : "false") << " (boolean)" << std::endl;
        } else if (scalar.has_string_value()) {
            std::cerr << "'" << scalar.string_value() << "' (string)" << std::endl;
        } else if (scalar.has_null_value()) {
            std::cerr << "null" << std::endl;
        } else {
            std::cerr << "(no value)" << std::endl;
        }
    }

public:

    std::string DebugCards(epoch_proto::CardDefList const& cards) const {
        std::stringstream ss;
        ss << "  Cards section:\n";
        for (int i = 0; i < cards.cards_size(); ++i) {
            const epoch_proto::CardDef& card = cards.cards(i);
            ss << "    Card " << i << ":\n";
            ss << "      type: " << card.type() << "\n";
            ss << "      category: " << card.category() << "\n";
            ss << "      group_size: " << card.group_size() << "\n";
            ss << "      data:\n";
            for (int j = 0; j < card.data_size(); ++j) {
                const auto& card_data = card.data(j);
                ss << "        [" << j << "] title: " << card_data.title() << "\n";
                ss << "            type: " << card_data.type() << "\n";
                ss << "            group: " << card_data.group() << "\n";
                ss << "            value: ";
                if (card_data.has_value()) {
                    if (card_data.value().has_decimal_value()) {
                        ss << card_data.value().decimal_value() << " (decimal)";
                    } else if (card_data.value().has_integer_value()) {
                        ss << card_data.value().integer_value() << " (integer)";
                    } else if (card_data.value().has_boolean_value()) {
                        ss << (card_data.value().boolean_value() ? "true" : "false") << " (boolean)";
                    } else if (card_data.value().has_string_value()) {
                        ss << "'" << card_data.value().string_value() << "' (string)";
                    } else if (card_data.value().has_null_value()) {
                        ss << "null";
                    } else {
                        ss << "(unknown type)";
                    }
                } else {
                    ss << "(no value)";
                }
                ss << "\n";
            }
        }
        return ss.str();
    }

    std::string DebugTables(epoch_proto::TableList const& tables) const {
        std::stringstream ss;
        ss << "  Tables section:\n";
        for (int i = 0; i < tables.tables_size(); ++i) {
            const epoch_proto::Table& table = tables.tables(i);
            ss << "    Table " << i << ":\n";
            ss << "      title: " << table.title() << "\n";
            ss << "      type: " << table.type() << "\n";
            ss << "      category: " << table.category() << "\n";
            ss << "      columns (" << table.columns_size() << "):\n";
            for (int j = 0; j < table.columns_size(); ++j) {
                const auto& col = table.columns(j);
                ss << "        [" << j << "] " << col.name() << " (" << col.type() << ")\n";
            }
            if (table.has_data()) {
                ss << "      rows (" << table.data().rows_size() << "):\n";
                for (int j = 0; j < table.data().rows_size() && j < 5; ++j) { // Show first 5 rows
                    const auto& row = table.data().rows(j);
                    ss << "        [" << j << "] ";
                    for (int k = 0; k < row.values_size(); ++k) {
                        if (k > 0) ss << ", ";
                        const auto& value = row.values(k);
                        if (value.has_decimal_value()) {
                            ss << value.decimal_value();
                        } else if (value.has_integer_value()) {
                            ss << value.integer_value();
                        } else if (value.has_boolean_value()) {
                            ss << (value.boolean_value() ? "true" : "false");
                        } else if (value.has_string_value()) {
                            ss << "\"" << value.string_value() << "\"";
                        } else if (value.has_null_value()) {
                            ss << "null";
                        } else {
                            ss << "(unknown)";
                        }
                    }
                    ss << "\n";
                }
                if (table.data().rows_size() > 5) {
                    ss << "        ... (" << (table.data().rows_size() - 5) << " more rows)\n";
                }
            } else {
                ss << "      rows: no data\n";
            }
        }
        return ss.str();
    }

    std::string toString() const override {
        // Simple string representation without using protobuf descriptor methods
        std::stringstream ss;
        ss << "Tearsheet Output:\n";

        // Just report presence of sections without trying to serialize
        if (protoTearsheet.has_cards() && protoTearsheet.cards().cards_size() > 0) {
            ss << DebugCards(protoTearsheet.cards());
        }

        if (protoTearsheet.has_charts()) {
            ss << "  Has charts section\n";
        }

        if (protoTearsheet.has_tables()) {
            ss << DebugTables(protoTearsheet.tables());
        }

        if (!protoTearsheet.has_cards() && !protoTearsheet.has_charts() && !protoTearsheet.has_tables()) {
            ss << "  Empty tearsheet\n";
        }

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