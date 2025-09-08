//
// Created by adesola on 12/3/24.
//
#include "epoch_metadata/strategy/registration.h"
#include "epoch_metadata/bar_attribute.h"
#include "transforms/src/config_helper.h"
#include "transforms/src/operators/equality.h"
#include "transforms/src/operators/logical.h"
#include "transforms/src/operators/select.h"
#include "epoch_metadata/transforms/transform_definition.h"
#include "epoch_metadata/transforms/transform_registry.h"
#include "epoch_metadata/transforms/itransform.h"
#include "epoch_metadata/transforms/transform_configuration.h"
#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/index_factory.h>

using namespace epoch_core;
using namespace epoch_metadata;
using namespace epoch_metadata::transform;
using namespace std::chrono_literals;
using namespace epoch_frame;

/**
 * Helper Functions to Build DataFrames
 */
epoch_frame::DataFrame MakeNumericDataFrame() {
  auto index = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

  return make_dataframe<double>(
      index,
      {
          {10.0, 20.0, 30.0, 40.0}, // price
          {1.0, 2.0, 3.0, 4.0},     // actual
          {1.0, 0.0, 3.0, 5.0},     // expected
          {5.0, 10.0, 15.0, 20.0},  // current
          {3.0, 10.0, 20.0, 15.0}   // previous
      },
      {"price", "actual", "expected", "current", "previous"});
}

epoch_frame::DataFrame MakeBoolDataFrame() {
  auto index = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

  return make_dataframe<bool>(index,
                              {
                                  {true, false, true, false}, // bool_a
                                  {false, false, true, true}  // bool_b
                              },
                              {"bool_a", "bool_b"});
}

epoch_frame::DataFrame MakeSelectDataFrame2() {
  auto index = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

  return make_dataframe(
      index,
      {
          {0_scalar, 1_scalar, 0_scalar, 1_scalar},                // selector
          {10.0_scalar, 20.0_scalar, 30.0_scalar, 40.0_scalar},    // option_0
          {100.0_scalar, 200.0_scalar, 300.0_scalar, 400.0_scalar} // option_1
      },
      {arrow::field("selector", arrow::int64()),
       arrow::field("option_0", arrow::float64()),
       arrow::field("option_1", arrow::float64())});
}

