# Test Cases Categorization Proposal

## Proposed Directory Structure

```
test_cases/
├── basic/                          # Basic language features
│   ├── basic_success
│   ├── simple_literal
│   ├── simple_operator
│   └── zero_input_components
│
├── operators/                      # Operator tests
│   ├── binary_operators
│   ├── unary_operators
│   ├── assoc_ops
│   ├── operators_comprehensive
│   ├── precedence_ops
│   ├── precedence_complex
│   └── chained_bool
│
├── constants/                      # Constant folding & evaluation
│   ├── constant_arithmetic_basic
│   ├── constant_boolean_ops
│   ├── constant_comparison_ops
│   ├── constant_inline_arithmetic
│   ├── constant_mixed_types
│   ├── constant_operators_all
│   ├── constant_precedence
│   ├── constant_subscript
│   ├── constant_subscript_arithmetic
│   ├── constant_subscript_complex
│   ├── constant_subscript_multiple
│   ├── constant_subscript_negative
│   └── constant_unary_negative
│
├── literals/                       # Literal values
│   ├── mixed_literals
│   ├── mixed_numeric_literals
│   ├── none_literals
│   ├── string_literals
│   ├── empty_string_params
│   └── triple_quoted_json
│
├── variables/                      # Variable resolution
│   ├── variable_resolution_simple
│   ├── variable_resolution_complex
│   ├── variable_resolution_multiple
│   └── variable_resolution_numeric
│
├── control_flow/                   # Conditionals & selection
│   ├── conditional_select_valid
│   ├── ternary_select
│   ├── nested_ternaries
│   └── boolean_branch (if exists)
│
├── tuples/                         # Tuple handling
│   ├── tuple_outputs
│   ├── tuple_chain_ops
│   ├── multiple_underscore_unpacking
│   └── deferred_tuple_unpacking
│
├── parameters/                     # Parameter handling
│   ├── bare_identifier_params
│   ├── duplicate_params
│   ├── inline_calls_mixed
│   ├── shorthand_inline_syntax
│   └── dict_color_map_example
│
├── type_system/                    # Type checking & casting
│   ├── type_casting_compatible
│   ├── type_casting_bool_to_num
│   ├── type_casting_num_to_bool
│   ├── metadata_type_boolean
│   ├── metadata_type_decimal
│   ├── metadata_type_integer
│   └── metadata_type_select
│
├── transforms/                     # Transform-specific tests
│   ├── lead_lag_analysis
│   ├── negative_lag_indices
│   ├── rolling_corr_simple
│   ├── cardschema_filter
│   ├── cardschema_filter_object_construction
│   ├── cardschema_filter_slot_refs
│   └── first_non_null_valid
│
├── graphs/                         # Graph topology & connections
│   ├── multi_sinks
│   ├── multi_slot_connections
│   ├── deeply_nested_expressions
│   └── ambiguous_multi_output
│
├── timeframes/                     # Timeframe handling
│   ├── timeframe_variations
│   └── self_contained_datasource
│
├── strategies/                     # Strategy examples
│   ├── strategy_ma_rsi
│   ├── strategy_macd_gate
│   ├── strategy_bbands_breakout
│   ├── insider_trading_signals
│   └── form13f_smart_money
│
├── reports/                        # Report generation
│   ├── reporter_no_output_usage
│   └── sma_crossover_with_dict
│
├── runtime/                        # Full integration (script + data → output)
│   ├── bull_patterns
│   ├── gap_analysis
│   └── rsi_report
│
├── errors/                         # Error/negative tests
│   ├── conditional_select_insufficient_inputs
│   ├── conditional_select_invalid_condition_type
│   ├── constant_error_float_index
│   ├── constant_error_non_constant
│   ├── first_non_null_no_inputs
│   ├── incompatible_type_cast
│   ├── invalid_attribute_assignment
│   ├── invalid_lag_float
│   ├── invalid_lag_zero
│   ├── invalid_option_type_mismatch
│   ├── invalid_option_unknown
│   ├── invalid_rebind
│   ├── invalid_session
│   ├── invalid_timeframe
│   ├── keyword_vs_positional
│   ├── negative_chained_compare
│   ├── negative_control_flow_if
│   ├── negative_unary_plus
│   ├── negative_unknown_component
│   ├── negative_unknown_handle
│   ├── too_many_positional
│   ├── tuple_mismatch
│   └── undefined_variable
│
├── string_operations/              # String handling
│   ├── string_in_expressions
│   └── string_literals
│
├── special/                        # Special/utility directories
│   ├── archived/
│   └── shared_data/
└── expected.json (orphan file - should be removed)

## Benefits

1. **Easier Navigation**: Find related tests quickly
2. **Better Organization**: Logical grouping by feature area
3. **Maintainability**: Add new tests to appropriate category
4. **Discovery**: New developers can understand test coverage
5. **Selective Testing**: Run specific categories (e.g., only error tests)

## Implementation Notes

- Update `LoadIntegrationTestCases()` to recursively scan subdirectories
- Skip special directories: `archived`, `shared_data`, `special`
- Maintain flat test names in output (e.g., "errors/invalid_timeframe")
