# Troubleshooting: SMA Crossover Card Example

## Original Code (With Errors)

```python
src=market_data_source()

short_ma=ema(period=15)(src.c)
long_ma=ema(period=100)(src.c)

bullish = crossover(short_ma,long_ma)
bearish = crossover(long_ma,short_ma)
new_signal = bullish or bearish

trade_signal_executor()(enter_long=bullish, enter_short=bearish)

color = "BULLISH" if bullish else ("BEARISH" if bearish else None)

card_selector_filter(card_schema=CardSchemaFilter(
  title="SMA Crossover",
  icon=Signal,
  select_keys="new_signal#result",  # ❌ Error 1: should be select_key (singular)
  schemas=[
      CardColumnSchema(
          column_id="color#result",
          slot=Text,  # ❌ Error 2: Text is not a valid CardSlot
          render_type=Hero,  # ❌ Error 3: Hero is not a valid CardRenderType
          color_map={Success: "BULLISH", Error: "BEARISH"}  # ❌ Error 4: dict literals not supported yet
      ),
      CardColumnSchema(column_id="long_ma#result", slot=Subtitle, render_type=Decimal),
      CardColumnSchema(column_id="short_ma#result", slot=Subtitle, render_type=Decimal)
  ]))(new_signal, color, long_ma, short_ma)
```

## Error Message
```
Unsupported expression type: dictionary (line 20, col 89)
```

## Issues Identified

### 1. Dictionary Literals Not Yet Supported
**Issue**: The AST parser doesn't currently support Python dictionary literal expressions `{key: value}`.

**Workaround**: For now, omit the `color_map` parameter (it's optional). Dictionary support will be added in a future update.

### 2. Wrong Parameter Name
**Issue**: `select_keys` should be `select_key` (singular)

**Fix**: Change to `select_key="new_signal#result"`

### 3. Swapped enum values
**Issue**: `slot=Text` and `render_type=Hero` - these are swapped

**Fix**:
- `slot` should be a `CardSlot` enum value: `Hero`, `Subtitle`, `Footer`, etc.
- `render_type` should be a `CardRenderType` enum value: `Text`, `Decimal`, `Boolean`, etc.

### 4. color_map value format
**Issue**: `color_map` values should be arrays of strings, not single strings

**Fix**: `{Success: ["BULLISH"], Error: ["BEARISH"]}` (when dict support is added)

---

## Corrected Code (Version 1 - Without color_map)

```python
src = market_data_source()

short_ma = ema(period=15)(src.c)
long_ma = ema(period=100)(src.c)

bullish = crossover(short_ma, long_ma)
bearish = crossover(long_ma, short_ma)
new_signal = bullish or bearish

trade_signal_executor()(enter_long=bullish, enter_short=bearish)

color = "BULLISH" if bullish else ("BEARISH" if bearish else None)

card_selector_filter(
    card_schema=CardSchemaFilter(
        title="SMA Crossover",
        icon=Signal,
        select_key="new_signal#result",  # ✅ Fixed: singular
        schemas=[
            CardColumnSchema(
                column_id="color#result",
                slot=Hero,  # ✅ Fixed: valid CardSlot
                render_type=Text  # ✅ Fixed: valid CardRenderType
                # color_map omitted for now
            ),
            CardColumnSchema(
                column_id="long_ma#result",
                slot=Subtitle,
                render_type=Decimal
            ),
            CardColumnSchema(
                column_id="short_ma#result",
                slot=Subtitle,
                render_type=Decimal
            )
        ]
    )
)(new_signal, color, long_ma, short_ma)
```

---

## Alternative Approach: Using JSON String (Current Workaround)

Until dictionary literal support is added, you can use the JSON string format for `card_schema`:

```python
src = market_data_source()

short_ma = ema(period=15)(src.c)
long_ma = ema(period=100)(src.c)

bullish = crossover(short_ma, long_ma)
bearish = crossover(long_ma, short_ma)
new_signal = bullish or bearish

trade_signal_executor()(enter_long=bullish, enter_short=bearish)

color = "BULLISH" if bullish else ("BEARISH" if bearish else None)

card_selector_filter(
    card_schema="""{
        "title": "SMA Crossover",
        "icon": "Signal",
        "select_key": "new_signal#result",
        "schemas": [
            {
                "column_id": "color#result",
                "slot": "Hero",
                "render_type": "Text",
                "color_map": {
                    "Success": ["BULLISH"],
                    "Error": ["BEARISH"]
                }
            },
            {
                "column_id": "long_ma#result",
                "slot": "Subtitle",
                "render_type": "Decimal"
            },
            {
                "column_id": "short_ma#result",
                "slot": "Subtitle",
                "render_type": "Decimal"
            }
        ]
    }"""
)(new_signal, color, long_ma, short_ma)
```

---

## Best Practice: Simplified Version

For the cleanest code right now, simplify the card schema without color_map:

```python
src = market_data_source()

short_ma = ema(period=15)(src.c)
long_ma = ema(period=100)(src.c)

bullish = crossover(short_ma, long_ma)
bearish = crossover(long_ma, short_ma)
new_signal = bullish or bearish

trade_signal_executor()(enter_long=bullish, enter_short=bearish)

# Create signal type indicator
signal_type = "BULLISH" if bullish else ("BEARISH" if bearish else "NONE")

# Simple card selector showing crossover signals
card_selector_filter(
    card_schema=CardSchemaFilter(
        title="EMA Crossover Signals",
        icon=Signal,
        select_key="new_signal#result",
        schemas=[
            CardColumnSchema(
                column_id="signal_type#result",
                slot=Hero,
                render_type=Text
            ),
            CardColumnSchema(
                column_id="short_ma#result",
                slot=Subtitle,
                render_type=Decimal
            ),
            CardColumnSchema(
                column_id="long_ma#result",
                slot=Footer,
                render_type=Decimal
            )
        ]
    )
)(new_signal, signal_type, short_ma, long_ma)
```

---

## Quick Reference: Valid Enum Values

### CardSlot
- `PrimaryBadge`
- `SecondaryBadge`
- `Hero`
- `Subtitle`
- `Footer`
- `Details`

### CardRenderType
- `Text`
- `Integer`
- `Decimal`
- `Percent`
- `Monetary`
- `Duration`
- `Badge`
- `Timestamp`
- `Boolean`

### CardIcon
- `Chart`
- `Gap`
- `Signal`
- `Trade`
- `Position`
- `Alert`
- `TrendUp`
- `TrendDown`
- `Calendar`
- `Dollar`
- `Candle`
- `Info`

### CardColor (for color_map)
- `Success`
- `Error`
- `Warning`
- `Info`
- `Primary`
- `Default`

---

## Status: Dictionary Literal Support

**Current Status**: 🚧 Not yet implemented

**Workarounds**:
1. Omit optional parameters that require dictionaries (like `color_map`)
2. Use JSON string format for the entire `card_schema` parameter
3. Wait for dictionary literal support in the AST parser

**Future Support**: The development team is aware of this limitation and will add dictionary literal support in a future update to enable inline `color_map` configuration.
