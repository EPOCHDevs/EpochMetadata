#ifndef JSON_TRANSFORM_TESTER_HPP
#define JSON_TRANSFORM_TESTER_HPP

#include <glaze/glaze.hpp>
#include <vector>
#include <map>
#include <optional>
#include <variant>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>


namespace epoch {
namespace test {
namespace json {

// Column data types - using variant for mixed types
using ColumnValue = std::variant<double, int64_t, bool, std::string, std::nullptr_t>;
using ColumnData = std::vector<ColumnValue>;

// Card data for tearsheet cards
struct CardData {
    std::string title;
    ColumnValue value;
    int64_t group = 0;
    std::string type;
};

// Card definition for tearsheet
struct TearsheetCard {
    std::string category;
    int64_t group_size = 1;
    std::string type = "WidgetCard";
    std::vector<CardData> data;
};

// Card list structure
struct CardsList {
    std::vector<TearsheetCard> cards;
};

// Table column definition for tearsheet tables
struct TableColumn {
    std::string name;
    std::string type;
};

// Table data rows
struct TableData {
    std::vector<std::vector<ColumnValue>> rows;
};

// Tearsheet table structure
struct TearsheetTable {
    std::string title;
    std::string category;
    std::string type;
    std::vector<TableColumn> columns;
    TableData data;
};

// Chart axis data
struct ChartAxisData {
    std::vector<ColumnValue> data;
    std::string type;
};

// Chart line data
struct ChartLineData {
    std::string name;
    std::vector<ColumnValue> data;
    std::string type;
};

// Bar chart data structure (supports both single value and array of values)
struct BarCategory {
    std::string name;
    ColumnValue value;  // For simple bars (single value)
    std::vector<ColumnValue> data;  // For series (array of values)
};

// Pie chart slice structure
struct PieChartSlice {
    std::string label;
    ColumnValue value;
};

// Pie chart series structure (for nested pies)
struct PieChartSeries {
    std::string name;
    uint32_t size;
    uint32_t inner_size;
    std::vector<PieChartSlice> data;
};

// Histogram bin structure
struct HistogramBin {
    double min;
    double max;
    int64_t count;
};

// Tearsheet chart structure - simplified for JSON parsing
struct TearsheetChart {
    std::string title;
    std::string category;
    std::string type;  // WidgetLinesChart, WidgetBarChart, etc.

    // For Lines/Area charts
    std::optional<ChartAxisData> x_axis;
    std::optional<std::vector<ChartLineData>> lines;

    // For Bar charts
    std::optional<std::vector<BarCategory>> bars;
    std::optional<bool> vertical;
    std::optional<bool> stacked;

    // For Pie charts
    std::optional<std::vector<PieChartSlice>> slices;
    std::optional<std::vector<PieChartSeries>> series;
    std::optional<uint32_t> inner_size;

    // For Histograms
    std::optional<std::vector<HistogramBin>> bins;
};

// Expected tearsheet output
struct TearsheetExpect {
    std::string type = "tearsheet";
    std::optional<CardsList> cards;
    std::vector<TearsheetTable> tables;
    std::vector<TearsheetChart> charts;
};

// Expected dataframe output
struct DataFrameExpect {
    std::string type = "dataframe";
    std::map<std::string, ColumnData> columns;
};

// Test case structure
struct TestCase {
    std::string title;
    std::map<std::string, ColumnData> input;  // Direct map for input columns
    std::optional<std::variant<TearsheetExpect, DataFrameExpect>> expect;
    std::map<std::string, std::variant<bool, double, int64_t, std::string>> options;
    std::vector<std::string> timestamp_columns;
    std::optional<std::string> index_column;
};

// Root test file structure
struct TestFile {
    std::vector<TestCase> tests;
};

// Helper function to read file
inline std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// JSON Transform Tester class
class JsonTransformTester {
public:
    // Load tests from JSON file using structured approach
    static std::vector<TestCase> loadTestsFromJSON(const std::string& filePath) {
        std::string buffer = readFile(filePath);
        TestFile testFile;

        auto error = glz::read<glz::opts{.error_on_unknown_keys = false}>(testFile, buffer);
        if (error) {
            throw std::runtime_error("Failed to parse JSON from " + filePath + ": " +
                                   glz::format_error(error, buffer));
        }

        return testFile.tests;
    }

