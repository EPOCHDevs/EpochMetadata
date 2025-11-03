---
page_type: overview
layout: grid
order: 2
category: Language
description: Complete language reference for EpochScript syntax, types, operators, and limitations
parent: ../index.md
---

# Language Fundamentals

Complete language reference for EpochScript syntax, types, operators, and critical limitations.

## Quick Start

Here's a simple moving average crossover strategy to get you started:

```epochscript
# Get market data
src = market_data_source()

# Calculate indicators
fast = ema(period=20)(src.c)
slow = ema(period=50)(src.c)

# Generate signals
buy = crossover(fast, slow)
sell = crossover(slow, fast)

# Execute trades
trade_signal_executor()(enter_long=buy, enter_short=sell)
```

**Key concepts**:
- `market_data_source()` - Access price data (c=close, h=high, l=low, v=volume)
- `ema(period=20)` - Transform constructor (sets parameters)
- `(src.c)` - Transform application (applies to data)
- `crossover()` - Detects when first series crosses above second
- `trade_signal_executor()` - Required for all trading strategies

:::tip
EpochScript is declarative - you describe *what* you want, not *how* to compute it. No loops, no if statements, variables are immutable.
:::

Choose a topic below for detailed information:

:::grid
[
  {
    "title": "Syntax",
    "description": "Statements, expressions, variables, and assignment rules",
    "link": "./syntax.md",
    "icon": "code",
    "category": "Fundamentals"
  },
  {
    "title": "Types & Literals",
    "description": "Data types, literals, type system, and type compatibility",
    "link": "./types.md",
    "icon": "package",
    "category": "Fundamentals"
  },
  {
    "title": "Operators",
    "description": "Arithmetic, comparison, logical, and lag operators",
    "link": "./operators.md",
    "icon": "settings",
    "category": "Fundamentals"
  },
  {
    "title": "Limitations",
    "description": "Python features not supported and design constraints",
    "link": "./limitations.md",
    "icon": "alert-circle",
    "category": "Important"
  }
]
:::
