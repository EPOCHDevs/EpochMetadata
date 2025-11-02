# Language Fundamentals

Complete syntax reference for Epoch script.

---

## Table of Contents

1. [Statements](#statements)
2. [Data Types](#data-types)
3. [Literals](#literals)
4. [Operators](#operators)
5. [Expressions](#expressions)
6. [Critical Limitations](#critical-limitations)
7. [Type System](#type-system)

---

## Statements

Epoch script supports two statement types:

### Assignment Statement

```python
variable = expression
```

**Rules:**
- Variables must start with letter or underscore: `ema_20`, `_temp`, `Signal`
- Can contain letters, numbers, underscores: `fast_ema_12`, `rsi_14`
- Case-sensitive: `Price` ≠ `price`
- **Single assignment only** - reassignment is prohibited

```python
# Valid
close = src.c
fast_ma = ema(period=20)(close)
is_bullish = close > close[1]

# Invalid - reassignment
price = src.c
price = src.o  # ERROR: Variable 'price' already bound
```

### Expression Statement

```python
transform_call()(inputs)
```

**Rules:**
- Only allowed for "sink" transforms (no outputs)
- Must be a function call, not a bare variable

**Valid sinks:**
- `trade_signal_executor` - Execute trades
- `gap_report` - Generate gap analysis report
- `event_marker` - Interactive UI selector

```python
# Valid expression statement
trade_signal_executor()(enter_long=buy_signal)

# Invalid - not a sink
ema(period=20)(src.c)  # ERROR: Must assign to variable
```

---

## Data Types

| Type | Description | Examples |
|------|-------------|----------|
| `Boolean` | True/False | `True`, `False` |
| `Integer` | Whole numbers | `42`, `-5`, `0`, `1000` |
| `Decimal` | Floating-point | `3.14`, `0.5`, `-1.23`, `1.5e-3` |
| `Number` | Integer or Decimal | Any numeric value |
| `String` | Text | `"BULLISH"`, `'AAPL'` |
| `Timestamp` | Date/time | Implicit from data source |
| `Any` | Generic type | Used for flexible inputs |

### Type Compatibility

#### Numbers Mix Freely
```python
int_val = 10
dec_val = 3.14
result = int_val + dec_val  # OK: 13.14 (Decimal)
```

#### Boolean ↔ Number Casting
```python
# Boolean → Number (automatic)
is_up = True
score = is_up + 10  # OK: 11 (True = 1)

# Number → Boolean (automatic)
volume = 1000
has_volume = volume and True  # OK: True (non-zero = True)
```

#### Strings Are Isolated
```python
# String + Number NOT allowed
label = "PRICE"
result = label + 10  # ERROR: incompatible_type_cast

# Strings only for labels
direction = "UP" if bullish else "DOWN"  # Valid
```

---

## Literals

### Numbers

```python
# Integers
count = 42
lookback = 20
negative = -10

# Decimals
ratio = 3.14
threshold = 0.618
small = 1.5e-3  # Scientific notation (0.0015)

# Unary minus
neg_value = -src.c
```

### Booleans

```python
enabled = True
disabled = False
```

### Strings

```python
# Single or double quotes
label = "BULLISH"
symbol = 'AAPL'

# Triple-quoted (for JSON configs)
config = """
{
    "strategy": "momentum",
    "version": "1.0"
}
"""
```

### None

```python
no_value = None
fallback = signal if condition else None
```

### Lists (for transform options)

```python
# Used in certain transform parameters
colors = [0xFF0000, 0x00FF00, 0x0000FF]
labels = ["Buy", "Sell", "Hold"]
```

### Dictionaries (for configurations)

```python
# Used in event_marker and similar
color_map = {
    Success: ["BULLISH", "BUY"],
    Error: ["BEARISH", "SELL"],
    Warning: ["NEUTRAL"]
}
```

---

## Operators

### Arithmetic Operators

```python
# Binary operators
sum = a + b          # Addition
diff = a - b         # Subtraction
prod = a * b         # Multiplication
quot = a / b         # Division
mod = a % b          # Modulo
power = a ** b       # Exponentiation

# Examples
spread = fast_ema - slow_ema
ratio = close / open
percent_change = ((close - open) / open) * 100
distance = sqrt(dx ** 2 + dy ** 2)
```

**Unary operator:**
```python
neg = -a  # Negation (only unary operator supported)
```

### Comparison Operators

```python
greater = a > b
greater_equal = a >= b
less = a < b
less_equal = a <= b
equal = a == b
not_equal = a != b

# Examples
is_up = close > open
is_high_volume = volume >= avg_volume * 1.5
is_oversold = rsi_val < 30
```

**⚠️ Chained comparisons NOT supported:**
```python
# Invalid
valid_range = 30 < rsi_val < 70  # ERROR

# Valid
above_30 = rsi_val > 30
below_70 = rsi_val < 70
valid_range = above_30 and below_70
```

### Logical Operators

```python
# AND - all conditions must be true
both = a and b
all_three = a and b and c

# OR - at least one condition must be true
either = a or b

# NOT - negate condition
opposite = not a

# Examples
strong_buy = (price > ma) and (volume > avg_vol) and (rsi_val < 30)
any_signal = buy_signal or sell_signal
not_bearish = not is_bearish
```

---

## Expressions

### Ternary Conditional

**Syntax:**
```python
result = value_if_true if condition else value_if_false
```

**Examples:**
```python
# Basic
direction = "UP" if bullish else "DOWN"
size = 100 if high_confidence else 50

# Numeric conditionals
adjusted = src.c * 1.1 if needs_adjustment else src.c
capped = 100 if rsi_val > 100 else rsi_val

# Nested (for multi-level logic)
label = (
    "STRONG_BUY" if rsi_val < 20 else (
    "BUY" if rsi_val < 30 else (
    "NEUTRAL" if rsi_val < 70 else "SELL"
    )))

# Signal strength
strength = 3 if strong else (2 if medium else 1)
```

**Better alternative for multi-condition:**
```python
# Instead of deeply nested ternary, use conditional_select
label = conditional_select(
    rsi_val < 20, "STRONG_BUY",
    rsi_val < 30, "BUY",
    rsi_val < 70, "NEUTRAL",
    "SELL"
)
```

### Lag Operator (Historical Data)

Access past values using bracket notation:

```python
# Positive index = look backward
prev_close = src.c[1]      # Previous bar
prev_5 = src.c[5]          # 5 bars ago
week_ago = src.c[5]        # 1 week ago (daily data)

# Use in calculations
price_change = src.c - src.c[1]
momentum = src.c - src.c[10]
rate_of_change = (src.c - src.c[10]) / src.c[10]
```

**⚠️ Restrictions:**
```python
# Zero index NOT allowed
current = src.c[0]  # ERROR: invalid_lag_zero
current = src.c     # Correct

# Float index NOT allowed
lag = src.c[1.5]  # ERROR: invalid_lag_float
lag = src.c[2]    # Correct

# Negative lag NOT allowed
future = src.c[-1]  # ERROR
```

**Constant variables in lag:**
```python
# You CAN use constant variables in subscripts
lookback = 20
price_lag = src.c[lookback]  # OK: Same as src.c[20]

# But NOT in transform options
period = 20
ma = sma(period=period)(src.c)  # ERROR: Must use literal
ma = sma(period=20)(src.c)      # Correct
```

### Attribute Access

Access transform outputs using dot notation:

```python
# Market data
src = market_data_source()
close = src.c      # Close price
high = src.h       # High price
volume = src.v     # Volume

# Indicator outputs
bb = bbands(period=20, stddev=2)(src.c)
upper = bb.bbands_upper
middle = bb.bbands_middle
lower = bb.bbands_lower
```

### Tuple Unpacking (Multi-Output Transforms)

Many transforms return multiple values:

```python
# Bollinger Bands (3 outputs)
lower, middle, upper = bbands(period=20, stddev=2)(src.c)

# MACD (2 outputs)
macd_line, signal_line = macd(fast=12, slow=26, signal=9)(src.c)

# Discard unwanted outputs with underscore
lower, _, upper = bbands(period=20, stddev=2)(src.c)  # Discard middle
_, signal_line = macd(fast=12, slow=26, signal=9)(src.c)  # Discard macd
```

**⚠️ Count must match:**
```python
# 3 outputs, 2 variables = ERROR
lower, upper = bbands(period=20, stddev=2)(src.c)  # ERROR

# Correct
lower, middle, upper = bbands(period=20, stddev=2)(src.c)
lower, _, upper = bbands(period=20, stddev=2)(src.c)
```

### Operator Precedence

From **highest** to **lowest** priority:

1. **Subscript**: `x[1]`
2. **Attribute access**: `src.c`, `bb.bbands_upper`
3. **Function calls**: `ema(period=20)(src.c)`
4. **Exponentiation**: `x ** 2`
5. **Unary negation/not**: `-x`, `not x`
6. **Multiplication/Division/Modulo**: `*`, `/`, `%`
7. **Addition/Subtraction**: `+`, `-`
8. **Comparisons**: `<`, `>`, `<=`, `>=`, `==`, `!=`
9. **Logical AND**: `and`
10. **Logical OR**: `or`
11. **Ternary**: `x if cond else y`

**Use parentheses for clarity:**
```python
# Ambiguous
result = a + b * c  # = a + (b * c)

# Clear
result = (a + b) * c

# Complex
signal = (price > ma) and ((volume > avg * 1.5) or high_volatility)
```

---

## Critical Limitations

### No Control Flow Statements

**❌ If Statements**
```python
# NOT ALLOWED
if price > 100:
    signal = True
else:
    signal = False

# USE INSTEAD
signal = price > 100
# Or: signal = True if price > 100 else False
```

**❌ Loops**
```python
# NOT ALLOWED
for i in range(10):
    calculate_something()

# USE INSTEAD
# Transforms are vectorized - they operate on entire series
```

**❌ Functions/Classes**
```python
# NOT ALLOWED
def my_indicator(period):
    return sma(period=period)(src.c)

# USE INSTEAD
short_ma = sma(period=20)(src.c)
long_ma = sma(period=50)(src.c)
```

### No Reassignment

```python
# NOT ALLOWED
price = src.c
price = src.o  # ERROR

# USE INSTEAD
close_price = src.c
open_price = src.o
```

### No Chained Comparisons

```python
# NOT ALLOWED
valid = 30 < rsi_val < 70

# USE INSTEAD
above_30 = rsi_val > 30
below_70 = rsi_val < 70
valid = above_30 and below_70
```

### Lag Restrictions

```python
# Zero index NOT allowed
current = src.c[0]  # ERROR: invalid_lag_zero
current = src.c     # Correct

# Float index NOT allowed
lag = src.c[1.5]  # ERROR: invalid_lag_float
lag = src.c[2]    # Correct
```

### Multi-Output Expression Context

```python
# NOT ALLOWED - multi-output in expression
bb = bbands(period=20, stddev=2)(src.c)
signal = bb > 100  # ERROR: Which output?

# USE INSTEAD
signal = bb.bbands_upper > 100
```

### Constants Limited to Subscripts

```python
# ALLOWED in subscripts
lookback = 20
lag = src.c[lookback]  # OK

# NOT ALLOWED in options
period = 20
ma = sma(period=period)(src.c)  # ERROR

# NOT ALLOWED in expressions
multiplier = 1.5
threshold = avg * multiplier  # ERROR
```

---

## Type System

### Automatic Type Inference

Types are inferred automatically:

```python
src = market_data_source()
close = src.c              # Decimal
is_high = close > 100      # Boolean
sum = 10 + 20              # Integer
ratio = close / 100        # Decimal
```

### Automatic Casting

The compiler inserts cast nodes when needed:

```python
# Boolean → Number (True = 1, False = 0)
is_bullish = True
score = is_bullish + 1  # Auto-cast: 2

# Number → Boolean (0 = False, non-zero = True)
volume = 500
has_vol = volume and True  # Auto-cast: True
```

### Type Errors

```python
# String + Number incompatible
label = "HIGH"
result = label + 100  # ERROR: incompatible_type_cast

# Wrong option type
ma = ema(period="20")(src.c)  # ERROR: expects Integer, got String
```

---

## Summary

### Key Rules

1. **Single Assignment**: Variables are immutable
2. **No Control Flow**: Use ternary or conditional transforms
3. **No Chained Comparisons**: Break into separate checks
4. **Lag Notation**: Positive integers only, no zero
5. **Tuple Unpacking**: Count must match output count
6. **Constants**: Only work in subscripts, not options/expressions

### Valid Patterns

```python
# Assignment
value = expression

# Ternary
result = true_val if condition else false_val

# Lag
historical = value[1]

# Tuple unpacking
a, b, c = multi_output_transform()(inputs)

# Attribute access
output = result.handle_name

# Function call
result = transform(option=val)(input)
```

### Invalid Patterns

```python
# Reassignment
x = 1
x = 2  # ERROR

# Control flow
if cond:  # ERROR
    pass

# Chained comparison
a < b < c  # ERROR

# Zero lag
value[0]  # ERROR

# Float lag
value[1.5]  # ERROR

# Multi-output in expression
bb = bbands(...)(input)
x = bb > 100  # ERROR
```

---

**Next:** [Transform System →](02-transform-system.md)
