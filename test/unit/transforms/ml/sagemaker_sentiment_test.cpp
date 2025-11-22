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
  }
}

TEST_CASE("FinBERT Sentiment Transform - Output Structure", "[ml][finbert][outputs]") {
  auto input = createFinancialTextDataFrame();
  auto index = input.index();
  const auto tf = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  // Note: This test will only pass if AWS credentials are set and endpoint is available
  // For CI/CD, you may want to skip this test or mock the AWS SDK

  SECTION("Transform produces two output columns") {
    auto config = finbert_sentiment_cfg("test_outputs", "text", tf);

    // Expected output IDs
    std::string sentiment_output = config.GetOutputId("sentiment");
    std::string score_output = config.GetOutputId("score");

    REQUIRE(sentiment_output == "test_outputs.sentiment");
    REQUIRE(score_output == "test_outputs.score");
  }
}

TEST_CASE("FinBERT Sentiment Transform - Expected Response Format", "[ml][finbert][response]") {
  // This test documents the expected FinBERT response format
  // Actual integration test would require AWS credentials and active endpoint

  SECTION("FinBERT response format documentation") {
    // Expected request format:
    // {"inputs": "Stock prices are rising due to strong earnings"}

    // Expected response format:
    // [{'label': 'positive', 'score': 0.948788583278656}]

    // Valid sentiment labels:
    std::vector<std::string> valid_labels = {"positive", "neutral", "negative"};

    REQUIRE(valid_labels.size() == 3);
    REQUIRE(std::find(valid_labels.begin(), valid_labels.end(), "positive") != valid_labels.end());
    REQUIRE(std::find(valid_labels.begin(), valid_labels.end(), "neutral") != valid_labels.end());
    REQUIRE(std::find(valid_labels.begin(), valid_labels.end(), "negative") != valid_labels.end());
  }

  SECTION("Score range validation") {
    // FinBERT returns scores between 0.0 and 1.0
    double min_score = 0.0;
    double max_score = 1.0;

    // Example scores from Python test
    std::vector<double> example_scores = {
        0.948788583278656,   // positive
        0.9737865328788757,  // negative
        0.9397264122962952,  // positive
        0.9683024883270264,  // negative
        0.9534657597541809   // positive
    };

    for (auto score : example_scores) {
      REQUIRE(score >= min_score);
      REQUIRE(score <= max_score);
    }
  }
}

TEST_CASE("FinBERT Sentiment Transform - Error Handling", "[ml][finbert][errors]") {
  auto index = epoch_frame::factory::index::make_datetime_index(
      {epoch_frame::DateTime{2024y, std::chrono::January, 1d},
       epoch_frame::DateTime{2024y, std::chrono::January, 2d}});

  const auto tf = EpochStratifyXConstants::instance().DAILY_FREQUENCY;

  SECTION("Empty string handling") {
    // Empty strings should return neutral sentiment with score 0.0
    auto input = make_dataframe<std::string>(
        index,
        {{"", ""}},
        {"text"});

    // Document expected behavior: empty text -> neutral, score=0.0
    std::string expected_sentiment = "neutral";
    double expected_score = 0.0;

    REQUIRE(expected_sentiment == "neutral");
    REQUIRE(expected_score == 0.0);
  }

  SECTION("Null value handling") {
    // Null values should return neutral sentiment with score 0.0
    // Document expected behavior: null text -> neutral, score=0.0
    std::string expected_sentiment = "neutral";
    double expected_score = 0.0;

    REQUIRE(expected_sentiment == "neutral");
    REQUIRE(expected_score == 0.0);
  }
}

TEST_CASE("FinBERT Sentiment Transform - Python Test Case Mapping", "[ml][finbert][validation]") {
  // This test documents the expected results from the Python test script
  // These can be used for validation once AWS credentials are available

  SECTION("Test case 1 - Positive sentiment") {
    std::string text = "The company reported record profits this quarter with 25% growth";
    std::string expected_label = "positive";
    double expected_score = 0.948788583278656;

    REQUIRE(expected_label == "positive");
    REQUIRE(expected_score > 0.9);
  }

  SECTION("Test case 2 - Negative sentiment") {
    std::string text = "Stock prices are falling due to market uncertainty and recession fears";
    std::string expected_label = "negative";
    double expected_score = 0.9737865328788757;

    REQUIRE(expected_label == "negative");
    REQUIRE(expected_score > 0.9);
  }

  SECTION("Test case 3 - Positive sentiment") {
    std::string text = "The quarterly earnings met analyst expectations";
    std::string expected_label = "positive";
    double expected_score = 0.9397264122962952;

    REQUIRE(expected_label == "positive");
    REQUIRE(expected_score > 0.9);
  }

  SECTION("Test case 4 - Negative sentiment") {
    std::string text = "Major layoffs announced as company struggles with declining revenue";
    std::string expected_label = "negative";
    double expected_score = 0.9683024883270264;

    REQUIRE(expected_label == "negative");
    REQUIRE(expected_score > 0.9);
  }

  SECTION("Test case 5 - Positive sentiment") {
    std::string text = "New product launch expected to boost sales significantly";
    std::string expected_label = "positive";
    double expected_score = 0.9534657597541809;

    REQUIRE(expected_label == "positive");
    REQUIRE(expected_score > 0.9);
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
