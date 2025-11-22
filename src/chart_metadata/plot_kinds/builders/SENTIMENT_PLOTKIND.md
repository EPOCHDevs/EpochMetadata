# Sentiment Plot Kind

## Overview

Custom plot kind for visualizing sentiment analysis results from ML models like FinBERT.

## Implementation

**File:** `sentiment_builder.h`

**Builder Class:** `SentimentBuilder`

**Plot Type:** Multi-line with custom color coding

## Data Mapping

The sentiment plot kind expects two outputs:

1. **sentiment** (String): Sentiment label
   - "positive" → Green color
   - "neutral" → Yellow/Gray color
   - "negative" → Red color

2. **score** (Decimal): Confidence score (0.0 to 1.0)
   - Plotted as the primary value
   - Higher values = higher confidence

## Visualization

**Display:**
- **Separate panel** below main price chart (`RequiresOwnAxis = true`)
- **Z-Index:** 5 (same as other panel indicators)
- **Value range:** 0.0 to 1.0 (confidence score)
- **Color coding:** Based on sentiment label

**Visual Elements:**
- Line/area chart showing confidence score over time
- Color changes based on sentiment label
- Optional: Horizontal reference lines at 0.5 (neutral threshold)

## Usage Example

```cpp
// In transform metadata
.plotKind = epoch_core::TransformPlotKind::sentiment

// Transform must have these outputs:
.outputs = {
  {IODataType::String, "sentiment", "Sentiment Label"},
  {IODataType::Decimal, "score", "Confidence Score[0-1]"}
}
```

## Registration

```cpp
// In registry.cpp
Register(PK::sentiment, std::make_unique<SentimentBuilder>());
```

## Validation

The builder validates that both required outputs exist:
- `sentiment` output must be present (String type)
- `score` output must be present (Decimal type)

If either is missing, throws runtime error.

## Frontend Integration

The frontend can use the mapped data to:
1. Plot the score as a line/area chart
2. Apply color gradients based on sentiment label:
   - `sentiment === "positive"` → Green (#00C853)
   - `sentiment === "neutral"` → Gray (#9E9E9E)
   - `sentiment === "negative"` → Red (#D32F2F)
3. Add hover tooltips showing label + score
4. Display in dedicated panel with fixed 0-1 Y-axis range

## Benefits Over panel_line

1. **Semantic meaning**: Explicitly designed for sentiment data
2. **Color coding**: Frontend can apply sentiment-specific colors
3. **Validation**: Ensures both sentiment and score outputs exist
4. **Clarity**: Makes it clear this is sentiment data, not generic metrics

## Future Enhancements

- Add threshold lines for confidence levels
- Support for multi-class sentiment (beyond positive/neutral/negative)
- Aggregate sentiment visualization (e.g., sentiment heatmap)
- Sentiment trend indicators (increasing/decreasing positivity)