    // Load tests using dynamic json_t for more flexibility
    static std::vector<TestCase> loadTestsFromJSONDynamic(const std::string& filePath) {
        std::string buffer = readFile(filePath);
        glz::json_t json;

        auto error = glz::read_json(json, buffer);
        if (error) {
            throw std::runtime_error("Failed to parse JSON from " + filePath + ": " +
                                   glz::format_error(error, buffer));
        }

        std::vector<TestCase> tests;

        // Parse tests array
        if (json.contains("tests") && json["tests"].holds<glz::json_t::array_t>()) {
            for (auto& testJson : json["tests"].get<glz::json_t::array_t>()) {
                TestCase tc;

                // Parse basic fields
                if (testJson.contains("title")) {
                    tc.title = testJson["title"].get<std::string>();
                }

                // Parse input columns
                if (testJson.contains("input") && testJson["input"].holds<glz::json_t::object_t>()) {
                    for (auto& [key, value] : testJson["input"].get<glz::json_t::object_t>()) {
                        if (value.holds<glz::json_t::array_t>()) {
                            ColumnData columnData;
                            for (auto& v : value.get<glz::json_t::array_t>()) {
                                if (v.holds<double>()) {
                                    // Check if it's actually an integer
                                    double val = v.get<double>();
                                    if (std::floor(val) == val && std::abs(val) < 1e15) {
                                        columnData.push_back(static_cast<int64_t>(val));
                                    } else {
                                        columnData.push_back(val);
                                    }
                                } else if (v.holds<std::string>()) {
                                    columnData.push_back(v.get<std::string>());
                                } else if (v.holds<bool>()) {
                                    columnData.push_back(v.get<bool>());
                                } else if (v.holds<glz::json_t::null_t>()) {
                                    columnData.push_back(std::nullptr_t{});
                                }
                            }
                            tc.input[key] = columnData;
                        }
                    }
                }

                // Parse options
                if (testJson.contains("options") && testJson["options"].holds<glz::json_t::object_t>()) {
                    for (auto& [key, value] : testJson["options"].get<glz::json_t::object_t>()) {
                        if (value.holds<bool>()) {
                            tc.options[key] = value.get<bool>();
                        } else if (value.holds<double>()) {
                            // Check if it's actually an integer
                            double val = value.get<double>();
                            if (std::floor(val) == val && std::abs(val) < 1e15) {
                                tc.options[key] = static_cast<int64_t>(val);
                            } else {
                                tc.options[key] = val;
                            }
                        } else if (value.holds<std::string>()) {
                            tc.options[key] = value.get<std::string>();
                        }
                    }
                }

                // Parse timestamp_columns
                if (testJson.contains("timestamp_columns") && testJson["timestamp_columns"].holds<glz::json_t::array_t>()) {
                    for (auto& col : testJson["timestamp_columns"].get<glz::json_t::array_t>()) {
                        if (col.holds<std::string>()) {
                            tc.timestamp_columns.push_back(col.get<std::string>());
                        }
                    }
                }

                // Parse index_column
                if (testJson.contains("index_column") && testJson["index_column"].holds<std::string>()) {
                    tc.index_column = testJson["index_column"].get<std::string>();
                }

                // Parse expect field
                if (testJson.contains("expect") && testJson["expect"].holds<glz::json_t::object_t>()) {
                    auto& expectJson = testJson["expect"].get<glz::json_t::object_t>();

                    // Check the type field
                    if (expectJson.contains("type") && expectJson["type"].holds<std::string>()) {
                        std::string expectType = expectJson["type"].get<std::string>();

                        if (expectType == "tearsheet") {
                            TearsheetExpect tearsheet;
                            tearsheet.type = expectType;

                            // Parse cards
                            if (expectJson.contains("cards") && expectJson["cards"].holds<glz::json_t::object_t>()) {
                                auto& cardsJson = expectJson["cards"].get<glz::json_t::object_t>();
                                if (cardsJson.contains("cards") && cardsJson["cards"].holds<glz::json_t::array_t>()) {
                                    CardsList cardsList;
                                    for (auto& cardJson : cardsJson["cards"].get<glz::json_t::array_t>()) {
                                        if (cardJson.holds<glz::json_t::object_t>()) {
                                            auto& card = cardJson.get<glz::json_t::object_t>();
                                            TearsheetCard tearCard;

                                            if (card.contains("category") && card["category"].holds<std::string>()) {
                                                tearCard.category = card["category"].get<std::string>();
                                            }
                                            if (card.contains("group_size") && card["group_size"].holds<double>()) {
                                                tearCard.group_size = static_cast<int64_t>(card["group_size"].get<double>());
                                            }
                                            if (card.contains("type") && card["type"].holds<std::string>()) {
                                                tearCard.type = card["type"].get<std::string>();
                                            }

                                            // Parse card data
                                            if (card.contains("data") && card["data"].holds<glz::json_t::array_t>()) {
                                                for (auto& dataJson : card["data"].get<glz::json_t::array_t>()) {
                                                    if (dataJson.holds<glz::json_t::object_t>()) {
                                                        auto& data = dataJson.get<glz::json_t::object_t>();
                                                        CardData cardData;

                                                        if (data.contains("title") && data["title"].holds<std::string>()) {
                                                            cardData.title = data["title"].get<std::string>();
                                                        }
                                                        if (data.contains("group") && data["group"].holds<double>()) {
                                                            cardData.group = static_cast<int64_t>(data["group"].get<double>());
                                                        }
                                                        if (data.contains("type") && data["type"].holds<std::string>()) {
                                                            cardData.type = data["type"].get<std::string>();
                                                        }

                                                        // Parse value
                                                        if (data.contains("value")) {
                                                            if (data["value"].holds<double>()) {
                                                                double val = data["value"].get<double>();
                                                                // Keep as integer for TypeInteger, double for TypeDecimal
                                                                if (cardData.type == "TypeInteger" && std::floor(val) == val && std::abs(val) < 1e15) {
                                                                    cardData.value = static_cast<int64_t>(val);
                                                                } else {
                                                                    cardData.value = val;
                                                                }
                                                            } else if (data["value"].holds<std::string>()) {
                                                                cardData.value = data["value"].get<std::string>();
                                                            } else if (data["value"].holds<bool>()) {
                                                                cardData.value = data["value"].get<bool>();
                                                            } else {
                                                                cardData.value = std::nullptr_t{};
                                                            }
                                                        }

                                                        tearCard.data.push_back(cardData);
                                                    }
                                                }
                                            }

                                            cardsList.cards.push_back(tearCard);
                                        }
                                    }
                                    tearsheet.cards = cardsList;
                                }
                            }

                            // Parse tables
                            if (expectJson.contains("tables") && expectJson["tables"].holds<glz::json_t::array_t>()) {
                                for (auto& tableJson : expectJson["tables"].get<glz::json_t::array_t>()) {
                                    if (tableJson.holds<glz::json_t::object_t>()) {
                                        auto& tbl = tableJson.get<glz::json_t::object_t>();
                                        TearsheetTable table;

                                        if (tbl.contains("title") && tbl["title"].holds<std::string>()) {
                                            table.title = tbl["title"].get<std::string>();
                                        }
                                        if (tbl.contains("category") && tbl["category"].holds<std::string>()) {
                                            table.category = tbl["category"].get<std::string>();
                                        }
                                        if (tbl.contains("type") && tbl["type"].holds<std::string>()) {
                                            table.type = tbl["type"].get<std::string>();
                                        }

                                        // Parse columns
                                        if (tbl.contains("columns") && tbl["columns"].holds<glz::json_t::array_t>()) {
                                            for (auto& colJson : tbl["columns"].get<glz::json_t::array_t>()) {
                                                if (colJson.holds<glz::json_t::object_t>()) {
                                                    auto& col = colJson.get<glz::json_t::object_t>();
                                                    TableColumn column;

                                                    if (col.contains("name") && col["name"].holds<std::string>()) {
                                                        column.name = col["name"].get<std::string>();
                                                    }
                                                    if (col.contains("type") && col["type"].holds<std::string>()) {
                                                        column.type = col["type"].get<std::string>();
                                                    }

                                                    table.columns.push_back(column);
                                                }
                                            }
                                        }

                                        // Parse data rows
                                        if (tbl.contains("data") && tbl["data"].holds<glz::json_t::object_t>()) {
                                            auto& dataJson = tbl["data"].get<glz::json_t::object_t>();
                                            if (dataJson.contains("rows") && dataJson["rows"].holds<glz::json_t::array_t>()) {
                                                for (auto& rowJson : dataJson["rows"].get<glz::json_t::array_t>()) {
                                                    if (rowJson.holds<glz::json_t::array_t>()) {
                                                        std::vector<ColumnValue> row;
                                                        for (auto& cellJson : rowJson.get<glz::json_t::array_t>()) {
                                                            if (cellJson.holds<double>()) {
                                                                double val = cellJson.get<double>();
                                                                if (std::floor(val) == val && std::abs(val) < 1e15) {
                                                                    row.push_back(static_cast<int64_t>(val));
                                                                } else {
                                                                    row.push_back(val);
                                                                }
                                                            } else if (cellJson.holds<std::string>()) {
                                                                row.push_back(cellJson.get<std::string>());
                                                            } else if (cellJson.holds<bool>()) {
                                                                row.push_back(cellJson.get<bool>());
                                                            } else {
                                                                row.push_back(std::nullptr_t{});
                                                            }
                                                        }
                                                        table.data.rows.push_back(row);
                                                    }
                                                }
                                            }
                                        }

                                        tearsheet.tables.push_back(table);
                                    }
                                }
                            }

                            // Parse charts
                            if (expectJson.contains("charts") && expectJson["charts"].holds<glz::json_t::array_t>()) {
                                for (auto& chartJson : expectJson["charts"].get<glz::json_t::array_t>()) {
                                    if (chartJson.holds<glz::json_t::object_t>()) {
                                        auto& chart = chartJson.get<glz::json_t::object_t>();
                                        TearsheetChart tearChart;

                                        // Parse basic chart properties
                                        if (chart.contains("title") && chart["title"].holds<std::string>()) {
                                            tearChart.title = chart["title"].get<std::string>();
                                        }
                                        if (chart.contains("category") && chart["category"].holds<std::string>()) {
                                            tearChart.category = chart["category"].get<std::string>();
                                        }
                                        if (chart.contains("type") && chart["type"].holds<std::string>()) {
                                            tearChart.type = chart["type"].get<std::string>();
                                        }

                                        // Parse x_axis for line charts
                                        if (chart.contains("x_axis") && chart["x_axis"].holds<glz::json_t::object_t>()) {
                                            auto& xAxisJson = chart["x_axis"].get<glz::json_t::object_t>();
                                            ChartAxisData xAxisData;

                                            if (xAxisJson.contains("type") && xAxisJson["type"].holds<std::string>()) {
                                                xAxisData.type = xAxisJson["type"].get<std::string>();
                                            }

                                            if (xAxisJson.contains("data") && xAxisJson["data"].holds<glz::json_t::array_t>()) {
                                                for (auto& dataPoint : xAxisJson["data"].get<glz::json_t::array_t>()) {
                                                    if (dataPoint.holds<double>()) {
                                                        double val = dataPoint.get<double>();
                                                        if (std::floor(val) == val && std::abs(val) < 1e15) {
                                                            xAxisData.data.push_back(static_cast<int64_t>(val));
                                                        } else {
                                                            xAxisData.data.push_back(val);
                                                        }
                                                    } else if (dataPoint.holds<std::string>()) {
                                                        xAxisData.data.push_back(dataPoint.get<std::string>());
                                                    } else if (dataPoint.holds<bool>()) {
                                                        xAxisData.data.push_back(dataPoint.get<bool>());
                                                    } else {
                                                        xAxisData.data.push_back(std::nullptr_t{});
                                                    }
                                                }
                                            }

                                            tearChart.x_axis = xAxisData;
                                        }

                                        // Parse lines for line charts
                                        if (chart.contains("lines") && chart["lines"].holds<glz::json_t::array_t>()) {
                                            std::vector<ChartLineData> linesData;
                                            for (auto& lineJson : chart["lines"].get<glz::json_t::array_t>()) {
                                                if (lineJson.holds<glz::json_t::object_t>()) {
                                                    auto& line = lineJson.get<glz::json_t::object_t>();
                                                    ChartLineData lineData;

                                                    if (line.contains("name") && line["name"].holds<std::string>()) {
                                                        lineData.name = line["name"].get<std::string>();
                                                    }
                                                    if (line.contains("type") && line["type"].holds<std::string>()) {
                                                        lineData.type = line["type"].get<std::string>();
                                                    }

                                                    if (line.contains("data") && line["data"].holds<glz::json_t::array_t>()) {
                                                        for (auto& dataPoint : line["data"].get<glz::json_t::array_t>()) {
                                                            if (dataPoint.holds<double>()) {
                                                                double val = dataPoint.get<double>();
                                                                if (std::floor(val) == val && std::abs(val) < 1e15) {
                                                                    lineData.data.push_back(static_cast<int64_t>(val));
                                                                } else {
                                                                    lineData.data.push_back(val);
                                                                }
                                                            } else if (dataPoint.holds<std::string>()) {
                                                                lineData.data.push_back(dataPoint.get<std::string>());
                                                            } else if (dataPoint.holds<bool>()) {
                                                                lineData.data.push_back(dataPoint.get<bool>());
                                                            } else {
                                                                lineData.data.push_back(std::nullptr_t{});
                                                            }
                                                        }
                                                    }

                                                    linesData.push_back(lineData);
                                                }
                                            }
                                            tearChart.lines = linesData;
                                        }

                                        // Parse slices for pie charts
                                        if (chart.contains("slices") && chart["slices"].holds<glz::json_t::array_t>()) {
                                            std::vector<PieChartSlice> slicesData;
                                            for (auto& sliceJson : chart["slices"].get<glz::json_t::array_t>()) {
                                                if (sliceJson.holds<glz::json_t::object_t>()) {
                                                    auto& slice = sliceJson.get<glz::json_t::object_t>();
                                                    PieChartSlice sliceData;

                                                    if (slice.contains("label") && slice["label"].holds<std::string>()) {
                                                        sliceData.label = slice["label"].get<std::string>();
                                                    }

                                                    if (slice.contains("value")) {
                                                        auto& val = slice["value"];
                                                        if (val.holds<double>()) {
                                                            double dval = val.get<double>();
                                                            // Convert to int64 if it's a whole number
                                                            if (std::floor(dval) == dval && std::abs(dval) < 1e15) {
                                                                sliceData.value = static_cast<int64_t>(dval);
                                                            } else {
                                                                sliceData.value = dval;
                                                            }
                                                        }
                                                    }

                                                    slicesData.push_back(sliceData);
                                                }
                                            }
                                            tearChart.slices = slicesData;
                                        }

                                        // Parse series for nested pie charts
                                        if (chart.contains("series") && chart["series"].holds<glz::json_t::array_t>()) {
                                            std::vector<PieChartSeries> seriesData;
                                            for (auto& seriesJson : chart["series"].get<glz::json_t::array_t>()) {
                                                if (seriesJson.holds<glz::json_t::object_t>()) {
                                                    auto& seriesObj = seriesJson.get<glz::json_t::object_t>();
                                                    PieChartSeries series;

                                                    if (seriesObj.contains("name") && seriesObj["name"].holds<std::string>()) {
                                                        series.name = seriesObj["name"].get<std::string>();
                                                    }
                                                    if (seriesObj.contains("size")) {
                                                        auto& sizeVal = seriesObj["size"];
                                                        if (sizeVal.holds<double>()) {
                                                            series.size = static_cast<uint32_t>(sizeVal.get<double>());
                                                        }
                                                    }
                                                    if (seriesObj.contains("inner_size")) {
                                                        auto& innerVal = seriesObj["inner_size"];
                                                        if (innerVal.holds<double>()) {
                                                            series.inner_size = static_cast<uint32_t>(innerVal.get<double>());
                                                        }
                                                    }

                                                    if (seriesObj.contains("data") && seriesObj["data"].holds<glz::json_t::array_t>()) {
                                                        for (auto& dataPoint : seriesObj["data"].get<glz::json_t::array_t>()) {
                                                            if (dataPoint.holds<glz::json_t::object_t>()) {
                                                                auto& point = dataPoint.get<glz::json_t::object_t>();
                                                                PieChartSlice pointData;

                                                                if (point.contains("name") && point["name"].holds<std::string>()) {
                                                                    pointData.label = point["name"].get<std::string>();
                                                                }

                                                                if (point.contains("y")) {
                                                                    auto& yVal = point["y"];
                                                                    if (yVal.holds<double>()) {
                                                                        pointData.value = yVal.get<double>();
                                                                    }
                                                                }

                                                                series.data.push_back(pointData);
                                                            }
                                                        }
                                                    }

                                                    seriesData.push_back(series);
                                                }
                                            }
                                            tearChart.series = seriesData;
                                        }

                                        // Parse bars for bar charts
                                        if (chart.contains("bars") && chart["bars"].holds<glz::json_t::array_t>()) {
                                            std::vector<BarCategory> barsData;
                                            for (auto& barJson : chart["bars"].get<glz::json_t::array_t>()) {
                                                if (barJson.holds<glz::json_t::object_t>()) {
                                                    auto& bar = barJson.get<glz::json_t::object_t>();
                                                    BarCategory barData;

                                                    if (bar.contains("name") && bar["name"].holds<std::string>()) {
                                                        barData.name = bar["name"].get<std::string>();
                                                    }

                                                    // Handle single value format
                                                    if (bar.contains("value")) {
                                                        auto& val = bar["value"];
                                                        if (val.holds<double>()) {
                                                            barData.value = val.get<double>();
                                                        }
                                                    }

                                                    // Handle data array format (for multi-series charts)
                                                    if (bar.contains("data") && bar["data"].holds<glz::json_t::array_t>()) {
                                                        for (auto& dataVal : bar["data"].get<glz::json_t::array_t>()) {
                                                            if (dataVal.holds<double>()) {
                                                                barData.data.push_back(dataVal.get<double>());
                                                            }
                                                        }
                                                    }

                                                    barsData.push_back(barData);
                                                }
                                            }
                                            tearChart.bars = barsData;
                                        }

                                        // Parse vertical and stacked for bar charts
                                        if (chart.contains("vertical")) {
                                            auto& vertVal = chart["vertical"];
                                            if (vertVal.holds<bool>()) {
                                                tearChart.vertical = vertVal.get<bool>();
                                                std::cout << "DEBUG JSON PARSE: Setting vertical=" << vertVal.get<bool>() << std::endl;
                                            }
                                        }
                                        if (chart.contains("stacked")) {
                                            auto& stackedVal = chart["stacked"];
                                            if (stackedVal.holds<bool>()) {
                                                tearChart.stacked = stackedVal.get<bool>();
                                                std::cout << "DEBUG JSON PARSE: Setting stacked=" << stackedVal.get<bool>() << std::endl;
                                            }
                                        }

                                        // Parse bins for histogram charts
                                        if (chart.contains("bins") && chart["bins"].holds<glz::json_t::array_t>()) {
                                            std::vector<HistogramBin> binsData;
                                            for (auto& binJson : chart["bins"].get<glz::json_t::array_t>()) {
                                                if (binJson.holds<glz::json_t::object_t>()) {
                                                    auto& bin = binJson.get<glz::json_t::object_t>();
                                                    HistogramBin binData;

                                                    if (bin.contains("min")) {
                                                        auto& minVal = bin["min"];
                                                        if (minVal.holds<double>()) {
                                                            binData.min = minVal.get<double>();
                                                        }
                                                    }
                                                    if (bin.contains("max")) {
                                                        auto& maxVal = bin["max"];
                                                        if (maxVal.holds<double>()) {
                                                            binData.max = maxVal.get<double>();
                                                        }
                                                    }
                                                    if (bin.contains("count")) {
                                                        auto& countVal = bin["count"];
                                                        if (countVal.holds<double>()) {
                                                            binData.count = static_cast<int64_t>(countVal.get<double>());
                                                        }
                                                    }

                                                    binsData.push_back(binData);
                                                }
                                            }
                                            tearChart.bins = binsData;
                                        }

                                        tearsheet.charts.push_back(tearChart);
                                    }
                                }
                            }

                            tc.expect = tearsheet;

                        } else if (expectType == "dataframe") {
                            DataFrameExpect df;
                            df.type = expectType;

                            // Parse columns
                            if (expectJson.contains("columns") && expectJson["columns"].holds<glz::json_t::object_t>()) {
                                for (auto& [colName, colData] : expectJson["columns"].get<glz::json_t::object_t>()) {
                                    if (colData.holds<glz::json_t::array_t>()) {
                                        ColumnData column;
                                        for (auto& v : colData.get<glz::json_t::array_t>()) {
                                            if (v.holds<double>()) {
                                                double val = v.get<double>();
                                                if (std::floor(val) == val && std::abs(val) < 1e15) {
                                                    column.push_back(static_cast<int64_t>(val));
                                                } else {
                                                    column.push_back(val);
                                                }
                                            } else if (v.holds<std::string>()) {
                                                column.push_back(v.get<std::string>());
                                            } else if (v.holds<bool>()) {
                                                column.push_back(v.get<bool>());
                                            } else {
                                                column.push_back(std::nullptr_t{});
                                            }
                                        }
                                        df.columns[colName] = column;
                                    }
                                }
                            }

                            tc.expect = df;
                        }
                    }
                }

                tests.push_back(std::move(tc));
            }
        }

        return tests;
    }
};

} // namespace json
} // namespace test
} // namespace epoch

// With latest glaze, no meta specializations needed for structs with primitive types!
// Glaze will automatically serialize/deserialize these structures.

#endif // JSON_TRANSFORM_TESTER_HPP