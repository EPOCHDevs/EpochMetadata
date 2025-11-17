# Investigation tests for debugging specific issues
target_sources(epoch_script_test PRIVATE
    bar_chart_null_test.cpp
    fill_null_test.cpp
    index_aggregation_bug_test.cpp
    tulip_indicator_defaults_test.cpp
)