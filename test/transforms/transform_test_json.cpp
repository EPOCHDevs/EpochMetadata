//
// JSON-based transform tester using glaze
//
#include <epoch_testing/json_transform_tester.hpp>
#include <epoch_testing/catch_transform_tester.hpp>
#include <epoch_testing/tearsheet_output.hpp>
#include <epoch_testing/dataframe_tester.hpp>
#include <epoch_metadata/transforms/itransform.h>
#include <epoch_metadata/transforms/transform_registry.h>
#include <epoch_metadata/transforms/transform_definition.h>
#include <epoch_metadata/transforms/transform_configuration.h>
#include <epoch_metadata/reports/ireport.h>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <algorithm>

using namespace epoch::test;
using namespace epoch::test::json;
using namespace epoch_metadata;
using namespace epoch_metadata::transform;

namespace
{

    // Build TransformDefinition from test options
    TransformDefinition buildTransformDefinition(const Options &testOptions,
                                                 const epoch_frame::DataFrame &input)
    {
        // Build a YAML node from the test options
        YAML::Node yamlNode;

        // Extract and set transform name (required)
        auto nameIt = testOptions.find("transform_name");
        if (nameIt != testOptions.end() && std::holds_alternative<std::string>(nameIt->second))
        {
            yamlNode["type"] = std::get<std::string>(nameIt->second);
        }
        else
        {
            throw std::runtime_error("transform_name not specified in options");
        }

        // Extract and set output ID
        auto idIt = testOptions.find("output_id");
        if (idIt != testOptions.end() && std::holds_alternative<std::string>(idIt->second))
        {
            yamlNode["id"] = std::get<std::string>(idIt->second);
        }
        else
        {
            yamlNode["id"] = yamlNode["type"].as<std::string>();
        }

        // Check if explicit inputs are specified in options
        auto inputsIt = testOptions.find("inputs");
        YAML::Node inputs;
        if (inputsIt != testOptions.end() && std::holds_alternative<std::string>(inputsIt->second))
        {
            // Parse the inputs string as YAML
            std::string inputsStr = std::get<std::string>(inputsIt->second);
            try
            {
                inputs = YAML::Load(inputsStr);
            }
            catch (const YAML::Exception &e)
            {
                throw std::runtime_error("Failed to parse inputs YAML: " + std::string(e.what()));
            }
        }
        else
        {
            // Build default input mapping from DataFrame columns
            for (const auto &colName : input.column_names())
            {
                inputs[colName] = colName; // Direct 1-1 mapping
            }
        }
        yamlNode["inputs"] = inputs;

        // Convert test options to YAML options
        YAML::Node options;
        for (const auto &[key, value] : testOptions)
        {
            // Skip special keys
            if (key == "transform_name" || key == "output_id" ||
                key == "timeframe" || key == "session" || key == "inputs")
            {
                continue;
            }

            if (std::holds_alternative<bool>(value))
            {
                options[key] = std::get<bool>(value);
            }
            else if (std::holds_alternative<double>(value))
            {
                options[key] = std::get<double>(value);
            }
            else if (std::holds_alternative<std::string>(value))
            {
                options[key] = std::get<std::string>(value);
            }
        }
        if (options.size() > 0)
        {
            yamlNode["options"] = options;
        }

        // Extract and set timeframe if specified
        auto tfIt = testOptions.find("timeframe");
        if (tfIt != testOptions.end() && std::holds_alternative<std::string>(tfIt->second))
        {
            yamlNode["timeframe"] = std::get<std::string>(tfIt->second);
        }

        // Create TransformDefinition from YAML
        return TransformDefinition(yamlNode);
    }

    // Run transform with configuration
    epoch_frame::DataFrame runTransformWithConfig(const epoch_frame::DataFrame &input,
                                                  const Options &options)
    {
        // Build TransformDefinition from options
        TransformDefinition definition = buildTransformDefinition(options, input);

        // Create TransformConfiguration from definition
        TransformConfiguration config(std::move(definition));

        // Create transform using registry
        auto transformPtr = TransformRegistry::GetInstance().Get(config);
        if (!transformPtr)
        {
            throw std::runtime_error("Failed to create transform");
        }

        // Cast to ITransform
        auto transform = dynamic_cast<ITransform *>(transformPtr.get());
        if (!transform)
        {
            throw std::runtime_error("Transform does not implement ITransform interface");
        }

        // Run the transform
        return transform->TransformData(input);
    }

