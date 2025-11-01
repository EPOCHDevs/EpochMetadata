//
// Created by adesola on 5/15/25.
//

#include <epochflow/transforms/core/registration.h>
#include "agg.h"
#include "data_source.h"
#include <epochflow/transforms/core/registry.h>
#include <epochflow/transforms/core/trade_executors.h>
#include <epochflow/transforms/core/transform_registry.h>

#include "hosseinmoein/indicators/hurst_exponent.h"
#include "hosseinmoein/volatility/hodges_tompkins.h"
#include "hosseinmoein/volatility/ulcer_index.h"
#include "price_actions/smc/bos_choch.h"
#include "price_actions/smc/fvg.h"
#include "price_actions/smc/liquidity.h"
#include "price_actions/smc/ob.h"
#include "price_actions/smc/previous_high_low.h"
#include "price_actions/smc/retracements.h"
#include "price_actions/smc/sessions.h"
#include "price_actions/smc/session_time_window.h"
#include "price_actions/smc/swing_highs_lows.h"
#include "statistics/hmm.h"
#include "hosseinmoein/statistics/rolling_corr.h"
#include "hosseinmoein/statistics/rolling_cov.h"
#include "hosseinmoein/statistics/ewm_corr.h"
#include "hosseinmoein/statistics/ewm_cov.h"

// Chart Formation Pattern Transforms
#include "price_actions/infrastructure/flexible_pivot_detector.h"
#include "price_actions/chart_formations/head_and_shoulders.h"
#include "price_actions/chart_formations/inverse_head_and_shoulders.h"
#include "price_actions/chart_formations/double_top_bottom.h"
#include "price_actions/chart_formations/flag.h"
#include "price_actions/chart_formations/triangles.h"
#include "price_actions/chart_formations/pennant.h"
#include "price_actions/chart_formations/consolidation_box.h"
#include <epochflow/strategy/registration.h>

// Calendar Effects
#include "calendar/calendar_effect.h"

// String Operations
#include "string/string_operations.h"

// Data Source includes
#include "data_sources/polygon_data_source.h"
#include "data_sources/polygon_metadata.h"
#include "data_sources/polygon_indices_metadata.h"
#include "data_sources/fred_metadata.h"
#include "data_sources/fred_transform.h"

// Selector includes
#include <epochflow/transforms/components/selectors/card_selector.h>

// SQL and Report includes
#include "sql/sql_query_transform.h"
#include "sql/sql_query_metadata.h"
#include "reports/numeric_card_report.h"
#include "reports/boolean_card_report.h"
#include "reports/any_card_report.h"
#include "reports/index_card_report.h"
#include "reports/quantile_card_report.h"
#include "reports/gap_report.h"
#include "reports/table_report.h"
#include "reports/bar_chart_report.h"
#include "reports/pie_chart_report.h"
#include "reports/nested_pie_chart_report.h"
#include "reports/histogram_chart_report.h"
// #include "reports/card_selector_report.h"  // Commented out - missing epoch_proto types

#include "cross_sectional/rank.h"
#include "cross_sectional/returns.h"
#include "cummulative/cum_op.h"

#include "hosseinmoein/hosseinmoein.h"

#include "indicators/bband_variant.h"
#include "indicators/forward_returns.h"
#include "indicators/session_gap.h"
#include "indicators/bar_gap.h"
#include "indicators/lag.h"
#include "indicators/moving_average.h"
#include "operators/equality.h"
#include "operators/logical.h"
#include "operators/select.h"
#include "scalar.h"
#include "tulip/tulip_model.h"
#include "volatility/volatility.h"

#include <candles.h>

