# Transform Documentation Verification Report

**Generated**: verify_docs.py
**Metadata**: transform_metadata.json
**Documentation**: docs/epochscript/core-transforms.md

## Summary

- **Actual transforms in codebase**: 281
- **Documented transforms**: 52
- **Total discrepancies**: 336

- **ERROR**: 53
- **WARNING**: 283
- **INFO**: 0

---

## Coverage Analysis

**Documentation coverage**: 52 / 281 (18.5%)

### Undocumented Transforms

237 transforms exist in the codebase but are not documented:

- **** (Aggregate)
- **abandoned_baby_bear** (PriceAction)
- **abandoned_baby_bull** (PriceAction)
- **abs** (Math)
- **acceleration_bands** (Volatility)
- **acos** (Math)
- **add** (Math)
- **adxr** (Trend)
- **agg_all_equal** (Aggregate)
- **agg_all_of** (Aggregate)
- **agg_all_unique** (Aggregate)
- **agg_any_of** (Aggregate)
- **agg_max** (Aggregate)
- **agg_mean** (Aggregate)
- **agg_min** (Aggregate)
- **agg_none_of** (Aggregate)
- **agg_sum** (Aggregate)
- **aggregates** (DataSource)
- **ao** (Momentum)
- **apo** (Momentum)
- **aroonosc** (Trend)
- **asin** (Math)
- **atan** (Math)
- **avgprice** (Trend)
- **balance_sheet** (DataSource)
- **big_black_candle** (PriceAction)
- **big_white_candle** (PriceAction)
- **black_marubozu** (PriceAction)
- **bool_false** (Scalar)
- **bool_true** (Scalar)
- **boolean_branch** (ControlFlow)
- **boolean_select** (ControlFlow)
- **bop** (Momentum)
- **bos_choch** (PriceAction)
- **bottom_k** (Momentum)
- **bottom_k_percent** (Momentum)
- **cash_flow** (DataSource)
- **ceil** (Math)
- **chande_kroll_stop** (Trend)
- **cmo** (Momentum)
- **common_indices** (DataSource)
- **consolidation_box** (PriceAction)
- **cos** (Math)
- **cosh** (Math)
- **crossany** (Math)
- **cs_momentum** (Momentum)
- **cum_prod** (Math)
- **cvi** (Volatility)
- **decay** (Math)
- **dema** (Trend)
- **di** (Trend)
- **div** (Math)
- **dm** (Trend)
- **doji** (PriceAction)
- **donchian_channel** (Trend)
- **double_top_bottom** (PriceAction)
- **dpo** (Trend)
- **dragonfly_doji** (PriceAction)
- **dx** (Trend)
- **e** (Scalar)
- **economic_indicator** (DataSource)
- **edecay** (Math)
- **elders_thermometer** (Momentum)
- **emv** (Volume)
- **engulfing_bear** (PriceAction)
- **engulfing_bull** (PriceAction)
- **eq** (Utility)
- **evening_doji_star** (PriceAction)
- **evening_star** (PriceAction)
- **exp** (Math)
- **financial_ratios** (DataSource)
- **fisher** (Momentum)
- **flag** (PriceAction)
- **flexible_pivot_detector** (PriceAction)
- **floor** (Math)
- **form13f_holdings** (DataSource)
- **forward_returns** (Trend)
- **fosc** (Momentum)
- **four_price_doji** (PriceAction)
- **garman_klass** (Volatility)
- **gravestone_doji** (PriceAction)
- **gt** (Math)
- **gte** (Math)
- **hammer** (PriceAction)
- **hanging_man** (PriceAction)
- **highest_eq** (Math)
- **highest_gt** (Math)
- **highest_gte** (Math)
- **highest_lt** (Math)
- **highest_lte** (Math)
- **highest_neq** (Math)
- **hmm** (Statistical)
- **hodges_tompkins** (Volatility)
- **holiday** (Statistical)
- **hurst_exponent** (Momentum)
- **ichimoku** (Trend)
- **income_statement** (DataSource)
- **indices** (DataSource)
- **insider_trading** (DataSource)
- **inverse_head_and_shoulders** (PriceAction)
- **inverted_hammer** (PriceAction)
- **keltner_channels** (Volatility)
- **kvo** (Volume)
- **linreg** (Statistical)
- **linregintercept** (Statistical)
- **linregslope** (Statistical)
- **liquidity** (PriceAction)
- **ln** (Math)
- **ln10** (Scalar)
- **ln2** (Scalar)
- **log10** (Math)
- **log10e** (Scalar)
- **log2e** (Scalar)
- **logical_and** (Math)
- **logical_and_not** (Math)
- **logical_not** (Math)
- **logical_or** (Math)
- **logical_xor** (Math)
- **long_legged_doji** (PriceAction)
- **lowest_eq** (Math)
- **lowest_gt** (Math)
- **lowest_gte** (Math)
- **lowest_lt** (Math)
- **lowest_lte** (Math)
- **lowest_neq** (Math)
- **lt** (Math)
- **lte** (Math)
- **ma** (Trend)
- **marketfi** (Volume)
- **marubozu** (PriceAction)
- **mass** (Volatility)
- **max** (Trend)
- **md** (Trend)
- **medprice** (Trend)
- **min** (Trend)
- **mom** (Momentum)
- **morning_doji_star** (PriceAction)
- **morning_star** (PriceAction)
- **msw** (Momentum)
- **mul** (Math)
- **natr** (Volatility)
- **negative_one** (Scalar)
- **neq** (Utility)
- **null** (Scalar)
- **number** (Scalar)
- **nvi** (Volume)
- **one** (Scalar)
- **parkinson** (Volatility)
- **pennant** (PriceAction)
- **percentile_select** (ControlFlow)
- **phi** (Scalar)
- **pi** (Scalar)
- **pivot_point_sr** (PriceAction)
- **ppo** (Momentum)
- **previous_eq** (Math)
- **previous_gt** (Math)
- **previous_gte** (Math)
- **previous_high_low** (PriceAction)
- **previous_lt** (Math)
- **previous_lte** (Math)
- **previous_neq** (Math)
- **price_diff_vol** (Volatility)
- **price_distance** (Volatility)
- **psar** (Momentum)
- **psl** (PriceAction)
- **pvi** (Volume)
- **qqe** (Momentum)
- **qstick** (PriceAction)
- **quarter** (Statistical)
- **quotes** (DataSource)
- **ratio_branch** (ControlFlow)
- **retracements** (PriceAction)
- **return_vol** (Volatility)
- **rocr** (Momentum)
- **rolling_hurst_exponent** (Momentum)
- **round** (Math)
- **select_2** (ControlFlow)
- **select_3** (ControlFlow)
- **select_4** (ControlFlow)
- **select_5** (ControlFlow)
- **session_time_window** (PriceAction)
- **sin** (Math)
- **sinh** (Math)
- **spinning_top** (PriceAction)
- **sqrt** (Math)
- **sqrt2** (Scalar)
- **sqrt3** (Scalar)
- **sqrt5** (Scalar)
- **star** (PriceAction)
- **stddev** (Math)
- **stderr** (Math)
- **stochrsi** (Momentum)
- **string_case** (Utility)
- **string_check** (Utility)
- **string_contains** (Utility)
- **string_length** (Utility)
- **string_pad** (Utility)
- **string_replace** (Utility)
- **string_reverse** (Utility)
- **string_trim** (Utility)
- **sub** (Math)
- **sum** (Math)
- **tan** (Math)
- **tanh** (Math)
- **tema** (Trend)
- **text** (Scalar)
- **three_black_crows** (PriceAction)
- **three_white_soldiers** (PriceAction)
- **todeg** (Math)
- **top_k** (Momentum)
- **top_k_percent** (Momentum)
- **torad** (Math)
- **tr** (Volatility)
- **trade_executor_adapter** (ControlFlow)
- **trades** (DataSource)
- **trima** (Trend)
- **trix** (Momentum)
- **trunc** (Math)
- **tsf** (Trend)
- **typprice** (Trend)
- **ultosc** (Momentum)
- **var** (Math)
- **vhf** (Trend)
- **vidya** (Trend)
- **volatility** (Volatility)
- **vortex** (Momentum)
- **vosc** (Volume)
- **vwma** (Trend)
- **wad** (Volume)
- **wcprice** (Trend)
- **week_of_month** (Statistical)
- **white_marubozu** (PriceAction)
- **wilders** (Trend)
- **wma** (Trend)
- **yang_zhang** (Volatility)
- **zero** (Scalar)
- **zlema** (Trend)

