# Shared Test Data

This directory contains reusable CSV datasets that can be referenced by multiple test cases.

## Available Datasets

### Stock Data (Daily)
- `1D_AAPL-Stocks-2024H1.csv` - Apple Inc. daily OHLCV data (Jan-Jun 2024)
- `1D_GOOGL-Stocks-2024H1.csv` - Alphabet Inc. daily OHLCV data (Jan-Jun 2024)
- `1D_MSFT-Stocks-2024H1.csv` - Microsoft Corp. daily OHLCV data (Jan-Jun 2024)

### FX Data (Minute)
- `1Min_EURUSD-FX-2024Q1.csv` - EUR/USD minute-level data (Q1 2024)

## CSV Format

All CSV files follow the standard OHLCV format:
```csv
index,o,h,l,c,vw,n
2024-01-02T00:00:00,185.50,186.25,184.10,185.80,185.40,52000000
```

Where:
- `index`: Timestamp (ISO 8601 format)
- `o`: Open price
- `h`: High price
- `l`: Low price
- `c`: Close price
- `vw`: Volume-weighted price
- `n`: Number of trades / volume

## Usage

Test cases can reference shared data in two ways:

1. **Direct Copy**: Copy shared CSV files into test's `input_data/` directory
2. **Symlink**: Create symlinks from test's `input_data/` to shared files
3. **Config Reference** (future): Use `config.yaml` to specify shared data refs

Example structure:
```
bull_patterns/
├── input.txt
├── input_data/
│   ├── 1D_AAPL-Stocks.csv -> ../../shared_data/1D_AAPL-Stocks-2024H1.csv
│   ├── 1D_GOOGL-Stocks.csv
│   └── 1D_MSFT-Stocks.csv
└── expected/
    └── graph.json
```