namespace epochflow::transform {
void InitializeTransforms(
    std::function<YAML::Node(std::string const &)> const &loader,
    std::vector<std::string> const &algorithmBuffers,
    std::vector<std::string> const &strategyBuffers) {
  epochflow::strategy::RegisterStrategyMetadata(loader, algorithmBuffers,
                                                     strategyBuffers);

  // Scalar Transforms
  REGISTER_TRANSFORM(number, NumericScalarDataFrameTransform);
  REGISTER_TRANSFORM(text, StringScalarDataFrameTransform);

  REGISTER_TRANSFORM(bool_true, BoolTrueScalar);
  REGISTER_TRANSFORM(bool_false, BoolFalseScalar);
  REGISTER_TRANSFORM(zero, ZeroScalar);
  REGISTER_TRANSFORM(one, OneScalar);
  REGISTER_TRANSFORM(negative_one, NegativeOneScalar);
  REGISTER_TRANSFORM(pi, PiScalar);
  REGISTER_TRANSFORM(e, EScalar);
  REGISTER_TRANSFORM(phi, PhiScalar);
  REGISTER_TRANSFORM(sqrt2, Sqrt2Scalar);
  REGISTER_TRANSFORM(sqrt3, Sqrt3Scalar);
  REGISTER_TRANSFORM(sqrt5, Sqrt5Scalar);
  REGISTER_TRANSFORM(ln2, Ln2Scalar);
  REGISTER_TRANSFORM(ln10, Ln10Scalar);
  REGISTER_TRANSFORM(log2e, Log2EScalar);
  REGISTER_TRANSFORM(log10e, Log10EScalar);
  REGISTER_TRANSFORM(null, NullScalar);

  // String Transforms
  REGISTER_TRANSFORM(string_case, StringCaseTransform);
  REGISTER_TRANSFORM(string_trim, StringTrimTransform);
  REGISTER_TRANSFORM(string_pad, StringPadTransform);
  REGISTER_TRANSFORM(string_contains, StringContainsTransform);
  REGISTER_TRANSFORM(string_check, StringCheckTransform);
  REGISTER_TRANSFORM(string_replace, StringReplaceTransform);
  REGISTER_TRANSFORM(string_length, StringLengthTransform);
  REGISTER_TRANSFORM(string_reverse, StringReverseTransform);

  // Vector Transforms
  REGISTER_TRANSFORM(gt, VectorGT);
  REGISTER_TRANSFORM(gte, VectorGTE);
  REGISTER_TRANSFORM(lt, VectorLT);
  REGISTER_TRANSFORM(lte, VectorLTE);
  REGISTER_TRANSFORM(eq, VectorEQ);
  REGISTER_TRANSFORM(neq, VectorNEQ);

  REGISTER_TRANSFORM(logical_or, LogicalOR);
  REGISTER_TRANSFORM(logical_and, LogicalAND);
  REGISTER_TRANSFORM(logical_xor, LogicalXOR);
  REGISTER_TRANSFORM(logical_and_not, LogicalAND_NOT);
  REGISTER_TRANSFORM(logical_not, LogicalNot);

  REGISTER_TRANSFORM(boolean_select, BooleanSelectTransform);
  REGISTER_TRANSFORM(select_2, Select2);
  REGISTER_TRANSFORM(select_3, Select3);
  REGISTER_TRANSFORM(select_4, Select4);
  REGISTER_TRANSFORM(select_5, Select5);
  REGISTER_TRANSFORM(first_non_null, FirstNonNullTransform);
  REGISTER_TRANSFORM(conditional_select, ConditionalSelectTransform);

  REGISTER_TRANSFORM(previous_gt, GreaterThanPrevious);
  REGISTER_TRANSFORM(previous_gte, GreaterThanOrEqualsPrevious);
  REGISTER_TRANSFORM(previous_lt, LessThanPrevious);
  REGISTER_TRANSFORM(previous_lte, LessThanOrEqualsPrevious);
  REGISTER_TRANSFORM(previous_eq, EqualsPrevious);
  REGISTER_TRANSFORM(previous_neq, NotEqualsPrevious);

  REGISTER_TRANSFORM(highest_gt, GreaterThanHighest);
  REGISTER_TRANSFORM(highest_gte, GreaterThanOrEqualsHighest);
  REGISTER_TRANSFORM(highest_lt, LessThanHighest);
  REGISTER_TRANSFORM(highest_lte, LessThanOrEqualsHighest);
  REGISTER_TRANSFORM(highest_eq, EqualsHighest);
  REGISTER_TRANSFORM(highest_neq, NotEqualsHighest);

  REGISTER_TRANSFORM(lowest_gt, GreaterThanLowest);
  REGISTER_TRANSFORM(lowest_gte, GreaterThanOrEqualsLowest);
  REGISTER_TRANSFORM(lowest_lt, LessThanLowest);
  REGISTER_TRANSFORM(lowest_lte, LessThanOrEqualsLowest);
  REGISTER_TRANSFORM(lowest_eq, EqualsLowest);
  REGISTER_TRANSFORM(lowest_neq, NotEqualsLowest);

  REGISTER_TRANSFORM(market_data_source, DataSourceTransform);
  REGISTER_TRANSFORM(percentile_select, PercentileSelect);
  REGISTER_TRANSFORM(boolean_branch, BooleanBranch);
  REGISTER_TRANSFORM(ratio_branch, RatioBranch);

  REGISTER_TRANSFORM(cum_prod, CumProdOperation);
  REGISTER_TRANSFORM(cs_momentum, CrossSectionalMomentumOperation);
  REGISTER_TRANSFORM(top_k, CrossSectionalTopKOperation);
  REGISTER_TRANSFORM(bottom_k, CrossSectionalBottomKOperation);
  REGISTER_TRANSFORM(top_k_percent, CrossSectionalTopKPercentileOperation);
  REGISTER_TRANSFORM(bottom_k_percent,
                     CrossSectionalBottomKPercentileOperation);

  REGISTER_TRANSFORM(bband_percent, BollingerBandsPercent);
  REGISTER_TRANSFORM(bband_width, BollingerBandsWidth);

  // Gap detection transforms
  REGISTER_TRANSFORM(session_gap, SessionGap);
  REGISTER_TRANSFORM(bar_gap, BarGap);

  REGISTER_TRANSFORM(forward_returns, ForwardReturns);
  REGISTER_TRANSFORM(lag, Lag);
  REGISTER_TRANSFORM(ma, MovingAverage);

  REGISTER_TRANSFORM(price_diff_vol, PriceDiffVolatility);
  REGISTER_TRANSFORM(return_vol, ReturnVolatility);

  // Price Action Transforms - SMC
  REGISTER_TRANSFORM(bos_choch, BosChoch);
  REGISTER_TRANSFORM(fair_value_gap, FairValueGap);
  REGISTER_TRANSFORM(liquidity, Liquidity);
  REGISTER_TRANSFORM(order_blocks, OrderBlocks);
  REGISTER_TRANSFORM(previous_high_low, PreviousHighLow);
  REGISTER_TRANSFORM(retracements, Retracements);
  REGISTER_TRANSFORM(sessions, DefaultSessions);
  REGISTER_TRANSFORM(session_time_window, SessionTimeWindow);
  REGISTER_TRANSFORM(swing_highs_lows, SwingHighsLows);

  // Price Action Transforms - Infrastructure
  REGISTER_TRANSFORM(flexible_pivot_detector, FlexiblePivotDetector);

  // Price Action Transforms - Chart Formations
  REGISTER_TRANSFORM(head_and_shoulders, HeadAndShoulders);
  REGISTER_TRANSFORM(inverse_head_and_shoulders, InverseHeadAndShoulders);
  REGISTER_TRANSFORM(double_top_bottom, DoubleTopBottom);
  REGISTER_TRANSFORM(flag, Flag);
  REGISTER_TRANSFORM(triangles, Triangles);
  REGISTER_TRANSFORM(pennant, Pennant);
  REGISTER_TRANSFORM(consolidation_box, ConsolidationBox);

  // Aggregate Transforms
  REGISTER_TRANSFORM(agg_sum, SumAggregateTransform);
  REGISTER_TRANSFORM(agg_mean, AverageAggregateTransform);
  REGISTER_TRANSFORM(agg_min, MinAggregateTransform);
  REGISTER_TRANSFORM(agg_max, MaxAggregateTransform);
  REGISTER_TRANSFORM(agg_all_of, AllOfAggregateTransform);
  REGISTER_TRANSFORM(agg_any_of, AnyOfAggregateTransform);
  REGISTER_TRANSFORM(agg_none_of, NoneOfAggregateTransform);
  REGISTER_TRANSFORM(agg_all_equal, AllEqualAggregateTransform);
  REGISTER_TRANSFORM(agg_all_unique, AllUniqueAggregateTransform);

  std::unordered_set<std::string> tiToSkip{"lag"};
  for (auto const &metaData :
       std::span(ti_indicators, ti_indicators + ti_indicator_count())) {
    if (not tiToSkip.contains(metaData.name)) {
      transform::Register<TulipModelImpl<true>>(metaData.name);
    }
  }

  // Register custom Tulip-based indicators that are not native to Tulip
  // crossunder is implemented as crossover with swapped inputs
  transform::Register<TulipModelImpl<true>>("crossunder");

  for (auto const &metaData :
       std::span(tc_candles, tc_candles + tc_candle_count())) {
    transform::Register<TulipModelImpl<false>>(metaData.name);
  }

  // Hossein Moein Transforms
  REGISTER_TRANSFORM(acceleration_bands, AccelerationBands);
  REGISTER_TRANSFORM(garman_klass, GarmanKlass);
  REGISTER_TRANSFORM(hodges_tompkins, HodgesTompkins);
  REGISTER_TRANSFORM(keltner_channels, KeltnerChannels);
  REGISTER_TRANSFORM(parkinson, Parkinson);
  REGISTER_TRANSFORM(ulcer_index, UlcerIndex);
  REGISTER_TRANSFORM(yang_zhang, YangZhang);

  REGISTER_TRANSFORM(chande_kroll_stop, ChandeKrollStop);
  REGISTER_TRANSFORM(donchian_channel, DonchianChannel);
  REGISTER_TRANSFORM(elders_thermometer, EldersThermometer);
  REGISTER_TRANSFORM(hurst_exponent, HurstExponent);
  REGISTER_TRANSFORM(rolling_hurst_exponent, RollingHurstExponent);
  REGISTER_TRANSFORM(ichimoku, Ichimoku);
  REGISTER_TRANSFORM(pivot_point_sr, PivotPointSR);
  REGISTER_TRANSFORM(price_distance, PriceDistance);
  REGISTER_TRANSFORM(psl, PSL);
  REGISTER_TRANSFORM(qqe, QuantQualEstimation);
  REGISTER_TRANSFORM(vortex, Vortex);
  REGISTER_TRANSFORM(zscore, ZScore);

  // Statistical Transforms
  REGISTER_TRANSFORM(rolling_corr, RollingCorr);
  REGISTER_TRANSFORM(rolling_cov, RollingCov);
  REGISTER_TRANSFORM(ewm_corr, EWMCorr);
  REGISTER_TRANSFORM(ewm_cov, EWMCov);

  REGISTER_TRANSFORM(trade_executor_adapter, TradeExecutorAdapter);
  REGISTER_TRANSFORM(trade_signal_executor, TradeExecutorTransform);


  // Statistics Transforms
  REGISTER_TRANSFORM(hmm, HMMTransform);

  // Calendar Effects Transforms
  REGISTER_TRANSFORM(turn_of_month, TurnOfMonthEffect);
  REGISTER_TRANSFORM(day_of_week, DayOfWeekEffect);
  REGISTER_TRANSFORM(month_of_year, MonthOfYearEffect);
  REGISTER_TRANSFORM(quarter, QuarterEffect);
  REGISTER_TRANSFORM(holiday, HolidayEffect);
  REGISTER_TRANSFORM(week_of_month, WeekOfMonthEffect);

  // Fundamental & Market Data Source Transforms
  REGISTER_TRANSFORM(balance_sheet, PolygonBalanceSheetTransform);
  REGISTER_TRANSFORM(income_statement, PolygonIncomeStatementTransform);
  REGISTER_TRANSFORM(cash_flow, PolygonCashFlowTransform);
  REGISTER_TRANSFORM(financial_ratios, PolygonFinancialRatiosTransform);
  REGISTER_TRANSFORM(quotes, PolygonQuotesTransform);
  REGISTER_TRANSFORM(trades, PolygonTradesTransform);
  REGISTER_TRANSFORM(aggregates, PolygonAggregatesTransform);
  REGISTER_TRANSFORM(common_indices, PolygonCommonIndicesTransform);
  REGISTER_TRANSFORM(indices, PolygonIndicesTransform);

  // Economic Data Source Transforms
  REGISTER_TRANSFORM(economic_indicator, FREDTransform);

  // Register Selectors
  REGISTER_TRANSFORM(card_selector_filter, CardSelectorFromFilter);
  transforms::ITransformRegistry::GetInstance().Register(
    SelectorMetadata::Get());

  // SQL Query Transforms (1-4 outputs) - DISABLED
  // REGISTER_TRANSFORM(sql_query_1, SQLQueryTransform1);
  // REGISTER_TRANSFORM(sql_query_2, SQLQueryTransform2);
  // REGISTER_TRANSFORM(sql_query_3, SQLQueryTransform3);
  // REGISTER_TRANSFORM(sql_query_4, SQLQueryTransform4);

  // Register Reports
  reports::RegisterReport<reports::NumericCardReport>();
  reports::RegisterReport<reports::BooleanCardReport>();
  reports::RegisterReport<reports::AnyCardReport>();
  reports::RegisterReport<reports::IndexCardReport>();
  reports::RegisterReport<reports::QuantileCardReport>();
  reports::RegisterReport<reports::TableReport>();

  // Register Chart Reports
  reports::RegisterReport<reports::BarChartReport>();
  reports::RegisterReport<reports::PieChartReport>();
  reports::RegisterReport<reports::NestedPieChartReport>();
  reports::RegisterReport<reports::HistogramChartReport>();

  // Register Specialized Reports
  reports::RegisterReport<reports::GapReport>();
  // reports::RegisterReport<reports::CardSelectorReport>();  // Commented out - missing epoch_proto types

};
} // namespace epochflow::transform