---

## Detailed Discrepancies

### `adosc` (docs/epochscript/core-transforms.md:840)

❌ **ERROR**: Option 'fast' documented but not in code
❌ **ERROR**: Option 'slow' documented but not in code
⚠️ **WARNING**: Option 'short_period' exists in code but not documented
⚠️ **WARNING**: Option 'long_period' exists in code but not documented

### `aggregate operators` (docs/epochscript/core-transforms.md:1038)

❌ **ERROR**: Transform 'aggregate operators' is documented but does not exist in the codebase

### `bar_gap` (docs/epochscript/core-transforms.md:1303)

⚠️ **WARNING**: Option 'fill_percent' type mismatch: docs say 'Decimal', code says 'Integer'
❌ **ERROR**: Option 'timeframe' documented but not in code

### `bbands` (docs/epochscript/core-transforms.md:599)

⚠️ **WARNING**: Option 'stddev' type mismatch: docs say 'Decimal', code says 'Integer'

### `comparison operators` (docs/epochscript/core-transforms.md:1012)

❌ **ERROR**: Transform 'comparison operators' is documented but does not exist in the codebase

### `day_of_week` (docs/epochscript/core-transforms.md:1600)

❌ **ERROR**: Option 'target_day' documented but not in code
⚠️ **WARNING**: Option 'weekday' exists in code but not documented
❌ **ERROR**: Output 'is_target_day' documented but not in code
⚠️ **WARNING**: Output 'result' exists in code but not documented

