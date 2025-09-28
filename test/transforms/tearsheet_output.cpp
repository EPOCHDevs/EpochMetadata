#include "epoch_testing/tearsheet_output.hpp"
#include <stdexcept>

namespace epoch {
namespace test {

using namespace epoch::test;

static Value parseCardValue(const YAML::Node& node) {
    if (node.IsScalar()) {
        try {
            return Value(node.as<double>());
        } catch (...) {
            try {
                return Value(node.as<bool>());
            } catch (...) {
                return Value(node.as<std::string>());
            }
        }
    }
    throw std::runtime_error("Invalid card value type");
}

static Card parseCard(const YAML::Node& node) {
    Card card;
    if (node["title"]) {
        card.title = node["title"].as<std::string>();
    }
    if (node["value"]) {
        card.value = parseCardValue(node["value"]);
    }
    return card;
}

static ChartData parseChartData(const YAML::Node& node) {
    ChartData data;
    if (node["name"]) {
        data.name = node["name"].as<std::string>();
    }
    if (node["values"]) {
        for (const auto& val : node["values"]) {
            data.values.push_back(val.as<double>());
        }
    }
    if (node["value"]) {
        data.values.push_back(node["value"].as<double>());
    }
    if (node["points"]) {
        for (const auto& point : node["points"]) {
            std::string x = point["x"].as<std::string>();
            double y = point["y"].as<double>();
            data.points.push_back({x, y});
        }
    }
    return data;
}

static Chart parseChart(const YAML::Node& node) {
    Chart chart;
    if (node["type"]) {
        chart.type = node["type"].as<std::string>();
    }
    if (node["title"]) {
        chart.title = node["title"].as<std::string>();
    }
    if (node["bins"]) {
        chart.bins = node["bins"].as<int>();
    }
    if (node["x_axis"] && node["x_axis"]["categories"]) {
        for (const auto& cat : node["x_axis"]["categories"]) {
            chart.categories.push_back(cat.as<std::string>());
        }
    }
    if (node["data"]) {
        for (const auto& dataNode : node["data"]) {
            chart.data.push_back(parseChartData(dataNode));
        }
    }
    return chart;
}

static TableColumn parseTableColumn(const YAML::Node& node) {
    TableColumn col;
    if (node["name"]) {
        col.name = node["name"].as<std::string>();
    }
    if (node["type"]) {
        col.type = node["type"].as<std::string>();
    }
    return col;
}

static Value parseTableValue(const YAML::Node& node, const std::string& colType) {
    if (!node || node.IsNull()) {
        return Value(std::nan(""));
    }

    if (colType == "integer" || colType == "percent" || colType == "double") {
        return Value(node.as<double>());
    } else if (colType == "boolean") {
        return Value(node.as<bool>());
    } else {
        // For strings, handle timestamps that YAML might have parsed
        // Check if this node has a tag indicating it's a timestamp
        if (!node.Tag().empty() && node.Tag() == "tag:yaml.org,2002:timestamp") {
            // YAML parsed this as a timestamp, we need to reconstruct it
            // The Scalar() method should give us something, but YAML has already parsed it
            std::string scalar = node.Scalar();

            // If we only got a year (like "2023"), this is the YAML timestamp parsing issue
            // Unfortunately, we can't recover the original string once YAML has parsed it
            // The best we can do is return what we have
            return Value(scalar);
        }

        // Regular string handling
        if (node.IsScalar()) {
            return Value(node.Scalar());
        }
        return Value(node.as<std::string>());
    }
}

static TableRow parseTableRow(const YAML::Node& node, const std::vector<TableColumn>& columns) {
    TableRow row;
    if (node.IsSequence()) {
        size_t i = 0;
        for (const auto& val : node) {
            std::string colType = (i < columns.size()) ? columns[i].type : "string";
            row.values.push_back(parseTableValue(val, colType));
            i++;
        }
    }
    return row;
}

static ReportTable parseTable(const YAML::Node& node) {
    ReportTable table;
    if (node["title"]) {
        table.title = node["title"].as<std::string>();
    }
    if (node["type"]) {
        table.type = node["type"].as<std::string>();
    }
    if (node["columns"]) {
        for (const auto& col : node["columns"]) {
            table.columns.push_back(parseTableColumn(col));
        }
    }
    if (node["rows"]) {
        for (const auto& row : node["rows"]) {
            table.rows.push_back(parseTableRow(row, table.columns));
        }
    }
    return table;
}

// fromYAML implementation moved to tearsheet_output.hpp to use protobuf-based parsing

} // namespace test
} // namespace epoch