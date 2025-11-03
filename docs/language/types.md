---
page_type: concept
layout: default
order: 4
category: Language
description: Data types, literals, type system, and type compatibility in EpochScript
parent: ./index.md
---

# Types & Literals

EpochScript's type system including built-in types, literals, and type compatibility rules.

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

---

## Type Compatibility

### Numbers Mix Freely

```epochscript
int_val = 10
dec_val = 3.14
result = int_val + dec_val  # 13.14 (Decimal)
```

### Boolean ↔ Number Casting

```epochscript
# Boolean → Number (automatic)
is_up = True
score = is_up + 10  # 11 (True = 1)

# Number → Boolean (automatic)
volume = 1000
has_volume = volume and True  # True (non-zero = True)
```

### Strings Are Isolated

Strings can only be used for labels and conditional selection.

:::warning
Cannot mix strings with numbers in arithmetic operations.
:::

**Valid uses:**
```epochscript
direction = "UP" if bullish else "DOWN"
label = conditional_select(rsi < 30, "OVERSOLD", rsi > 70, "OVERBOUGHT", "NEUTRAL")
```

---

## Literals

### Number Literals

```epochscript
# Integers
count = 42
lookback = 20
negative = -10

# Decimals
ratio = 3.14
threshold = 0.618
scientific = 1.5e-3  # 0.0015
```

### Boolean Literals

```epochscript
enabled = True
disabled = False
```

### String Literals

```epochscript
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

### None Literal

```epochscript
no_value = None
fallback = signal if condition else None
```

### List Literals

Used in certain transform parameters:

```epochscript
colors = [0xFF0000, 0x00FF00, 0x0000FF]
labels = ["Buy", "Sell", "Hold"]
```

---

## Type System

### Automatic Type Inference

Types are inferred automatically:

```epochscript
price = src.c          # Inferred: Decimal series
fast = ema(period=20)(price)  # Inferred: Decimal series
signal = fast > slow   # Inferred: Boolean series
```

### Type Checking

The compiler validates type compatibility:

```epochscript
# Type safe
ratio = fast / slow    # Decimal / Decimal → Decimal
condition = fast > slow  # Decimal > Decimal → Boolean

# Will NOT compile - type mismatch
text = "PRICE"
result = text + 10     # ERROR: String + Number incompatible
```

:::note
Type errors are caught at compile time, before execution.
:::

---

## Parameter Type Validation

When passing options (parameters) to transforms, the type must match exactly what the transform expects.

### Transform Option Types

Each transform option has a specific type requirement:

```epochscript
# Integer option - must be whole number
ema_result = ema(period=20)(src.c)  # ✅ Correct

# ERROR: String passed to Integer option
ema_result = ema(period="20")(src.c)  # ❌ Type mismatch
```

### Common Option Types

| Option Type | Valid Values | Invalid Values |
|-------------|--------------|----------------|
| **Integer** | `20`, `-5`, `0`, `100` | `"20"`, `20.5`, `True` |
| **Decimal** | `0.5`, `3.14`, `1.5e-3` | `"0.5"`, `True` |
| **Boolean** | `True`, `False` | `1`, `0`, `"true"` |
| **String** | `"AAPL"`, `"1D"`, `"London"` | `AAPL` (no quotes), numbers |

### Type Mismatch Examples

```epochscript
# ERROR: String where Integer expected
rsi_val = rsi(period="14")(src.c)

# ERROR: Decimal where Integer expected
ma = sma(period=20.5)(src.c)

# ERROR: Boolean where String expected
src = market_data_source(timeframe=True)
```

:::warning
**Type Mismatch for Option**

When you see this error, check:
1. Is the value in quotes when it shouldn't be? (`"20"` → `20`)
2. Is the value missing quotes when it should have them? (`1D` → `"1D"`)
3. Does the number have decimals when it should be whole? (`20.5` → `20`)
4. Are you passing a variable of the wrong type?
:::

### Boolean Operation Type Casting

EpochScript automatically casts between booleans and numbers in some contexts:

```epochscript
# Boolean → Number (automatic)
is_up = True
score = is_up + 10  # ✅ 11 (True = 1, False = 0)

# Number → Boolean (automatic in logical context)
volume = 1000
has_volume = volume and True  # ✅ True (non-zero = True)
```

However, boolean operations expect boolean inputs:

```epochscript
# ✅ Correct: Boolean operands
condition = (price > 100) and (volume > 1000)

# ⚠️ Auto-cast: Non-zero numbers treated as True
result = 5 and 10  # Works, but unclear

# ❌ ERROR: String in boolean operation
result = "text" and True  # Type error
```

:::note
**Best Practice:** Use explicit comparisons for clarity:
- Write `volume > 0` instead of just `volume`
- Write `price == target` instead of relying on auto-casting
:::

---

## Summary

**Key points:**
- Seven built-in types (Boolean, Integer, Decimal, Number, String, Timestamp, Any)
- Automatic type inference from expressions
- Numbers mix freely, booleans cast to/from numbers
- Strings isolated (labels only)
- Type checking at compile time
