#include "candles.h"
#include "common.h"
#include "epoch_core/common_utils.h"
#include "epoch_metadata/transforms/metadata.h"
#include <epoch_core/ranges_to.h>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace epoch_metadata::transforms {

struct CandlePatternMetaData {
  std::vector<std::string> tags;
  std::string desc;
};

std::unordered_map<std::string, CandlePatternMetaData>
MakeCandlePatternMetaData() {
  std::unordered_map<std::string, CandlePatternMetaData> patternMetaData;

  patternMetaData["abandoned_baby_bear"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bearish", "reversal",
               "abandoned-baby"},
      .desc = "Bearish reversal pattern with a large up candle, followed by a "
              "gapped doji, and a gapped down candle. Signals potential "
              "downward trend."};

  patternMetaData["abandoned_baby_bull"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bullish", "reversal",
               "abandoned-baby"},
      .desc = "Bullish reversal pattern with a large down candle, followed by "
              "a gapped doji, and a gapped up candle. Signals potential upward "
              "trend."};

  patternMetaData["big_black_candle"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bearish", "continuation",
               "big-candle"},
      .desc = "Large bearish candle with a long body. Indicates strong selling "
              "pressure and potential downward momentum."};

  patternMetaData["big_white_candle"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bullish", "continuation",
               "big-candle"},
      .desc = "Large bullish candle with a long body. Indicates strong buying "
              "pressure and potential upward momentum."};

  patternMetaData["black_marubozu"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bearish", "marubozu", "no-shadow"},
      .desc = "Bearish candle with no upper or lower shadows (wicks). Strong "
              "selling pressure with opening at high and closing at low."};

  patternMetaData["doji"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "neutral", "indecision", "doji"},
      .desc =
          "Candle with virtually no body where open and close are at the same "
          "level. Indicates market indecision and potential reversal."};

  patternMetaData["dragonfly_doji"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bullish", "reversal", "doji"},
      .desc = "Doji with no upper shadow but a long lower shadow. Indicates "
              "rejection of lower prices and potential bullish reversal."};

  patternMetaData["engulfing_bear"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bearish", "reversal", "engulfing"},
      .desc =
          "Bearish pattern where a large down candle completely engulfs the "
          "previous up candle. Strong signal of trend reversal to downside."};

  patternMetaData["engulfing_bull"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bullish", "reversal", "engulfing"},
      .desc =
          "Bullish pattern where a large up candle completely engulfs the "
          "previous down candle. Strong signal of trend reversal to upside."};

  patternMetaData["evening_doji_star"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bearish", "reversal", "star", "doji"},
      .desc = "Bearish reversal pattern with an up candle, followed by a doji "
              "gapped up, then a down candle gapped down. Stronger signal than "
              "Evening Star."};

  patternMetaData["evening_star"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bearish", "reversal", "star"},
      .desc = "Bearish reversal pattern with an up candle, followed by a small "
              "body candle gapped up, then a down candle gapped down."};

  patternMetaData["four_price_doji"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "neutral", "indecision", "doji"},
      .desc = "Special doji where open, high, low, and close are all at the "
              "same price. Extreme indecision in the market."};

  patternMetaData["gravestone_doji"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bearish", "reversal", "doji"},
      .desc = "Doji with no lower shadow but a long upper shadow. Indicates "
              "rejection of higher prices and potential bearish reversal."};

  patternMetaData["hammer"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bullish", "reversal", "hammer"},
      .desc =
          "Bullish reversal pattern with a small body at the top and a long "
          "lower shadow. Indicates rejection of lower prices in a downtrend."};

  patternMetaData["hanging_man"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bearish", "reversal", "hanging-man"},
      .desc = "Bearish reversal pattern with a small body at the top and a "
              "long lower shadow, appearing in an uptrend. Warning of a "
              "potential reversal."};

  patternMetaData["inverted_hammer"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bullish", "reversal", "hammer"},
      .desc = "Bullish reversal pattern with a small body at the bottom and a "
              "long upper shadow, appearing after a downtrend."};

  patternMetaData["long_legged_doji"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "neutral", "indecision", "doji",
               "volatility"},
      .desc = "Doji with long upper and lower shadows. Indicates significant "
              "volatility and indecision in the market."};

  patternMetaData["marubozu"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "neutral", "strong-momentum",
               "marubozu", "no-shadow"},
      .desc = "Candle with no upper or lower shadows. Indicates strong "
              "conviction in the direction of the trend."};

  patternMetaData["morning_doji_star"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bullish", "reversal", "star", "doji"},
      .desc = "Bullish reversal pattern with a down candle, followed by a doji "
              "gapped down, then an up candle gapped up. Stronger signal than "
              "Morning Star."};

  patternMetaData["morning_star"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bullish", "reversal", "star"},
      .desc = "Bullish reversal pattern with a down candle, followed by a "
              "small body candle gapped down, then an up candle gapped up."};

  patternMetaData["shooting_star"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bearish", "reversal",
               "shooting-star"},
      .desc = "Bearish reversal pattern with a small body at the bottom and a "
              "long upper shadow, appearing after an uptrend."};

  patternMetaData["spinning_top"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "neutral", "indecision",
               "spinning-top"},
      .desc = "Candle with a small body and longer upper and lower shadows. "
              "Indicates indecision between buyers and sellers."};

  patternMetaData["star"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "neutral", "star", "gap"},
      .desc = "Price gap between the current candle's body and the previous "
              "candle's body. Often a component of more complex patterns."};

  patternMetaData["three_black_crows"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bearish", "reversal", "three-crows"},
      .desc =
          "Bearish reversal pattern with three consecutive black candles with "
          "lower closes. Strong signal of continued downward momentum."};

  patternMetaData["three_white_soldiers"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bullish", "reversal",
               "three-soldiers"},
      .desc =
          "Bullish reversal pattern with three consecutive white candles with "
          "higher closes. Strong signal of continued upward momentum."};

  patternMetaData["white_marubozu"] = CandlePatternMetaData{
      .tags = {"candlestick", "pattern", "bullish", "marubozu", "no-shadow"},
      .desc = "Bullish candle with no upper or lower shadows (wicks). Strong "
              "buying pressure with opening at low and closing at high."};

  return patternMetaData;
}