TEST_CASE("Comparative Transforms") {

  SECTION("Equality Transforms - Comprehensive") {
    epoch_frame::DataFrame input = MakeNumericDataFrame();

    // Vector-based comparisons
    SECTION("Vector Equals (vector_eq)") {
      TransformConfiguration config =
          vector_op("eq", 7, "actual", "expected",
                    epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame output = transform->TransformData(input);
      epoch_frame::DataFrame expected = make_dataframe<bool>(
          input.index(), {{true, false, true, false}}, {"7#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }

    SECTION("Vector Not Equals (vector_neq)") {
      TransformConfiguration config =
          vector_op("neq", 8, "actual", "expected",
                    epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame output = transform->TransformData(input);
      epoch_frame::DataFrame expected = make_dataframe<bool>(
          input.index(), {{false, true, false, true}}, {"8#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }

    SECTION("Vector Less Than (vector_lt)") {
      TransformConfiguration config =
          vector_op("lt", 9, "previous", "current",
                    epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame output = transform->TransformData(input);
      epoch_frame::DataFrame expected = make_dataframe<bool>(
          input.index(), {{true, false, false, true}}, {"9#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }

    SECTION("Vector Less Than Equals (vector_lte)") {
      TransformConfiguration config =
          vector_op("lte", 10, "previous", "current",
                    epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame output = transform->TransformData(input);
      epoch_frame::DataFrame expected = make_dataframe<bool>(
          input.index(), {{true, true, false, true}}, {"10#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }
  }

  SECTION("Logical Transforms - Comprehensive") {
    epoch_frame::DataFrame input = MakeBoolDataFrame();

    SECTION("Logical OR (logical_or)") {
      TransformConfiguration config =
          logical_op("or", 11, "bool_a", "bool_b",
                     epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame output = transform->TransformData(input);
      epoch_frame::DataFrame expected = make_dataframe<bool>(
          input.index(), {{true, false, true, true}}, {"11#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }

    SECTION("Logical AND (logical_and)") {
      TransformConfiguration config =
          logical_op("and", 12, "bool_a", "bool_b",
                     epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame output = transform->TransformData(input);
      epoch_frame::DataFrame expected = make_dataframe<bool>(
          input.index(), {{false, false, true, false}}, {"12#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }

    SECTION("Logical NOT (logical_not)") {
      TransformConfiguration config = single_operand_op(
          "logical", "not", 13, "bool_a",
          epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame output = transform->TransformData(input);
      epoch_frame::DataFrame expected = make_dataframe<bool>(
          input.index(), {{false, true, false, true}}, {"13#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }

    SECTION("Logical XOR (logical_xor)") {
      TransformConfiguration config =
          logical_op("xor", 14, "bool_a", "bool_b",
                     epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame output = transform->TransformData(input);
      epoch_frame::DataFrame expected = make_dataframe<bool>(
          input.index(), {{true, false, false, true}}, {"14#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }

    SECTION("Logical AND NOT (logical_and_not)") {
      TransformConfiguration config =
          logical_op("and_not", 15, "bool_a", "bool_b",
                     epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame output = transform->TransformData(input);
      epoch_frame::DataFrame expected = make_dataframe<bool>(
          input.index(), {{true, false, false, false}}, {"15#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }
  }

  SECTION("Select Transforms - Comprehensive") {
    SECTION("BooleanSelectTransform") {
      auto index = epoch_frame::factory::index::make_datetime_index(
          {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
           epoch_frame::DateTime{2020y, std::chrono::January, 2d},
           epoch_frame::DateTime{2020y, std::chrono::January, 3d},
           epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

      epoch_frame::DataFrame input =
          make_dataframe(index,
                         {
                             {Scalar{true}, Scalar{false}, Scalar{true},
                              Scalar{false}}, // condition
                             {100.0_scalar, 200.0_scalar, 300.0_scalar,
                              400.0_scalar}, // value_if_true
                             {10.0_scalar, 20.0_scalar, 30.0_scalar,
                              40.0_scalar} // value_if_false
                         },
                         {arrow::field("condition", arrow::boolean()),
                          arrow::field("value_if_true", arrow::float64()),
                          arrow::field("value_if_false", arrow::float64())});

      TransformConfiguration config =
          boolean_select(20, "condition", "value_if_true", "value_if_false",
                         epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame output = transform->TransformData(input);
      epoch_frame::DataFrame expected = make_dataframe<double>(
          input.index(), {{100.0, 20.0, 300.0, 40.0}}, {"20#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }

    SECTION("Select2 Transform") {
      TransformConfiguration config =
          select_n(21, 2, "selector", {"option_0", "option_1"},
                   epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame input = MakeSelectDataFrame2();
      epoch_frame::DataFrame output = transform->TransformData(input);

      epoch_frame::DataFrame expected = make_dataframe<double>(
          input.index(), {{10.0, 200.0, 30.0, 400.0}}, {"21#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }

    SECTION("Select3 Transform") {
      auto index = epoch_frame::factory::index::make_datetime_index(
          {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
           epoch_frame::DateTime{2020y, std::chrono::January, 2d},
           epoch_frame::DateTime{2020y, std::chrono::January, 3d},
           epoch_frame::DateTime{2020y, std::chrono::January, 4d},
           epoch_frame::DateTime{2020y, std::chrono::January, 5d}});

      epoch_frame::DataFrame input = make_dataframe(
          index,
          {
              {0_scalar, 1_scalar, 2_scalar, 1_scalar, 0_scalar}, // selector
              {10.0_scalar, 20.0_scalar, 30.0_scalar, 40.0_scalar,
               50.0_scalar}, // option_0
              {100.0_scalar, 200.0_scalar, 300.0_scalar, 400.0_scalar,
               500.0_scalar}, // option_1
              {1000.0_scalar, 2000.0_scalar, 3000.0_scalar, 4000.0_scalar,
               5000.0_scalar} // option_2
          },
          {arrow::field("selector", arrow::int64()),
           arrow::field("option_0", arrow::float64()),
           arrow::field("option_1", arrow::float64()),
           arrow::field("option_2", arrow::float64())});

      TransformConfiguration config =
          select_n(22, 3, "selector", {"option_0", "option_1", "option_2"},
                   epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      epoch_frame::DataFrame output = transform->TransformData(input);

      epoch_frame::DataFrame expected = make_dataframe<double>(
          input.index(), {{10.0, 200.0, 3000.0, 400.0, 50.0}}, {"22#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }

    SECTION("Select4 Transform (normal usage)") {
      auto index = epoch_frame::factory::index::make_datetime_index(
          {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
           epoch_frame::DateTime{2020y, std::chrono::January, 2d},
           epoch_frame::DateTime{2020y, std::chrono::January, 3d},
           epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

      epoch_frame::DataFrame input = make_dataframe(
          index,
          {
              {0_scalar, 1_scalar, 2_scalar, 3_scalar},             // selector
              {10.0_scalar, 20.0_scalar, 30.0_scalar, 40.0_scalar}, // option_0
              {100.0_scalar, 200.0_scalar, 300.0_scalar,
               400.0_scalar}, // option_1
              {1000.0_scalar, 2000.0_scalar, 3000.0_scalar,
               4000.0_scalar},                                     // option_2
              {-1.0_scalar, -2.0_scalar, -3.0_scalar, -4.0_scalar} // option_3
          },
          {arrow::field("selector", arrow::int64()),
           arrow::field("option_0", arrow::float64()),
           arrow::field("option_1", arrow::float64()),
           arrow::field("option_2", arrow::float64()),
           arrow::field("option_3", arrow::float64())});

      TransformConfiguration config = select_n(
          23, 4, "selector", {"option_0", "option_1", "option_2", "option_3"},
          epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      // row0 => idx=0 => 10
      // row1 => idx=1 => 200
      // row2 => idx=2 => 3000
      // row3 => idx=3 => -4
      epoch_frame::DataFrame output = transform->TransformData(input);

      // row0 => idx=0 => 10
      // row1 => idx=1 => 200
      // row2 => idx=2 => 3000
      // row3 => idx=3 => -4
      epoch_frame::DataFrame expected = make_dataframe<double>(
          input.index(), {{10.0, 200.0, 3000.0, -4.0}}, {"23#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }

    SECTION("Select5 Transform (normal usage)") {
      auto index = epoch_frame::factory::index::make_datetime_index(
          {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
           epoch_frame::DateTime{2020y, std::chrono::January, 2d},
           epoch_frame::DateTime{2020y, std::chrono::January, 3d},
           epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

      epoch_frame::DataFrame input = make_dataframe(
          index,
          {
              {0_scalar, 1_scalar, 4_scalar, 3_scalar},             // selector
              {10.0_scalar, 20.0_scalar, 30.0_scalar, 40.0_scalar}, // option_0
              {100.0_scalar, 200.0_scalar, 300.0_scalar,
               400.0_scalar}, // option_1
              {1000.0_scalar, 2000.0_scalar, 3000.0_scalar,
               4000.0_scalar},                                      // option_2
              {-1.0_scalar, -2.0_scalar, -3.0_scalar, -4.0_scalar}, // option_3
              {999.0_scalar, 888.0_scalar, 777.0_scalar, 666.0_scalar}
              // option_4
          },
          {arrow::field("selector", arrow::int64()),
           arrow::field("option_0", arrow::float64()),
           arrow::field("option_1", arrow::float64()),
           arrow::field("option_2", arrow::float64()),
           arrow::field("option_3", arrow::float64()),
           arrow::field("option_4", arrow::float64())});

      TransformConfiguration config =
          select_n(24, 5, "selector",
                   {"option_0", "option_1", "option_2", "option_3", "option_4"},
                   epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
      auto transformBase = MAKE_TRANSFORM(config);
      auto transform = dynamic_cast<ITransform *>(transformBase.get());

      // row0 => idx=0 => 10
      // row1 => idx=1 => 200
      // row2 => idx=4 => 777
      // row3 => idx=3 => -4
      epoch_frame::DataFrame output = transform->TransformData(input);

      epoch_frame::DataFrame expected = make_dataframe<double>(
          input.index(), {{10.0, 200.0, 777.0, -4.0}}, {"24#result"});

      INFO("Comparing output with expected values\n"
           << output << "\n!=\n"
           << expected);
      REQUIRE(output.equals(expected));
    }
  }
}
TEST_CASE("Additional Comparative Transforms") {
  SECTION("Vector Greater Than (vector_gt)") {
    epoch_frame::DataFrame input = MakeNumericDataFrame();

    TransformConfiguration config =
        vector_op("gt", 25, "current", "previous",
                  epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    epoch_frame::DataFrame output = transform->TransformData(input);
    epoch_frame::DataFrame expected = make_dataframe<bool>(
        input.index(), {{true, false, false, true}}, {"25#result"});

    INFO("Comparing output with expected values\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("Vector Greater Than Equals (vector_gte)") {
    epoch_frame::DataFrame input = MakeNumericDataFrame();

    TransformConfiguration config =
        vector_op("gte", 26, "current", "previous",
                  epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    epoch_frame::DataFrame output = transform->TransformData(input);
    epoch_frame::DataFrame expected = make_dataframe<bool>(
        input.index(), {{true, true, false, true}}, {"26#result"});

    INFO("Comparing output with expected values\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("PercentileSelect") {
    auto index = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
         epoch_frame::DateTime{2020y, std::chrono::January, 2d},
         epoch_frame::DateTime{2020y, std::chrono::January, 3d},
         epoch_frame::DateTime{2020y, std::chrono::January, 4d},
         epoch_frame::DateTime{2020y, std::chrono::January, 5d},
         epoch_frame::DateTime{2020y, std::chrono::January, 6d}});

    epoch_frame::DataFrame input =
        make_dataframe(index,
                       {
                           {10.0_scalar, 15.0_scalar, 8.0_scalar, 20.0_scalar,
                            12.0_scalar, 25.0_scalar}, // value
                           {100.0_scalar, 150.0_scalar, 80.0_scalar,
                            200.0_scalar, 120.0_scalar, 250.0_scalar}, // high
                           {1.0_scalar, 1.5_scalar, 0.8_scalar, 2.0_scalar,
                            1.2_scalar, 2.5_scalar} // low
                       },
                       {arrow::field("value", arrow::float64()),
                        arrow::field("high", arrow::float64()),
                        arrow::field("low", arrow::float64())});

    // Use the helper function instead of direct YAML
    TransformConfiguration config =
        percentile_select("30", "value", "high", "low", 3, 50,
                          epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    epoch_frame::DataFrame output = transform->TransformData(input);

    // For window size 3, median (50th percentile) in each window:
    // Row 0-2: [10, 15, 8] -> median = 10, value >= median ? high : low
    // 10 >= 10 -> 100
    // 15 >= 10 -> 150
    // 8 >= 10 -> 0.8 (low)

    // Row 3-5: [20, 12, 25] -> median = 20, value >= median ? high : low
    // 20 >= 20 -> 200
    // 12 >= 20 -> 1.2 (low)
    // 25 >= 20 -> 250

    epoch_frame::DataFrame expected = make_dataframe<double>(
        input.index(), {{std::nan(""), std::nan(""), 0.8, 200.0, 120, 250.0}},
        {"30#result"});

    INFO("Comparing output with expected values\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("BooleanBranch") {
    auto index = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
         epoch_frame::DateTime{2020y, std::chrono::January, 2d},
         epoch_frame::DateTime{2020y, std::chrono::January, 3d},
         epoch_frame::DateTime{2020y, std::chrono::January, 4d}});

    epoch_frame::DataFrame input = make_dataframe<bool>(
        index, {{true, false, true, false}}, {"condition"});

    // Use the helper function instead of direct YAML
    TransformConfiguration config = boolean_branch(
        "31", "condition", epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    epoch_frame::DataFrame output = transform->TransformData(input);

    epoch_frame::DataFrame expected = make_dataframe<bool>(
        input.index(),
        {
            {true, false, true, false}, // true branch preserves condition
            {false, true, false, true}  // false branch is the negation
        },
        {"31#true", "31#false"});

    INFO("Comparing output with expected values\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }

  SECTION("RatioBranch") {
    auto index = epoch_frame::factory::index::make_datetime_index(
        {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
         epoch_frame::DateTime{2020y, std::chrono::January, 2d},
         epoch_frame::DateTime{2020y, std::chrono::January, 3d},
         epoch_frame::DateTime{2020y, std::chrono::January, 4d},
         epoch_frame::DateTime{2020y, std::chrono::January, 5d}});

    epoch_frame::DataFrame input =
        make_dataframe<double>(index, {{0.5, 1.2, 1.5, 0.8, 2.0}}, {"ratio"});

    // Use the helper function instead of direct YAML
    TransformConfiguration config =
        ratio_branch("32", "ratio", 1.5, 0.8,
                     epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    epoch_frame::DataFrame output = transform->TransformData(input);

    // threshold_high = 1.5, threshold_low = 0.8
    // high: ratio > 1.5
    // normal: 0.8 <= ratio <= 1.5
    // low: ratio < 0.8

    epoch_frame::DataFrame expected =
        make_dataframe(input.index(),
                       {
                           {Scalar{false}, Scalar{false}, Scalar{false},
                            Scalar{false}, Scalar{true}}, // high branch
                           {Scalar{false}, Scalar{true}, Scalar{true},
                            Scalar{true}, Scalar{false}}, // normal branch
                           {Scalar{true}, Scalar{false}, Scalar{false},
                            Scalar{false}, Scalar{false}}, // low branch
                       },
                       {arrow::field("32#high", arrow::boolean()),
                        arrow::field("32#normal", arrow::boolean()),
                        arrow::field("32#low", arrow::boolean())});

    INFO("Comparing output with expected values\n"
         << output << "\n!=\n"
         << expected);
    REQUIRE(output.equals(expected));
  }
}

TEST_CASE("Value Comparison Operators", "[value_compare]") {
  // Create test data for different value compare scenarios
  auto timeIndex = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d},
       epoch_frame::DateTime{2020y, std::chrono::January, 5d},
       epoch_frame::DateTime{2020y, std::chrono::January, 6d}});

  auto previousIndex = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2020y, std::chrono::January, 1d},
       epoch_frame::DateTime{2020y, std::chrono::January, 2d},
       epoch_frame::DateTime{2020y, std::chrono::January, 3d},
       epoch_frame::DateTime{2020y, std::chrono::January, 4d},
       epoch_frame::DateTime{2020y, std::chrono::January, 5d}});

  auto previousData = make_dataframe<double>(
      previousIndex, {{10.0, 15.0, 12.0, 20.0, 18.0}}, {"price"});

  auto highestData = make_dataframe<double>(
      timeIndex, {{10.0, 15.0, 12.0, 20.0, 18.0, 25.0}}, {"price"});

  auto lowestData = make_dataframe<double>(
      timeIndex, {{10.0, 15.0, 8.0, 20.0, 12.0, 25.0}}, {"price"});

  SECTION("Previous Value Comparisons") {
    // Simple tests for Previous comparisons - these work correctly
    struct TestCase {
      std::string name;
      std::function<TransformConfiguration(std::string, std::string, int,
                                           const epoch_metadata::TimeFrame &)>
          configFn;
      std::vector<std::optional<bool>> expectedResults;
    };

    const std::vector<TestCase> testCases = {
        {"GreaterThan", previous_gt, {std::nullopt, true, false, true, false}},
        {"GreaterThanOrEqual",
         previous_gte,
         {std::nullopt, true, false, true, false}},
        {"LessThan", previous_lt, {std::nullopt, false, true, false, true}},
        {"LessThanOrEqual",
         previous_lte,
         {std::nullopt, false, true, false, true}},
        {"Equals", previous_eq, {std::nullopt, false, false, false, false}},
        {"NotEquals", previous_neq, {std::nullopt, true, true, true, true}}};

    for (const auto &test : testCases) {
      SECTION(test.name) {
        TransformConfiguration config =
            test.configFn("test_id", "price", 1,
                          epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

        auto transformBase = MAKE_TRANSFORM(config);
        auto transform = dynamic_cast<ITransform *>(transformBase.get());

        epoch_frame::DataFrame output = transform->TransformData(previousData);

        std::vector<Scalar> expectedScalars;
        for (const auto &val : test.expectedResults) {
          if (val.has_value()) {
            expectedScalars.push_back(Scalar{val.value()});
          } else {
            expectedScalars.push_back(Scalar{});
          }
        }

        epoch_frame::DataFrame expected =
            make_dataframe(previousData.index(), {expectedScalars},
                           {"test_id#result"}, arrow::boolean());

        INFO("Previous " << test.name
                         << ": Comparing output with expected values\n"
                         << output << "\n!=\n"
                         << expected);
        REQUIRE(output.equals(expected));
      }
    }
  }

  SECTION("Highest Value Comparisons") {
    // Tests for Highest comparisons - fixing expectations based on
    // understanding the implementation
    const int lookback = 3;

    // For highestData = {10.0, 15.0, 12.0, 20.0, 18.0, 25.0}
    // Rolling max with window=3:
    // Index 0, 1: null (insufficient data)
    // Index 2: max of [10.0, 15.0, 12.0] = 15.0
    // Index 3: max of [15.0, 12.0, 20.0] = 20.0
    // Index 4: max of [12.0, 20.0, 18.0] = 20.0
    // Index 5: max of [20.0, 18.0, 25.0] = 25.0

    struct TestCase {
      std::string name;
      std::function<TransformConfiguration(std::string, std::string, int,
                                           const epoch_metadata::TimeFrame &)>
          configFn;
      std::vector<std::optional<bool>> expectedResults;
    };

    // Adjusted expected values based on the above calculation
    const std::vector<TestCase> highestTestCases = {
        {"GreaterThan",
         highest_gt,
         {std::nullopt, std::nullopt, false, false, false, false}},
        {"GreaterThanOrEqual",
         highest_gte,
         {std::nullopt, std::nullopt, false, true, false, true}},
        {"LessThan",
         highest_lt,
         {std::nullopt, std::nullopt, true, false, true, false}},
        {"LessThanOrEqual",
         highest_lte,
         {std::nullopt, std::nullopt, true, true, true, true}},
        {"Equals",
         highest_eq,
         {std::nullopt, std::nullopt, false, true, false, true}},
        {"NotEquals",
         highest_neq,
         {std::nullopt, std::nullopt, true, false, true, false}}};

    for (const auto &test : highestTestCases) {
      SECTION(test.name) {
        TransformConfiguration config =
            test.configFn("test_id", "price", lookback,
                          epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

        auto transformBase = MAKE_TRANSFORM(config);
        auto transform = dynamic_cast<ITransform *>(transformBase.get());

        epoch_frame::DataFrame output = transform->TransformData(highestData);

        std::vector<Scalar> expectedScalars;
        for (const auto &val : test.expectedResults) {
          if (val.has_value()) {
            expectedScalars.push_back(Scalar{val.value()});
          } else {
            expectedScalars.push_back(Scalar{});
          }
        }

        epoch_frame::DataFrame expected =
            make_dataframe(highestData.index(), {expectedScalars},
                           {"test_id#result"}, arrow::boolean());

        INFO("Highest " << test.name
                        << ": Comparing output with expected values\n"
                        << output << "\n!=\n"
                        << expected);
        REQUIRE(output.equals(expected));
      }
    }
  }

  SECTION("Lowest Value Comparisons") {
    // Tests for Lowest comparisons - fixing expectations based on understanding
    // the implementation
    const int lookback = 3;

    // For lowestData = {10.0, 15.0, 8.0, 20.0, 12.0, 25.0}
    // Rolling min with window=3:
    // Index 0, 1: null (insufficient data)
    // Index 2: min of [10.0, 15.0, 8.0] = 8.0
    // Index 3: min of [15.0, 8.0, 20.0] = 8.0
    // Index 4: min of [8.0, 20.0, 12.0] = 8.0
    // Index 5: min of [20.0, 12.0, 25.0] = 12.0

    struct TestCase {
      std::string name;
      std::function<TransformConfiguration(std::string, std::string, int,
                                           const epoch_metadata::TimeFrame &)>
          configFn;
      std::vector<std::optional<bool>> expectedResults;
    };

    // Adjusted expected values based on the above calculation
    const std::vector<TestCase> lowestTestCases = {
        {"GreaterThan",
         lowest_gt,
         {std::nullopt, std::nullopt, false, true, true, true}},
        {"GreaterThanOrEqual",
         lowest_gte,
         {std::nullopt, std::nullopt, true, true, true, true}},
        {"LessThan",
         lowest_lt,
         {std::nullopt, std::nullopt, false, false, false, false}},
        {"LessThanOrEqual",
         lowest_lte,
         {std::nullopt, std::nullopt, true, false, false, false}},
        {"Equals",
         lowest_eq,
         {std::nullopt, std::nullopt, true, false, false, false}},
        {"NotEquals",
         lowest_neq,
         {std::nullopt, std::nullopt, false, true, true, true}}};

    for (const auto &test : lowestTestCases) {
      SECTION(test.name) {
        TransformConfiguration config =
            test.configFn("test_id", "price", lookback,
                          epoch_metadata::EpochStratifyXConstants::instance().DAILY_FREQUENCY);

        auto transformBase = MAKE_TRANSFORM(config);
        auto transform = dynamic_cast<ITransform *>(transformBase.get());

        epoch_frame::DataFrame output = transform->TransformData(lowestData);

        std::vector<Scalar> expectedScalars;
        for (const auto &val : test.expectedResults) {
          if (val.has_value()) {
            expectedScalars.push_back(Scalar{val.value()});
          } else {
            expectedScalars.push_back(Scalar{});
          }
        }

        epoch_frame::DataFrame expected =
            make_dataframe(lowestData.index(), {expectedScalars},
                           {"test_id#result"}, arrow::boolean());

        INFO("Lowest " << test.name
                       << ": Comparing output with expected values\n"
                       << output << "\n!=\n"
                       << expected);
        REQUIRE(output.equals(expected));
      }
    }
  }
}
