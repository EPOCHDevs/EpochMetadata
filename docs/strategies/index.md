---
page_type: overview
layout: grid
order: 2
category: Strategies
description: Comprehensive strategy-building knowledge base - techniques, analysis, patterns, and complete examples
parent: ../index.md
---

# Strategies

Complete knowledge base for building trading strategies - from foundational techniques to complete working examples.

:::grid
[
  {
    "title": "Research Workflows",
    "description": "Research dashboards, signals, tearsheets, and report-driven strategy development",
    "link": "./research-workflows.md",
    "icon": "search",
    "category": "Foundation"
  },
  {
    "title": "Techniques",
    "description": "Conditional logic, dynamic parameters, signal scoring systems",
    "link": "./techniques.md",
    "icon": "code",
    "category": "Foundation"
  },
  {
    "title": "Guidelines & Best Practices",
    "description": "Common pitfalls, session filtering, overfitting, transaction costs",
    "link": "./guidelines.md",
    "icon": "book-open",
    "category": "Foundation"
  },
  {
    "title": "Statistical Analysis",
    "description": "Correlation, regression, Z-scores, rolling windows, statistical transforms",
    "link": "./statistical-analysis.md",
    "icon": "bar-chart",
    "category": "Analysis"
  },
  {
    "title": "Regime Detection",
    "description": "Trending vs ranging markets, volatility regimes, adaptive strategies",
    "link": "./regime-detection.md",
    "icon": "activity",
    "category": "Analysis"
  },
  {
    "title": "Advanced Volatility",
    "description": "ATR, Bollinger, Keltner, volatility estimators, volatility targeting",
    "link": "./advanced-volatility.md",
    "icon": "trending-up",
    "category": "Analysis"
  },
  {
    "title": "Advanced Momentum",
    "description": "Rate of change, momentum oscillators, trend strength indicators",
    "link": "./advanced-momentum.md",
    "icon": "arrow-up",
    "category": "Analysis"
  },
  {
    "title": "Volume & Order Flow",
    "description": "Volume profile, VWAP, OBV, order flow analysis",
    "link": "./volume-order-flow.md",
    "icon": "bar-chart-2",
    "category": "Analysis"
  },
  {
    "title": "ICT Smart Money Concepts",
    "description": "Order blocks, fair value gaps, liquidity voids, kill zones",
    "link": "./ict-smart-money.md",
    "icon": "layers",
    "category": "Patterns"
  },
  {
    "title": "Candlestick Patterns",
    "description": "Doji, hammer, engulfing, harami, morning/evening star patterns",
    "link": "./candlestick-patterns.md",
    "icon": "bar-chart",
    "category": "Patterns"
  },
  {
    "title": "Chart Formations",
    "description": "Head & shoulders, triangles, flags, wedges, double tops/bottoms",
    "link": "./chart-formations.md",
    "icon": "trending-up",
    "category": "Patterns"
  },
  {
    "title": "Cross-Sectional Ranking",
    "description": "Multi-asset momentum, factor models, top K selection",
    "link": "./cross-sectional-ranking.md",
    "icon": "grid",
    "category": "Patterns"
  },
  {
    "title": "Fundamental Reference",
    "description": "Comprehensive fundamental data integration - earnings, P/E, balance sheet, insider trading, 13F",
    "link": "./fundamental-reference.md",
    "icon": "briefcase",
    "category": "Integration"
  },
  {
    "title": "Calendrical Effects",
    "description": "Turn-of-month, January effect, day-of-week, seasonal patterns",
    "link": "./calendrical-effects.md",
    "icon": "calendar",
    "category": "Integration"
  },
  {
    "title": "Strategy Patterns",
    "description": "20+ complete strategy examples - technical, quantitative, intraday, fundamental",
    "link": "./patterns/index.md",
    "icon": "layers",
    "category": "Examples"
  }
]
:::

---

## Documentation Structure

This section is organized into complementary layers:

**Root-level files:** Comprehensive technique references and analysis methods
- Foundation: Core techniques (research, conditional logic, best practices)
- Analysis: Deep dives into specific indicator categories (statistical, regime, volatility, momentum, volume)
- Patterns: Pattern detection systems (ICT/SMC, candlesticks, chart formations, cross-sectional)
- Integration: Data integration methods (fundamentals, calendar effects)

**patterns/ folder:** Complete working strategy examples
- Ready-to-use strategy blueprints
- Technical, quantitative, intraday, fundamental, and combination patterns
- Practical implementations using techniques from root-level references

**Usage:** Study root-level files to understand specific techniques in depth, then see patterns/ folder for practical implementations combining multiple techniques.
