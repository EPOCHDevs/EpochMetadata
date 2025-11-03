---
page_type: concept
layout: default
order: 3
category: Language
description: EpochScript statements, expressions, variables, and assignment rules
parent: ./index.md
---

# Syntax

EpochScript syntax fundamentals including statements, expressions, and variable assignment.

---

## Statements

Epoch script supports two statement types:

### Assignment Statement

```epochscript
variable = expression
```

**Rules:**
- Variables must start with letter or underscore: `ema_20`, `_temp`, `Signal`
- Can contain letters, numbers, underscores: `fast_ema_12`, `rsi_14`
- Case-sensitive: `Price` ≠ `price`
- **Single assignment only** - reassignment is prohibited

**Example:**

```epochscript
# Valid
close = src.c
fast_ma = ema(period=20)(close)
is_bullish = close > close[1]
```

:::warning
Variables cannot be reassigned. Each variable can only be assigned once.
```epochscript
price = src.c
price = src.o  # ERROR: Variable 'price' already bound
```
:::

### Expression Statement

```epochscript
transform_call()(inputs)
```

**Rules:**
- Only allowed for transforms with **no outputs** (sink nodes)
- Must be a function call, not a bare variable
- Any transform that produces no output values can be used this way

**Common sink categories:**
- **Executors**: `trade_signal_executor`, `portfolio_rebalancer`
- **Reports**: `gap_report`, `table_report`, `lines_chart`
- **Event Markers**: `event_marker`, `card_selector`

**Examples:**

```epochscript
# Valid expression statements (all have no outputs)
trade_signal_executor()(enter_long=buy_signal)
gap_report(card_schema="...")(data=analysis_df)
event_marker(label="Signal Fired")(timestamp=signal_time)
```

:::warning
Transforms with outputs must be assigned to a variable.
```epochscript
ema(period=20)(src.c)  # ERROR: Must assign to variable
```
:::

---

## Expressions

Expressions produce values and can be used anywhere a value is expected.

### Valid Expression Contexts

1. **Right-hand side of assignment**:
   ```epochscript
   result = 2 + 2
   ```

2. **Function arguments**:
   ```epochscript
   ema(period=20)(close + 10)
   ```

3. **Nested expressions**:
   ```epochscript
   result = (fast_ma - slow_ma) / atr_value
   ```

### Expression Types

- **Literals**: `42`, `3.14`, `"BULLISH"`, `True`
- **Variables**: `close`, `rsi_val`, `my_signal`
- **Binary operations**: `a + b`, `x > y`, `p and q`
- **Unary operations**: `-x`, `not signal`
- **Function calls**: `ema(period=20)(close)`
- **Attribute access**: `src.c`, `bb.bbands_upper`
- **Lag operator**: `close[1]`, `price[5]`
- **Ternary**: `"BUY" if rsi < 30 else "SELL"`

---

## Comments

```epochscript
# Single-line comments start with #

close = src.c  # Inline comments also work
```

:::note
Multi-line comments (triple quotes) are not supported.
:::

---

## Variable Naming

### Valid Names

```epochscript
ema_20 = ema(period=20)(close)
fastMA = ema(period=12)(close)
_temp = close - close[1]
Signal = fast > slow
RSI_14 = rsi(period=14)(close)
```

:::note
**Reserved Keywords**: EpochScript respects Python's reserved keywords - they cannot be used as variable names. Examples: `True`, `False`, `None`, `and`, `or`, `not`, `if`, `else`, `in`, `is`, etc.
:::

---

## Summary

**Key principles:**
- Two statement types: assignment and expression statements
- Variables are immutable (single assignment)
- Expressions can be nested and composed
- Only transforms with no outputs can be standalone statements
