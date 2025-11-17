#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_script/core/bar_attribute.h>
#include <epoch_script/core/constants.h>
#include "epoch_script/strategy/registration.h"
#include <epoch_frame/dataframe.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_script/transforms/core/config_helper.h>
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/transforms/core/transform_configuration.h>
#include <epoch_script/transforms/core/transform_registry.h>
#include <arrow/api.h>

using namespace epoch_core;
using namespace epoch_frame;
using namespace epoch_script;
using namespace epoch_script::transform;
using namespace std::chrono_literals;

TEST_CASE("BooleanSelect typed transforms", "[transforms][operators][boolean_select][type]") {
    const auto &timeframe = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

    SECTION("boolean_select_string with string values produces string output") {
        // Create test data
        auto index = factory::index::make_datetime_index({
            DateTime{2020y, std::chrono::January, 1d},
            DateTime{2020y, std::chrono::January, 2d},
            DateTime{2020y, std::chrono::January, 3d}
        });

        // Create arrays for each column
        auto condition_array = factory::array::make_array<bool>({true, false, true});
        auto true_val_array = factory::array::make_array<std::string>({"ValuePick", "ValuePick", "ValuePick"});
        auto false_val_array = factory::array::make_array<std::string>({"Other", "Other", "Other"});

        // Combine into input dataframe
        std::vector<arrow::ChunkedArrayPtr> arrays = {condition_array, true_val_array, false_val_array};
        std::vector<std::string> column_names = {"condition", "true_val", "false_val"};
        auto input_df = make_dataframe(index, arrays, column_names);

        // Use typed variant directly
        TransformConfiguration config{TransformDefinition{YAML::Load(std::format(
            R"(
type: boolean_select_string
id: 1
inputs:
  "condition": "condition"
  "true": "true_val"
  "false": "false_val"
timeframe: {}
)",
            timeframe.Serialize()))}};
        auto transformBase = MAKE_TRANSFORM(config);
        auto transform = dynamic_cast<transform::ITransform*>(transformBase.get());

        // Run transform
        DataFrame result_df = transform->TransformData(input_df);

        // Check output type
        auto result_series = result_df[config.GetOutputId()];
        auto result_type = result_series.dtype();

        INFO("Result type: " << result_type->ToString());

        // Output should be STRING
        REQUIRE(result_type->id() == arrow::Type::STRING);

        // Check values: should be ["ValuePick", "Other", "ValuePick"]
        auto expected_array = factory::array::make_array<std::string>({"ValuePick", "Other", "ValuePick"});
        std::vector<arrow::ChunkedArrayPtr> expected_arrays = {expected_array};
        std::vector<std::string> expected_names = {std::string(config.GetOutputId())};
        auto expected = make_dataframe(index, expected_arrays, expected_names);

        INFO("Comparing boolean_select output\n" << result_df << "\n!=\n" << expected);
        REQUIRE(result_df.equals(expected));
    }

    SECTION("boolean_select_number with numeric values produces numeric output") {
        auto index = factory::index::make_datetime_index({
            DateTime{2020y, std::chrono::January, 1d},
            DateTime{2020y, std::chrono::January, 2d}
        });

        auto condition_array = factory::array::make_array<bool>({true, false});
        auto true_val_array = factory::array::make_array<double>({1.0, 1.0});
        auto false_val_array = factory::array::make_array<double>({0.0, 0.0});

        std::vector<arrow::ChunkedArrayPtr> arrays = {condition_array, true_val_array, false_val_array};
        std::vector<std::string> column_names = {"condition", "true_val", "false_val"};
        auto input_df = make_dataframe(index, arrays, column_names);

        // Use typed variant directly
        TransformConfiguration config{TransformDefinition{YAML::Load(std::format(
            R"(
type: boolean_select_number
id: 2
inputs:
  "condition": "condition"
  "true": "true_val"
  "false": "false_val"
timeframe: {}
)",
            timeframe.Serialize()))}};
        auto transformBase = MAKE_TRANSFORM(config);
        auto transform = dynamic_cast<transform::ITransform*>(transformBase.get());

        DataFrame result_df = transform->TransformData(input_df);
        auto result_type = result_df[config.GetOutputId()].dtype();

        INFO("Result type: " << result_type->ToString());

        // Output should be DOUBLE
        REQUIRE(result_type->id() == arrow::Type::DOUBLE);
    }
}
