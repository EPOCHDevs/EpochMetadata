---
page_type: reference
layout: default
order: 16
category: Strategies
description: Fundamental data integration - Balance Sheet, Income Statement, Cash Flow, 13F, Insider Trading
parent: ./index.md
---

# Fundamental Data Integration

Combine fundamental metrics with technical signals for quality screening and smart-money tracking.

---

## Quality Filters

```epochscript
bs = balance_sheet()
income = income_statement()
cf = cash_flow()

# Balance sheet quality
low_debt = bs.debt_to_equity < 0.7
liquid = bs.current_ratio > 1.3

# Profitability
profitable = income.net_income > 0
high_roe = income.roe > 0.15

# Cash generation
positive_fcf = (cf.operating_cash_flow - cf.capex) > 0

# Valuation
pe = src.c / income.eps
reasonable_pe = pe < 25 and pe > 5

# Combined quality filter
fundamentals_ok = low_debt and liquid and profitable and high_roe and positive_fcf and reasonable_pe

# Only trade quality companies
technical_signal = crossover(ema(period=20)(src.c), ema(period=50)(src.c))
entry = technical_signal and fundamentals_ok
```

---

## Insider Trading (Smart Money)

```epochscript
insider = insider_trading(
    filing_type="4",
    transaction_code="P",  # P = open market purchase
    min_value=100000
)

# Cluster buying (multiple insiders)
insider_buy = insider.transaction_code == "P"
buy_count = sum(period=30)(conditional_select(insider_buy, 1, 0))
cluster = buy_count >= 3  # 3+ buys in 30 days

# Large executive purchases
large_buy = (insider.shares * insider.price) > 500000
exec_buy = large_buy  # Can filter by insider.owner_name if needed

catalyst = cluster or exec_buy

# Technical + catalyst
technical_signal = src.c > sma(period=50)(src.c)
entry = technical_signal and catalyst
```

**Transaction codes:** `P` = purchase (bullish), `S` = sale (often diversification, not bearish), `A` = award/grant (neutral), `M` = exercise.

---

## Institutional Holdings (Form 13F)

```epochscript
brk_holdings = form13f_holdings(
    institution_cik="1067983",  # Berkshire Hathaway
    min_value=1000000000  # $1B+ positions
)

shares_now = brk_holdings.shares
shares_prev = brk_holdings.shares[90]  # ~1 quarter ago

position_increased = shares_now > shares_prev * 1.1  # 10%+ increase
new_position = (shares_prev == 0) and (shares_now > 0)

bullish_13f = position_increased or new_position

technical_signal = rsi(period=14)(src.c) < 50
entry = bullish_13f and technical_signal
```

**Popular CIKs:** 1067983 = Berkshire, 1324404 = Citadel, 1649506 = Renaissance.

---

## Macro Overlay (Economic Indicators)

```epochscript
fed_funds = economic_indicator(category="FedFunds")
vix = economic_indicator(category="VIX")

# Risk-on environment
low_rates = fed_funds.value < 2.0
falling_rates = fed_funds.value < fed_funds.value[30]

low_fear = vix.value < 15
rising_fear = vix.value > vix.value[5]

risk_on = low_rates and low_fear
risk_off = rising_fear or (vix.value > 30)

# Only trade in risk-on
technical_signal = crossover(ema(period=20)(src.c), ema(period=50)(src.c))
macro_ok = risk_on

entry = technical_signal and macro_ok
exit = risk_off
```

---

## Complete Fundamental + Technical Fusion

```epochscript
# Fundamentals
bs = balance_sheet()
income = income_statement()
cf = cash_flow()

low_debt = bs.debt_to_equity < 0.7
profitable = income.net_income > 0
high_roe = income.roe > 0.15
positive_fcf = (cf.operating_cash_flow - cf.capex) > 0
pe = src.c / income.eps
reasonable_pe = pe < 25 and pe > 5

fundamentals_strong = low_debt and profitable and high_roe and positive_fcf and reasonable_pe

# Smart money
insider = insider_trading(filing_type="4", transaction_code="P")
insider_buying = sum(period=30)(conditional_select(insider.transaction_code == "P", 1, 0)) >= 2

# Macro
vix = economic_indicator(category="VIX")
macro_ok = vix.value < 20

# Technical
uptrend = ema(period=50)(src.c) > ema(period=200)(src.c)
pullback = src.c < ema(period=20)(src.c)
rsi_val = rsi(period=14)(src.c)
not_overbought = rsi_val < 70

# Combined entry
entry = (
    fundamentals_strong and
    insider_buying and
    macro_ok and
    uptrend and
    pullback and
    not_overbought
)

# Exit on deterioration
exit = (not fundamentals_strong) or (vix.value > 30)
```

**Next:** [Calendrical Effects →](./calendrical-effects.md)