### `event_marker` (docs/epochscript/core-transforms.md:1712)

❌ **ERROR**: Transform 'event_marker' is documented but does not exist in the codebase

### `fair_value_gap` (docs/epochscript/core-transforms.md:1461)

⚠️ **WARNING**: Option 'join_consecutive' exists in code but not documented
❌ **ERROR**: Output 'bullish' documented but not in code
❌ **ERROR**: Output 'bearish' documented but not in code
❌ **ERROR**: Output 'bullish_filled' documented but not in code
❌ **ERROR**: Output 'bearish_filled' documented but not in code
❌ **ERROR**: Output 'fvg_high' documented but not in code
❌ **ERROR**: Output 'fvg_low' documented but not in code
⚠️ **WARNING**: Output 'fvg' exists in code but not documented
⚠️ **WARNING**: Output 'top' exists in code but not documented
⚠️ **WARNING**: Output 'bottom' exists in code but not documented
⚠️ **WARNING**: Output 'mitigated_index' exists in code but not documented

### `gap_report` (docs/epochscript/core-transforms.md:1670)

❌ **ERROR**: Transform 'gap_report' is documented but does not exist in the codebase

### `head_and_shoulders` (docs/epochscript/core-transforms.md:1498)

❌ **ERROR**: Option 'tolerance' documented but not in code
❌ **ERROR**: Option 'min_pattern_bars' documented but not in code
⚠️ **WARNING**: Option 'lookback' exists in code but not documented
⚠️ **WARNING**: Option 'head_ratio_before' exists in code but not documented
⚠️ **WARNING**: Option 'head_ratio_after' exists in code but not documented
⚠️ **WARNING**: Option 'neckline_slope_max' exists in code but not documented
❌ **ERROR**: Output 'neckline' documented but not in code
⚠️ **WARNING**: Output 'neckline_level' exists in code but not documented

### `lag` (docs/epochscript/core-transforms.md:275)

❌ **ERROR**: Option 'periods' documented but not in code
⚠️ **WARNING**: Option 'period' exists in code but not documented

### `macd` (docs/epochscript/core-transforms.md:347)

❌ **ERROR**: Option 'fast' documented but not in code
❌ **ERROR**: Option 'slow' documented but not in code
❌ **ERROR**: Option 'signal' documented but not in code
⚠️ **WARNING**: Option 'short_period' exists in code but not documented
⚠️ **WARNING**: Option 'long_period' exists in code but not documented
⚠️ **WARNING**: Option 'signal_period' exists in code but not documented
❌ **ERROR**: Output 'signal' documented but not in code
⚠️ **WARNING**: Output 'macd_signal' exists in code but not documented
⚠️ **WARNING**: Output 'macd_histogram' exists in code but not documented

### `month_of_year` (docs/epochscript/core-transforms.md:1631)

❌ **ERROR**: Option 'target_month' documented but not in code
⚠️ **WARNING**: Option 'month' exists in code but not documented
❌ **ERROR**: Output 'is_target_month' documented but not in code
⚠️ **WARNING**: Output 'result' exists in code but not documented

### `order_blocks` (docs/epochscript/core-transforms.md:1425)