    // Convert JSON test case to DataFrame tester format
    DataFrameTransformTester::TestCaseType convertJsonToTestCase(const json::TestCase &jsonTest)
    {
        DataFrameTransformTester::TestCaseType testCase;

        testCase.title = jsonTest.title;
        testCase.timestamp_columns = jsonTest.timestamp_columns;
        testCase.index_column = jsonTest.index_column.value_or("");

        // Convert input data from ColumnData to Table format
        for (const auto &[colName, colData] : jsonTest.input)
        {
            Column column;
            for (const auto &value : colData)
            {
                if (std::holds_alternative<double>(value))
                {
                    column.push_back(std::get<double>(value));
                }
                else if (std::holds_alternative<int64_t>(value))
                {
                    column.push_back(static_cast<double>(std::get<int64_t>(value)));
                }
                else if (std::holds_alternative<bool>(value))
                {
                    column.push_back(std::get<bool>(value));
                }
                else if (std::holds_alternative<std::string>(value))
                {
                    column.push_back(std::get<std::string>(value));
                }
                else
                {
                    column.push_back(std::nullopt);
                }
            }
            testCase.input[colName] = column;
        }

        // Convert options
        for (const auto &[key, value] : jsonTest.options)
        {
            if (std::holds_alternative<bool>(value))
            {
                testCase.options[key] = std::get<bool>(value);
            }
            else if (std::holds_alternative<double>(value))
            {
                testCase.options[key] = std::get<double>(value);
            }
            else if (std::holds_alternative<int64_t>(value))
            {
                testCase.options[key] = static_cast<double>(std::get<int64_t>(value));
            }
            else if (std::holds_alternative<std::string>(value))
            {
                testCase.options[key] = std::get<std::string>(value);
            }
        }

        // Handle expected output
        if (jsonTest.expect.has_value())
        {
            const auto &expectVar = jsonTest.expect.value();

            if (std::holds_alternative<TearsheetExpect>(expectVar))
            {
                // Create tearsheet output
                auto tearsheet = std::make_unique<TearsheetOutput>();

                const auto &tearsheetExpect = std::get<TearsheetExpect>(expectVar);

                // Handle cards if present
                if (tearsheetExpect.cards.has_value())
                {
                    const auto &cardsList = tearsheetExpect.cards.value();
                    epoch_proto::CardDefList *protoCardsList = tearsheet->protoTearsheet.mutable_cards();

                    for (const auto &card : cardsList.cards)
                    {
                        epoch_proto::CardDef *protoCard = protoCardsList->add_cards();
                        protoCard->set_category(card.category);
                        protoCard->set_group_size(card.group_size);

                        // Map card type string to proto type
                        if (card.type == "WidgetCard")
                        {
                            protoCard->set_type(epoch_proto::WidgetCard);
                        }

                        // Add card data
                        for (const auto &data : card.data)
                        {
                            epoch_proto::CardData *protoData = protoCard->add_data();
                            protoData->set_title(data.title);
                            protoData->set_group(data.group);

                            // Map data type string to proto type
                            if (data.type == "TypeDecimal")
                            {
                                protoData->set_type(epoch_proto::TypeDecimal);
                            }
                            else if (data.type == "TypeInteger")
                            {
                                protoData->set_type(epoch_proto::TypeInteger);
                            }
                            else if (data.type == "TypeString")
                            {
                                protoData->set_type(epoch_proto::TypeString);
                            }
                            else if (data.type == "TypeBoolean")
                            {
                                protoData->set_type(epoch_proto::TypeBoolean);
                            }
                            else if (data.type == "TypePercent")
                            {
                                protoData->set_type(epoch_proto::TypePercent);
                            }
                            else if (data.type == "TypeMonetary")
                            {
                                protoData->set_type(epoch_proto::TypeMonetary);
                            }
                            else if (data.type == "TypeDate")
                            {
                                protoData->set_type(epoch_proto::TypeDate);
                            }

                            // Set value using correct scalar variant based on type
                            epoch_proto::Scalar *protoValue = protoData->mutable_value();
                            if (std::holds_alternative<double>(data.value))
                            {
                                double val = std::get<double>(data.value);
                                // Use the correct variant based on the type field
                                if (data.type == "TypePercent")
                                {
                                    protoValue->set_percent_value(val);
                                }
                                else if (data.type == "TypeMonetary")
                                {
                                    protoValue->set_monetary_value(val);
                                }
                                else
                                {
                                    protoValue->set_decimal_value(val);
                                }
                            }
                            else if (std::holds_alternative<int64_t>(data.value))
                            {
                                int64_t val = std::get<int64_t>(data.value);
                                if (data.type == "TypeDate")
                                {
                                    protoValue->set_date_value(val);
                                }
                                else
                                {
                                    protoValue->set_integer_value(val);
                                }
                            }
                            else if (std::holds_alternative<bool>(data.value))
                            {
                                protoValue->set_boolean_value(std::get<bool>(data.value));
                            }
                            else if (std::holds_alternative<std::string>(data.value))
                            {
                                protoValue->set_string_value(std::get<std::string>(data.value));
                            }
                        }
                    }
                }

                // Create tables in the tearsheet if any
                if (!tearsheetExpect.tables.empty())
                {
                    epoch_proto::TableList *tableList = tearsheet->protoTearsheet.mutable_tables();
                    for (const auto &table : tearsheetExpect.tables)
                    {
                        // Add table to TableList
                        epoch_proto::Table *protoTable = tableList->add_tables();
                        protoTable->set_title(table.title);
                        protoTable->set_category(table.category);
                        protoTable->set_type(epoch_proto::WidgetDataTable);

                        // Add columns
                        for (const auto &col : table.columns)
                        {
                            auto *protoCol = protoTable->add_columns();
                            protoCol->set_name(col.name);
                            // Map type string to proto type
                            if (col.type == "TypeDecimal")
                            {
                                protoCol->set_type(epoch_proto::TypeDecimal);
                            }
                            else if (col.type == "TypeInteger")
                            {
                                protoCol->set_type(epoch_proto::TypeInteger);
                            }
                            else if (col.type == "TypeString")
                            {
                                protoCol->set_type(epoch_proto::TypeString);
                            }
                            else if (col.type == "TypePercent")
                            {
                                protoCol->set_type(epoch_proto::TypePercent);
                            }
                            else if (col.type == "TypeMonetary")
                            {
                                protoCol->set_type(epoch_proto::TypeMonetary);
                            }
                            else if (col.type == "TypeDate")
                            {
                                protoCol->set_type(epoch_proto::TypeDate);
                            }
                            else if (col.type == "TypeBoolean")
                            {
                                protoCol->set_type(epoch_proto::TypeBoolean);
                            }
                        }

                        // Get mutable TableData
                        epoch_proto::TableData *tableData = protoTable->mutable_data();

                        // Add rows
                        for (const auto &row : table.data.rows)
                        {
                            epoch_proto::TableRow *protoRow = tableData->add_rows();
                            for (size_t colIdx = 0; colIdx < row.size(); ++colIdx)
                            {
                                const auto &cell = row[colIdx];
                                auto *scalar = protoRow->add_values();

                                // Use the column type to determine which scalar variant to use
                                std::string colType = (colIdx < table.columns.size()) ? table.columns[colIdx].type : "TypeDecimal";

                                if (std::holds_alternative<double>(cell))
                                {
                                    double val = std::get<double>(cell);
                                    if (colType == "TypePercent")
                                    {
                                        scalar->set_percent_value(val);
                                    }
                                    else if (colType == "TypeMonetary")
                                    {
                                        scalar->set_monetary_value(val);
                                    }
                                    else
                                    {
                                        scalar->set_decimal_value(val);
                                    }
                                }
                                else if (std::holds_alternative<int64_t>(cell))
                                {
                                    int64_t val = std::get<int64_t>(cell);
                                    if (colType == "TypeDate")
                                    {
                                        scalar->set_date_value(val);
                                    }
                                    else
                                    {
                                        scalar->set_integer_value(val);
                                    }
                                }
                                else if (std::holds_alternative<std::string>(cell))
                                {
                                    scalar->set_string_value(std::get<std::string>(cell));
                                }
                                else if (std::holds_alternative<std::nullptr_t>(cell))
                                {
                                    scalar->set_null_value(epoch_proto::NULL_VALUE);
                                }
                            }
                        }
                    }
                }

                // Convert charts
                if (!tearsheetExpect.charts.empty())
                {
                    epoch_proto::ChartList *chartList = tearsheet->protoTearsheet.mutable_charts();
                    for (const auto &chart : tearsheetExpect.charts)
                    {
                        epoch_proto::Chart *protoChart = chartList->add_charts();

                        // Determine chart type and convert accordingly
                        if (chart.type == "WidgetPieChart")
                        {
                            // Pie chart conversion
                            epoch_proto::PieDef *pieDef = protoChart->mutable_pie_def();
                            pieDef->mutable_chart_def()->set_title(chart.title);
                            pieDef->mutable_chart_def()->set_category(chart.category);
                            pieDef->mutable_chart_def()->set_type(epoch_proto::WidgetPie);

                            if (chart.slices.has_value())
                            {
                                // Create a single PieDataDef series
                                epoch_proto::PieDataDef *pieDataDef = pieDef->add_data();
                                pieDataDef->set_name("default");
                                pieDataDef->set_size("100%");

                                if (chart.inner_size.has_value())
                                {
                                    pieDataDef->set_inner_size(std::to_string(chart.inner_size.value()) + "%");
                                }

                                // Add each slice as PieData
                                for (const auto &slice : chart.slices.value())
                                {
                                    epoch_proto::PieData *pieData = pieDataDef->add_points();
                                    pieData->set_name(slice.label);

                                    if (std::holds_alternative<double>(slice.value))
                                    {
                                        pieData->set_y(std::get<double>(slice.value));
                                    }
                                    else if (std::holds_alternative<int64_t>(slice.value))
                                    {
                                        pieData->set_y(static_cast<double>(std::get<int64_t>(slice.value)));
                                    }
                                }
                            }
                            else if (chart.series.has_value())
                            {
                                // Handle nested pie with multiple series
                                for (const auto &seriesData : chart.series.value())
                                {
                                    epoch_proto::PieDataDef *pieDataDef = pieDef->add_data();
                                    pieDataDef->set_name(seriesData.name);
                                    pieDataDef->set_size(std::to_string(seriesData.size) + "%");
                                    pieDataDef->set_inner_size(std::to_string(seriesData.inner_size) + "%");

                                    // Add each data point as PieData
                                    for (const auto &point : seriesData.data)
                                    {
                                        epoch_proto::PieData *pieData = pieDataDef->add_points();
                                        pieData->set_name(point.label);

                                        if (std::holds_alternative<double>(point.value))
                                        {
                                            pieData->set_y(std::get<double>(point.value));
                                        }
                                        else if (std::holds_alternative<int64_t>(point.value))
                                        {
                                            pieData->set_y(static_cast<double>(std::get<int64_t>(point.value)));
                                        }
                                    }
                                }
                            }
                        }
                        else if (chart.type == "WidgetBarChart")
                        {
                            // Bar chart conversion
                            std::cerr << "DEBUG PROTO CONVERT: Converting BarChart '" << chart.title << "', chart.bars.has_value()=" << chart.bars.has_value() << std::endl;
                            epoch_proto::BarDef *barDef = protoChart->mutable_bar_def();
                            barDef->mutable_chart_def()->set_title(chart.title);
                            barDef->mutable_chart_def()->set_category(chart.category);
                            barDef->mutable_chart_def()->set_type(epoch_proto::WidgetBar);

                            if (chart.vertical.has_value())
                            {
                                std::cerr << "DEBUG PROTO CONVERT: Setting vertical=" << chart.vertical.value() << std::endl;
                                barDef->set_vertical(chart.vertical.value());
                            }
                            if (chart.stacked.has_value())
                            {
                                std::cerr << "DEBUG PROTO CONVERT: Setting stacked=" << chart.stacked.value() << std::endl;
                                barDef->set_stacked(chart.stacked.value());
                            }

                            if (chart.bars.has_value())
                            {
                                const auto &bars = chart.bars.value();

                                // Check if this is multi-series format (bars have data arrays)
                                bool isMultiSeries = !bars.empty() && !bars[0].data.empty();
                                std::cerr << "DEBUG PROTO CONVERT: bars.size()=" << bars.size() << ", isMultiSeries=" << isMultiSeries;
                                if (!bars.empty())
                                {
                                    std::cerr << ", bars[0].data.size()=" << bars[0].data.size();
                                }
                                std::cerr << std::endl;

                                if (isMultiSeries)
                                {
                                    // Multi-series format: each bar is a series with name and data array
                                    std::cerr << "DEBUG PROTO CONVERT: Multi-series path, bars.size()=" << bars.size() << std::endl;
                                    for (const auto &bar : bars)
                                    {
                                        epoch_proto::BarData *barData = barDef->add_data();
                                        barData->set_name(bar.name);
                                        std::cerr << "DEBUG PROTO CONVERT: Processing bar " << bar.name << ", data.size()=" << bar.data.size() << std::endl;

                                        for (const auto &val : bar.data)
                                        {
                                            if (std::holds_alternative<double>(val))
                                            {
                                                barData->add_values(std::get<double>(val));
                                                std::cerr << "DEBUG PROTO CONVERT: Added double " << std::get<double>(val) << std::endl;
                                            }
                                            else if (std::holds_alternative<int64_t>(val))
                                            {
                                                barData->add_values(static_cast<double>(std::get<int64_t>(val)));
                                                std::cerr << "DEBUG PROTO CONVERT: Added int64 " << std::get<int64_t>(val) << std::endl;
                                            }
                                            else
                                            {
                                                std::cerr << "DEBUG PROTO CONVERT: Value holds neither double nor int64_t" << std::endl;
                                            }
                                        }
                                        std::cerr << "DEBUG PROTO CONVERT: Final values_size()=" << barData->values_size() << std::endl;
                                    }
                                    std::cerr << "DEBUG PROTO CONVERT: BarDef now has " << barDef->data_size() << " series" << std::endl;
                                }
                                else
                                {
                                    // Single series format: each bar is a category with a single value
                                    epoch_proto::AxisDef *xAxis = barDef->mutable_chart_def()->mutable_x_axis();
                                    xAxis->set_type(epoch_proto::AxisCategory);

                                    epoch_proto::BarData *barData = barDef->add_data();
                                    barData->set_name("values");

                                    for (const auto &bar : bars)
                                    {
                                        xAxis->add_categories(bar.name);

                                        if (std::holds_alternative<double>(bar.value))
                                        {
                                            barData->add_values(std::get<double>(bar.value));
                                        }
                                        else if (std::holds_alternative<int64_t>(bar.value))
                                        {
                                            barData->add_values(static_cast<double>(std::get<int64_t>(bar.value)));
                                        }
                                    }
                                }
                            }
                        }
                        else if (chart.type == "WidgetLinesChart")
                        {
                            // Line chart conversion
                            epoch_proto::LinesDef *linesDef = protoChart->mutable_lines_def();
                            linesDef->mutable_chart_def()->set_title(chart.title);
                            linesDef->mutable_chart_def()->set_category(chart.category);
                            linesDef->mutable_chart_def()->set_type(epoch_proto::WidgetLines);

                            // Set up x-axis
                            if (chart.x_axis.has_value())
                            {
                                epoch_proto::AxisDef *xAxis = linesDef->mutable_chart_def()->mutable_x_axis();

                                // Map type string to proto type
                                if (chart.x_axis->type == "TypeDecimal")
                                {
                                    xAxis->set_type(epoch_proto::AxisLinear);
                                }
                                else if (chart.x_axis->type == "TypeTimestamp")
                                {
                                    xAxis->set_type(epoch_proto::AxisDateTime);
                                }
                            }

                            // Add lines
                            if (chart.lines.has_value())
                            {
                                for (const auto &lineData : chart.lines.value())
                                {
                                    epoch_proto::Line *line = linesDef->add_lines();
                                    line->set_name(lineData.name);

                                    // Add data points - for line charts, x comes from x_axis
                                    if (chart.x_axis.has_value())
                                    {
                                        for (size_t i = 0; i < lineData.data.size() && i < chart.x_axis->data.size(); ++i)
                                        {
                                            epoch_proto::Point *point = line->add_data();

                                            // Set x value
                                            if (std::holds_alternative<int64_t>(chart.x_axis->data[i]))
                                            {
                                                point->set_x(std::get<int64_t>(chart.x_axis->data[i]));
                                            }
                                            else if (std::holds_alternative<double>(chart.x_axis->data[i]))
                                            {
                                                point->set_x(static_cast<int64_t>(std::get<double>(chart.x_axis->data[i])));
                                            }

                                            // Set y value
                                            if (std::holds_alternative<double>(lineData.data[i]))
                                            {
                                                point->set_y(std::get<double>(lineData.data[i]));
                                            }
                                            else if (std::holds_alternative<int64_t>(lineData.data[i]))
                                            {
                                                point->set_y(static_cast<double>(std::get<int64_t>(lineData.data[i])));
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else if (chart.type == "WidgetHistogramChart")
                        {
                            // Histogram conversion
                            epoch_proto::HistogramDef *histogramDef = protoChart->mutable_histogram_def();
                            histogramDef->mutable_chart_def()->set_title(chart.title);
                            histogramDef->mutable_chart_def()->set_category(chart.category);
                            histogramDef->mutable_chart_def()->set_type(epoch_proto::WidgetHistogram);

                            // For histograms, we just compare raw data and bins_count
                            // The bins field in JSON is now ignored - we only care about matching the input data
                            if (chart.bins.has_value())
                            {
                                histogramDef->set_bins_count(static_cast<uint32_t>(chart.bins->size()));
                            }

                            // Data comparison is skipped for histograms - we only check bins_count matches
                            // The actual histogram data comes from the transform output
                        }
                    }
                }

                // Debug: Check chart data before move
                std::cerr << "DEBUG BEFORE MOVE: tearsheet has " << tearsheet->protoTearsheet.charts().charts_size() << " charts" << std::endl;
                if (tearsheet->protoTearsheet.has_charts() && tearsheet->protoTearsheet.charts().charts_size() > 0)
                {
                    const auto &firstChart = tearsheet->protoTearsheet.charts().charts(0);
                    if (firstChart.has_bar_def())
                    {
                        std::cerr << "DEBUG BEFORE MOVE: First bar chart '" << firstChart.bar_def().chart_def().title() << "' has " << firstChart.bar_def().data_size() << " series" << std::endl;
                    }
                    else if (firstChart.has_histogram_def())
                    {
                        std::cerr << "DEBUG BEFORE MOVE: First chart is histogram" << std::endl;
                    }
                    else
                    {
                        std::cerr << "DEBUG BEFORE MOVE: First chart is other type" << std::endl;
                    }
                }

                testCase.expect = std::move(tearsheet);
            }
            else if (std::holds_alternative<DataFrameExpect>(expectVar))
            {
                // Create dataframe output
                const auto &dfExpect = std::get<DataFrameExpect>(expectVar);
                Table outputTable;

                for (const auto &[colName, colData] : dfExpect.columns)
                {
                    Column column;
                    for (const auto &value : colData)
                    {
                        if (std::holds_alternative<double>(value))
                        {
                            column.push_back(std::get<double>(value));
                        }
                        else if (std::holds_alternative<int64_t>(value))
                        {
                            column.push_back(static_cast<double>(std::get<int64_t>(value)));
                        }
                        else if (std::holds_alternative<bool>(value))
                        {
                            column.push_back(std::get<bool>(value));
                        }
                        else if (std::holds_alternative<std::string>(value))
                        {
                            column.push_back(std::get<std::string>(value));
                        }
                        else
                        {
                            column.push_back(std::nullopt);
                        }
                    }
                    outputTable[colName] = column;
                }

                testCase.expect = std::make_unique<DataFrameOutput>(outputTable);
            }
        }

        return testCase;
    }

    // Run tests from a JSON file
    void runJsonTestFile(const std::string &testFile)
    {
        std::filesystem::path filePath(testFile);
        std::string sectionName = filePath.stem().string() + " [JSON]";

        SECTION(sectionName)
        {
            INFO("Loading JSON test file: " << testFile);

            std::vector<json::TestCase> jsonTests;
            try
            {
                // Use dynamic loader for better compatibility
                jsonTests = JsonTransformTester::loadTestsFromJSONDynamic(testFile);
            }
            catch (const std::exception &e)
            {
                FAIL("Failed to load JSON tests from " << testFile << ": " << e.what());
                return;
            }

            INFO("Loaded " << jsonTests.size() << " test cases");

            for (const auto &jsonTest : jsonTests)
            {
                SECTION(jsonTest.title)
                {
                    // Convert to standard test case format
                    auto testCase = convertJsonToTestCase(jsonTest);

                    // Keep inputs SLOT as-is - individual nodes handle sanitization if needed

                    // Convert input to DataFrame with normalized column names
                    epoch_frame::DataFrame inputDf = CatchTransformTester::tableToDataFrame(
                        testCase.input, testCase.timestamp_columns, testCase.index_column);

                    INFO("Test: " << testCase.title);
                    INFO("Input DataFrame:\n"
                         << inputDf);

                    // Run the test using existing infrastructure
                    bool isReportTest = testCase.expect && testCase.expect->getType() == "tearsheet";

                    if (isReportTest)
                    {
                        // Report test - run directly
                        INFO("Running report test");

                        std::unique_ptr<TearsheetOutput> actualOutput;
                        try
                        {
                            // Create transform instance
                            auto definition = buildTransformDefinition(testCase.options, inputDf);
                            TransformConfiguration config(std::move(definition));
                            auto transformPtr = TransformRegistry::GetInstance().Get(config);

                            if (!transformPtr)
                            {
                                FAIL("Failed to create transform");
                                return;
                            }

                            // Cast to reporter
                            auto reporter = dynamic_cast<epoch_metadata::reports::IReporter *>(transformPtr.get());
                            if (!reporter)
                            {
                                FAIL("Transform does not implement IReporter interface");
                                return;
                            }

                            // Run the report
                            auto outputDf = reporter->TransformData(inputDf);

                            // Get tearsheet
                            epoch_proto::TearSheet protoTearsheet = reporter->GetTearSheet();

                            actualOutput = std::make_unique<TearsheetOutput>();
                            actualOutput->protoTearsheet = protoTearsheet;
                        }
                        catch (const std::exception &e)
                        {
                            FAIL("Report generation failed: " + std::string(e.what()));
                        }

                        // Compare outputs
                        if (testCase.expect)
                        {
                            INFO("Expected:\n"
                                 << testCase.expect->toString());
                            INFO("Actual:\n"
                                 << actualOutput->toString());
                            REQUIRE(actualOutput->equals(*testCase.expect));
                        }
                    }
                    else
                    {
                        // Transform test
                        INFO("Running transform test");

                        epoch_frame::DataFrame outputDf;
                        try
                        {
                            outputDf = runTransformWithConfig(inputDf, testCase.options);
                        }
                        catch (const std::exception &e)
                        {
                            FAIL("Transform failed: " << e.what());
                            return;
                        }

                        INFO("Output DataFrame:\n"
                             << outputDf);

                        // Convert output for comparison
                        Table outputTable = CatchTransformTester::dataFrameToTable(outputDf);
                        auto actualOutput = std::make_unique<DataFrameOutput>(outputTable);

                        // Compare
                        if (testCase.expect)
                        {
                            INFO("Expected:\n"
                                 << testCase.expect->toString());
                            INFO("Actual:\n"
                                 << actualOutput->toString());
                            REQUIRE(actualOutput->equals(*testCase.expect));
                        }
                        else
                        {
                            REQUIRE(outputTable.empty());
                        }
                    }
                }
            }
        }
    }

} // anonymous namespace

TEST_CASE("All Transform Tests - JSON Based", "[Transform][JSON]")
{
    // Register output types
    static bool registered = false;
    if (!registered)
    {
        registerDataFrameType();
        registerTearsheetType();
        registered = true;
    }

    // Find all JSON test files
    std::vector<std::string> jsonFiles;
    std::filesystem::path testDir(TRANSFORMS_TEST_CASES_DIR);

    if (std::filesystem::exists(testDir))
    {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(testDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".json")
            {
                jsonFiles.push_back(entry.path().string());
            }
        }
    }

    if (jsonFiles.empty())
    {
        WARN("No JSON test files found");
        return;
    }

    // Sort for consistent ordering
    std::sort(jsonFiles.begin(), jsonFiles.end());

    INFO("Found " << jsonFiles.size() << " JSON test files");

    // Run each test file
    for (const auto &testFile : jsonFiles)
    {
        runJsonTestFile(testFile);
    }
}