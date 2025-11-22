//
// Unit tests for SageMaker FinBERT Sentiment Analysis Transform
//

#include <epoch_script/core/bar_attribute.h>
#include <epoch_script/core/constants.h>
#include <epoch_script/transforms/core/config_helper.h>
#include <epoch_script/transforms/core/itransform.h>
#include <epoch_script/transforms/core/transform_configuration.h>
#include <epoch_script/transforms/core/transform_registry.h>
#include <catch2/catch_test_macros.hpp>
#include <epoch_core/catch_defs.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>

using namespace epoch_core;
using namespace epoch_script;
using namespace epoch_script::transform;
using namespace std::chrono_literals;
using namespace epoch_frame;

// Helper function to create test dataframe with financial text
DataFrame createFinancialTextDataFrame() {
  auto index = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2024y, std::chrono::January, 1d},
       epoch_frame::DateTime{2024y, std::chrono::January, 2d},
       epoch_frame::DateTime{2024y, std::chrono::January, 3d},
       epoch_frame::DateTime{2024y, std::chrono::January, 4d},
       epoch_frame::DateTime{2024y, std::chrono::January, 5d}});

  return make_dataframe<std::string>(
      index,
      {{"The company reported record profits this quarter with 25% growth",
        "Stock prices are falling due to market uncertainty and recession fears",
        "The quarterly earnings met analyst expectations",
        "Major layoffs announced as company struggles with declining revenue",
        "New product launch expected to boost sales significantly"}},
      {"text"});
}

TEST_CASE("FinBERT Sentiment Transform - Configuration", "[ml][finbert][config]") {
  const auto tf = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  SECTION("Basic configuration") {
    auto config = finbert_sentiment_cfg("test_finbert", "text", tf);

    REQUIRE(config.GetTransformName() == "finbert_sentiment");
    REQUIRE(config.GetId() == "test_finbert");

    // Verify transform can be instantiated
    auto transformBase = MAKE_TRANSFORM(config);
    REQUIRE(transformBase != nullptr);

    // Verify it's the correct transform type
    auto transform = dynamic_cast<ITransform *>(transformBase.get());
    REQUIRE(transform != nullptr);
  }

  SECTION("Metadata validation") {
    auto &registry = transforms::ITransformRegistry::GetInstance();
    REQUIRE(registry.IsValid("finbert_sentiment"));

    auto metadataOpt = registry.GetMetaData("finbert_sentiment");
    REQUIRE(metadataOpt.has_value());

    auto metadata = metadataOpt.value().get();
    REQUIRE(metadata.id == "finbert_sentiment");
    REQUIRE(metadata.name == "FinBERT Sentiment Analysis");
    REQUIRE(metadata.category == TransformCategory::ML);
    REQUIRE(metadata.plotKind == TransformPlotKind::sentiment);

    // Verify outputs
    REQUIRE(metadata.outputs.size() == 2);
    REQUIRE(metadata.outputs[0].id == "sentiment");
    REQUIRE(metadata.outputs[0].type == IODataType::String);
    REQUIRE(metadata.outputs[1].id == "score");
    REQUIRE(metadata.outputs[1].type == IODataType::Decimal);
  }
}

// Integration test (requires AWS credentials and active SageMaker endpoint)
// Mark with [.integration] tag to skip by default in CI/CD
TEST_CASE("FinBERT Sentiment Transform - Integration Test",
          "[.integration][ml][finbert][aws]") {

  // This test requires:
  // 1. AWS credentials in environment: AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY
  // 2. AWS_REGION=us-west-2
  // 3. Active SageMaker endpoint named "finbert"

  auto input = createFinancialTextDataFrame();
  auto index = input.index();
  const auto tf = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  SECTION("End-to-end sentiment analysis") {
    auto config = finbert_sentiment_cfg("test_integration", "text", tf);
    auto transformBase = MAKE_TRANSFORM(config);
    auto transform = dynamic_cast<ITransform *>(transformBase.get());

    // Execute transform
    DataFrame output = transform->TransformData(input);

    // Verify output structure
    REQUIRE(output.num_cols() == 2);
    REQUIRE(output.contains(config.GetOutputId("sentiment")));
    REQUIRE(output.contains(config.GetOutputId("score")));
    REQUIRE(output.size() == 5);

    // Get sentiment and score columns
    auto sentiment_col = output[config.GetOutputId("sentiment")];
    auto score_col = output[config.GetOutputId("score")];

    // Verify first result (positive sentiment expected)
    REQUIRE(sentiment_col.iloc(0).repr() == "positive");
    REQUIRE(score_col.iloc(0).as_double() > 0.9);

    // Verify second result (negative sentiment expected)
    REQUIRE(sentiment_col.iloc(1).repr() == "negative");
    REQUIRE(score_col.iloc(1).as_double() > 0.9);

    // Verify all scores are in valid range
    for (size_t i = 0; i < output.size(); ++i) {
      double score = score_col.iloc(i).as_double();
      REQUIRE(score >= 0.0);
      REQUIRE(score <= 1.0);

      // Verify sentiment is one of the valid labels
      std::string sentiment = sentiment_col.iloc(i).repr();
      REQUIRE((sentiment == "positive" ||
               sentiment == "neutral" ||
               sentiment == "negative"));
    }
  }
}