⚠️ **WARNING**: Option 'close_mitigation' exists in code but not documented
❌ **ERROR**: Output 'bullish' documented but not in code
❌ **ERROR**: Output 'bearish' documented but not in code
❌ **ERROR**: Output 'bullish_ob_high' documented but not in code
❌ **ERROR**: Output 'bullish_ob_low' documented but not in code
❌ **ERROR**: Output 'bearish_ob_high' documented but not in code
❌ **ERROR**: Output 'bearish_ob_low' documented but not in code
⚠️ **WARNING**: Output 'ob' exists in code but not documented
⚠️ **WARNING**: Output 'top' exists in code but not documented
⚠️ **WARNING**: Output 'bottom' exists in code but not documented
⚠️ **WARNING**: Output 'ob_volume' exists in code but not documented
⚠️ **WARNING**: Output 'mitigated_index' exists in code but not documented
⚠️ **WARNING**: Output 'percentage' exists in code but not documented

### `quick reference by use case` (docs/epochscript/core-transforms.md:1828)

❌ **ERROR**: Transform 'quick reference by use case' is documented but does not exist in the codebase

### `session_gap` (docs/epochscript/core-transforms.md:1266)

⚠️ **WARNING**: Option 'fill_percent' type mismatch: docs say 'Decimal', code says 'Integer'
❌ **ERROR**: Option 'timeframe' documented but not in code

### `sessions` (docs/epochscript/core-transforms.md:1342)

⚠️ **WARNING**: Option 'session_type' type mismatch: docs say 'String', code says 'Select'

### `stoch` (docs/epochscript/core-transforms.md:388)

❌ **ERROR**: Option 'k_smooth' documented but not in code
⚠️ **WARNING**: Option 'k_slowing_period' exists in code but not documented

### `swing_highs_lows` (docs/epochscript/core-transforms.md:1385)

❌ **ERROR**: Option 'left_count' documented but not in code
❌ **ERROR**: Option 'right_count' documented but not in code
⚠️ **WARNING**: Option 'swing_length' exists in code but not documented
❌ **ERROR**: Output 'swing_high' documented but not in code
❌ **ERROR**: Output 'swing_low' documented but not in code
❌ **ERROR**: Output 'swing_high_price' documented but not in code
❌ **ERROR**: Output 'swing_low_price' documented but not in code
⚠️ **WARNING**: Output 'high_low' exists in code but not documented
⚠️ **WARNING**: Output 'level' exists in code but not documented

### `table_report` (docs/epochscript/core-transforms.md:1770)

❌ **ERROR**: Transform 'table_report' is documented but does not exist in the codebase

### `transform categories` (docs/epochscript/core-transforms.md:1812)

❌ **ERROR**: Transform 'transform categories' is documented but does not exist in the codebase

### `triangles` (docs/epochscript/core-transforms.md:1530)

❌ **ERROR**: Option 'min_pattern_bars' documented but not in code
❌ **ERROR**: Option 'tolerance' documented but not in code
⚠️ **WARNING**: Option 'lookback' exists in code but not documented
⚠️ **WARNING**: Option 'triangle_type' exists in code but not documented
⚠️ **WARNING**: Option 'r_squared_min' exists in code but not documented
❌ **ERROR**: Output 'ascending' documented but not in code
❌ **ERROR**: Output 'descending' documented but not in code
❌ **ERROR**: Output 'symmetrical' documented but not in code
❌ **ERROR**: Output 'upper_line' documented but not in code
❌ **ERROR**: Output 'lower_line' documented but not in code
❌ **ERROR**: Output 'breakout' documented but not in code
⚠️ **WARNING**: Output 'pattern_detected' exists in code but not documented
⚠️ **WARNING**: Output 'upper_slope' exists in code but not documented
⚠️ **WARNING**: Output 'lower_slope' exists in code but not documented
⚠️ **WARNING**: Output 'triangle_type' exists in code but not documented

### `turn_of_month` (docs/epochscript/core-transforms.md:1570)

❌ **ERROR**: Output 'in_window' documented but not in code
⚠️ **WARNING**: Output 'result' exists in code but not documented

### `volatility (historical volatility)` (docs/epochscript/core-transforms.md:740)

❌ **ERROR**: Transform 'volatility (historical volatility)' is documented but does not exist in the codebase

### `zscore` (docs/epochscript/core-transforms.md:1217)

❌ **ERROR**: Option 'period' documented but not in code
⚠️ **WARNING**: Option 'window' exists in code but not documented