std::vector<MetaDataOption> MakeCandleOptions() {
  // Derive defaults from tc_config_default()
  auto defaults = tc_config_default();

  std::vector<MetaDataOption> options;

  // period is integer
  {
    MetaDataOption o{.id = "period",
                     .name = "Period",
                     .type = epoch_core::MetaDataOptionType::Integer,
                     .defaultValue = static_cast<double>(defaults->period),
                     .isRequired = true,
                     .min = 1, // Period must be at least 1
                     .max = 1000};
    options.push_back(o);
  }

  // body_none
  {
    MetaDataOption o{.id = "body_none",
                     .name = "Body None Threshold",
                     .type = epoch_core::MetaDataOptionType::Decimal,
                     .defaultValue = static_cast<double>(defaults->body_none),
                     .isRequired = true};
    options.push_back(o);
  }

  // body_short
  {
    MetaDataOption o{.id = "body_short",
                     .name = "Body Short Threshold",
                     .type = epoch_core::MetaDataOptionType::Decimal,
                     .defaultValue = static_cast<double>(defaults->body_short),
                     .isRequired = true};
    options.push_back(o);
  }

  // body_long
  {
    MetaDataOption o{.id = "body_long",
                     .name = "Body Long Threshold",
                     .type = epoch_core::MetaDataOptionType::Decimal,
                     .defaultValue = static_cast<double>(defaults->body_long),
                     .isRequired = true};
    options.push_back(o);
  }

  // wick_none
  {
    MetaDataOption o{.id = "wick_none",
                     .name = "Wick None Threshold",
                     .type = epoch_core::MetaDataOptionType::Decimal,
                     .defaultValue = static_cast<double>(defaults->wick_none),
                     .isRequired = true};
    options.push_back(o);
  }

  // wick_long
  {
    MetaDataOption o{.id = "wick_long",
                     .name = "Wick Long Threshold",
                     .type = epoch_core::MetaDataOptionType::Decimal,
                     .defaultValue = static_cast<double>(defaults->wick_long),
                     .isRequired = true};
    options.push_back(o);
  }

  // near
  {
    MetaDataOption o{.id = "near",
                     .name = "Near Threshold",
                     .type = epoch_core::MetaDataOptionType::Decimal,
                     .defaultValue = static_cast<double>(defaults->near),
                     .isRequired = true};
    options.push_back(o);
  }

  return options;
}

std::vector<TransformsMetaData> MakeTulipCandles() {
  // Create metadata for each candlestick pattern
  auto patternMetaData = MakeCandlePatternMetaData();

  // Iterate over tc_candles until we hit a null element.
  std::vector<TransformsMetaData> allCandles(TC_CANDLE_COUNT);

  for (size_t i = 0; i < TC_CANDLE_COUNT; ++i) {
    const auto &c = tc_candles[i];

    // Look up metadata for this pattern
    auto metadata = epoch_core::lookupDefault(patternMetaData, c.name,
                                              CandlePatternMetaData{});

    allCandles[i] = TransformsMetaData{
        .id = c.name,
        .category = epoch_core::TransformCategory::PriceAction,
        .renderKind = epoch_core::TransformNodeRenderKind::Standard,
        .plotKind = epoch_core::TransformPlotKind::flag,
        .name = c.full_name,
        .options = MakeCandleOptions(),
        .isCrossSectional = false,
        .desc = metadata.desc,
        .inputs = {},
        .outputs = {IOMetaDataConstants::BOOLEAN_OUTPUT_METADATA},
        .tags = metadata.tags,
        .requiresTimeFrame = true,
        .requiredDataSources = {"c", "o", "h", "l"}};
  }

  return allCandles;
}

} // namespace epoch_metadata::transforms