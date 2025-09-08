//
// Created by adesola on 12/3/24.
//
#include "epoch_metadata/strategy/registration.h"
#include "epoch_metadata/bar_attribute.h"
#include "transforms/src/config_helper.h"
#include "transforms/src/cummulative/cum_op.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include "epoch_metadata/transforms/itransform.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_core;
using namespace epoch_metadata;
using namespace epoch_metadata::transform;
using namespace std::chrono_literals;
using namespace epoch_frame;

TEST_CASE("Cummulative Transforms") {
  SECTION("CumProdOperation") {
    auto index = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
         epoch_frame::DateTime{2020y, std::chrono::January, 2d},
         epoch_frame::DateTime{2020y, std::chrono::January, 3d},
         epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

    // Shared Setup: Define an input DataFrame with a numeric column
    epoch_frame::DataFrame input =
        make_dataframe<double>(index, {{1.0, 2.0, 3.0, 4.0}}, {"input_column"});

    TransformConfiguration config =
        cum_prod("20", "input_column",
                 epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    // Use registry to create the transform
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<CumProdOperation *>(transformBase.get());

    // Expected Output: Cumulative product [1.0, 2.0, 6.0, 24.0]
    epoch_frame::DataFrame expected = make_dataframe<double>(
        index, {{1.0, 2.0, 6.0, 24.0}}, {config.GetOutputId()});

    // Apply transform
    epoch_frame::DataFrame output = transform->TransformData(input);

    // Verify output
    INFO("Comparing output with expected values\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }
}