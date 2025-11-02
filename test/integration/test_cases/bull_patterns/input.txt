src = market_data_source()

lookback_period = 20
flagpole_gain = (src.c - src.c[lookback_period]) / src.c[lookback_period]
flagpole_magnitude = abs(flagpole_gain)
flagpole_steepness = flagpole_magnitude / lookback_period

consolidation_vol_avg = sma(period=10)(src.v)
flagpole_vol_avg = sma(period=10)(src.v[20])
volume_decline = (flagpole_vol_avg - consolidation_vol_avg) / flagpole_vol_avg

flagpole_score = flagpole_magnitude * 100.0
steepness_score = flagpole_steepness * 500.0
volume_score = volume_decline * 50.0

quality_score = flagpole_score + steepness_score + volume_score

top_20_stocks = top_k(k=20)(quality_score)

filtered_score = quality_score if top_20_stocks else 0.0

table_report(
    sql="SELECT SLOT0 as RESULT_Quality_Score, SLOT1 as RESULT_Flagpole_Score, SLOT2 as RESULT_Steepness_Score, SLOT3 as RESULT_Volume_Score FROM self WHERE SLOT4 = True ORDER BY SLOT0 DESC",
    add_index=True,
    title="Top 20 Bull Flag Patterns by Quality Score"
)(quality_score, flagpole_score, steepness_score, volume_score, top_20_stocks)