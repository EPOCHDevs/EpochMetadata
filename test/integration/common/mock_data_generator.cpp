#include "mock_data_generator.h"

#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <spdlog/spdlog.h>
#include <regex>
#include <arrow/table.h>
#include <arrow/array/builder_primitive.h>

#include "csv_data_loader.h"
#include "epoch_frame/factory/dataframe_factory.h"
#include "epoch_frame/factory/index_factory.h"

namespace epoch_script::runtime::test {

using namespace std::chrono;

// Generate deterministic seed from string (FNV-1a hash)
uint64_t MockDataGenerator::GenerateSeed(const std::string& input) {
    uint64_t hash = 14695981039346656037ULL;
    for (char c : input) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

epoch_frame::DataFrame MockDataGenerator::GenerateData(const GenerationConfig& config) {
    SPDLOG_DEBUG("Generating mock data for {}: {} bars of {} (type: {})",
                 config.ticker, config.num_bars, config.timeframe,
                 static_cast<int>(config.data_source));

    // Dispatch to appropriate generator based on data source type
    switch (config.data_source) {
        case DataSourceType::MarketData:
        case DataSourceType::MarketIndices:
            return GenerateMarketData(config);
        case DataSourceType::FRED:
            return GenerateFREDData(config);
        case DataSourceType::BalanceSheet:
            return GenerateBalanceSheetData(config);
        case DataSourceType::IncomeStatement:
            return GenerateIncomeStatementData(config);
        case DataSourceType::Form13F:
            return GenerateForm13FData(config);
        case DataSourceType::InsiderTrading:
            return GenerateInsiderTradingData(config);
        default:
            throw std::runtime_error("Unsupported data source type");
    }
}

epoch_frame::DataFrame MockDataGenerator::GenerateMarketData(const GenerationConfig& config) {
    SPDLOG_DEBUG("Generating market data for {}: {} bars of {}",
                 config.ticker, config.num_bars, config.timeframe);

    // Generate bars
    auto bars = GenerateBars(config);

    // Convert to DataFrame
    std::vector<std::string> timestamps;
    std::vector<double> opens, highs, lows, closes, vwaps;
    std::vector<int64_t> volumes;

    timestamps.reserve(bars.size());
    opens.reserve(bars.size());
    highs.reserve(bars.size());
    lows.reserve(bars.size());
    closes.reserve(bars.size());
    vwaps.reserve(bars.size());
    volumes.reserve(bars.size());

    for (const auto& bar : bars) {
        // Convert timestamp to ISO 8601 string
        auto time_t_val = system_clock::to_time_t(bar.timestamp);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t_val), "%Y-%m-%dT%H:%M:%S");
        timestamps.push_back(ss.str());

        opens.push_back(bar.open);
        highs.push_back(bar.high);
        lows.push_back(bar.low);
        closes.push_back(bar.close);
        vwaps.push_back(bar.vwap);
        volumes.push_back(static_cast<int64_t>(bar.volume));
    }

    // Create DataFrame
    epoch_frame::DataFrame df;
    df.add_column("index", timestamps);
    df.add_column("o", opens);
    df.add_column("h", highs);
    df.add_column("l", lows);
    df.add_column("c", closes);
    df.add_column("vw", vwaps);
    df.add_column("n", volumes);

    // Set index column
    df = df.set_index("index");

    SPDLOG_DEBUG("Generated {} bars from {} to {}",
                 bars.size(), timestamps.front(), timestamps.back());

    return df;
}

std::vector<MockDataGenerator::OHLCVBar>
MockDataGenerator::GenerateBars(const GenerationConfig& config) {
    // Generate or use provided seed
    uint64_t seed = config.seed;
    if (seed == 0) {
        seed = GenerateSeed(config.ticker + "_" + config.timeframe);
    }

    std::mt19937_64 rng(seed);
    std::normal_distribution<double> price_noise(0.0, config.volatility);
    std::normal_distribution<double> volume_noise(1.0, config.volume_volatility);

    std::vector<OHLCVBar> bars;
    bars.reserve(config.num_bars);

    // Parse start date (format: "YYYY-MM-DD")
    std::tm tm = {};
    std::istringstream ss(config.start_date);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    auto current_time = system_clock::from_time_t(std::mktime(&tm));

    double current_price = config.initial_price;

    for (size_t i = 0; i < config.num_bars; ++i) {
        OHLCVBar bar;
        bar.timestamp = current_time;

        // Calculate daily return with trend
        double trend_component = config.trend_strength * config.volatility;
        double random_component = price_noise(rng);
        double daily_return = trend_component + random_component;

        // Generate OHLC
        bar.open = current_price;

        // Calculate close based on daily return
        bar.close = bar.open * (1.0 + daily_return);

        // Generate high and low based on intraday volatility
        // High is typically higher than both open and close
        // Low is typically lower than both
        double intraday_vol = std::abs(price_noise(rng)) * 0.5;
        double max_oc = std::max(bar.open, bar.close);
        double min_oc = std::min(bar.open, bar.close);

        bar.high = max_oc * (1.0 + intraday_vol);
        bar.low = min_oc * (1.0 - intraday_vol);

        // Ensure OHLC consistency: H >= max(O,C), L <= min(O,C)
        bar.high = std::max({bar.high, bar.open, bar.close});
        bar.low = std::min({bar.low, bar.open, bar.close});

        // Calculate VWAP (volume-weighted average price)
        // Approximate as weighted average of OHLC, closer to close
        bar.vwap = (bar.high + bar.low + 2.0 * bar.close) / 4.0;

        // Generate volume
        double volume_mult = std::abs(volume_noise(rng));
        bar.volume = static_cast<size_t>(config.base_volume * volume_mult);

        // Ensure positive volume
        if (bar.volume == 0) bar.volume = 1;

        bars.push_back(bar);

        // Update for next bar
        current_price = bar.close;
        current_time = IncrementTimestamp(current_time, config.timeframe, config.asset_class);
    }

    // Apply regime-specific patterns
    ApplyRegimePattern(bars, config.regime, rng);

    // Add realistic gaps for daily+ timeframes
    if (config.asset_class == AssetClass::Stock) {
        AddGaps(bars, config.timeframe, rng);
    }

    return bars;
}

void MockDataGenerator::ApplyRegimePattern(std::vector<OHLCVBar>& bars,
                                          MarketRegime regime,
                                          std::mt19937_64& rng) {
    if (regime == MarketRegime::Mixed) {
        return; // Already has natural mixed behavior
    }

    std::uniform_real_distribution<double> factor_dist(0.8, 1.2);

    for (size_t i = 1; i < bars.size(); ++i) {
        auto& bar = bars[i];
        auto& prev = bars[i-1];

        switch (regime) {
            case MarketRegime::Trending:
                // Amplify directional movement
                if (bar.close > bar.open) {
                    // Uptrend: push close higher
                    bar.close *= factor_dist(rng);
                    bar.high = std::max(bar.high, bar.close);
                } else {
                    // Downtrend: push close lower
                    bar.close *= (2.0 - factor_dist(rng));
                    bar.low = std::min(bar.low, bar.close);
                }
                bar.vwap = (bar.high + bar.low + 2.0 * bar.close) / 4.0;
                break;

            case MarketRegime::Ranging:
                // Mean reversion: push toward initial price
                double mean_price = bars[0].open;
                bar.close = bar.close * 0.3 + mean_price * 0.7;
                bar.high = std::max({bar.high * 0.8, bar.open, bar.close});
                bar.low = std::min({bar.low * 1.2, bar.open, bar.close});
                bar.vwap = (bar.high + bar.low + 2.0 * bar.close) / 4.0;
                break;

            case MarketRegime::Volatile:
                // Increase range but no clear direction
                double range_multiplier = factor_dist(rng);
                double midpoint = (bar.high + bar.low) / 2.0;
                bar.high = midpoint + (bar.high - midpoint) * range_multiplier;
                bar.low = midpoint - (midpoint - bar.low) * range_multiplier;
                bar.vwap = (bar.high + bar.low + 2.0 * bar.close) / 4.0;
                break;

            default:
                break;
        }
    }
}

void MockDataGenerator::AddGaps(std::vector<OHLCVBar>& bars,
                               const std::string& timeframe,
                               std::mt19937_64& rng) {
    // Only add gaps to daily+ timeframes
    auto tf_mins = ParseTimeframeMinutes(timeframe);
    if (!tf_mins || *tf_mins < 1440) { // Less than 1 day
        return;
    }

    std::uniform_real_distribution<double> gap_prob(0.0, 1.0);
    std::normal_distribution<double> gap_size(0.0, 0.02); // 2% gap on average

    for (size_t i = 1; i < bars.size(); ++i) {
        // 10% chance of gap
        if (gap_prob(rng) < 0.10) {
            auto& bar = bars[i];
            auto& prev = bars[i-1];

            double gap = gap_size(rng);

            // Apply gap to open
            bar.open = prev.close * (1.0 + gap);

            // Adjust other prices accordingly
            double shift = bar.open - bars[i].open;
            bar.high += shift;
            bar.low += shift;
            bar.close += shift;
            bar.vwap += shift;

            // Ensure OHLC consistency
            bar.high = std::max({bar.high, bar.open, bar.close});
            bar.low = std::min({bar.low, bar.open, bar.close});
        }
    }
}

std::chrono::system_clock::time_point MockDataGenerator::IncrementTimestamp(
    std::chrono::system_clock::time_point current,
    const std::string& timeframe,
    AssetClass asset_class) {

    auto mins = ParseTimeframeMinutes(timeframe);
    if (!mins) {
        throw std::runtime_error("Invalid timeframe: " + timeframe);
    }

    // For daily and above, skip weekends for stocks
    if (*mins >= 1440 && asset_class == AssetClass::Stock) {
        auto next = current + hours(24);
        auto time_t_val = system_clock::to_time_t(next);
        std::tm* tm = std::gmtime(&time_t_val);

        // Skip weekends (0 = Sunday, 6 = Saturday in struct tm)
        int wday = tm->tm_wday;
        if (wday == 6) { // Saturday
            next += hours(48);
        } else if (wday == 0) { // Sunday
            next += hours(24);
        }

        return next;
    }

    // Otherwise just add the timeframe
    return current + minutes(*mins);
}

std::optional<int> MockDataGenerator::ParseTimeframeMinutes(const std::string& timeframe) {
    // Parse timeframe strings: "1D", "1H", "15m", "1Min", etc.
    std::regex tf_regex(R"((\d+)(D|H|h|m|Min|min))");
    std::smatch matches;

    if (!std::regex_match(timeframe, matches, tf_regex)) {
        return std::nullopt;
    }

    int value = std::stoi(matches[1].str());
    std::string unit = matches[2].str();

    if (unit == "D") {
        return value * 1440; // Days to minutes
    } else if (unit == "H" || unit == "h") {
        return value * 60; // Hours to minutes
    } else if (unit == "m" || unit == "Min" || unit == "min") {
        return value; // Already in minutes
    }

    return std::nullopt;
}

std::string MockDataGenerator::GenerateFilename(const GenerationConfig& config) {
    return config.timeframe + "_" + config.ticker + "-" +
           AssetClassToString(config.asset_class) + ".csv";
}

std::string MockDataGenerator::AssetClassToString(AssetClass ac) {
    switch (ac) {
        case AssetClass::Stock: return "Stock";
        case AssetClass::Crypto: return "Crypto";
        case AssetClass::Forex: return "FX";
        case AssetClass::Futures: return "Futures";
        default: return "Unknown";
    }
}

void MockDataGenerator::WriteToCSV(const epoch_frame::DataFrame& data,
                                  const std::filesystem::path& output_path) {
    CsvDataLoader::WriteCsvFile(data, output_path, true);
    SPDLOG_INFO("Wrote mock data to {}", output_path.string());
}

epoch_frame::DataFrame MockDataGenerator::GenerateFREDData(const GenerationConfig& config) {
    // FRED data is typically monthly or quarterly economic indicators
    // Returns sparse data (only on publication dates)

    uint64_t seed = config.seed ? config.seed : GenerateSeed(config.indicator_name);
    std::mt19937_64 rng(seed);
    std::normal_distribution<double> indicator_noise(0.0, 0.01); // 1% noise

    std::vector<std::string> observation_dates;
    std::vector<double> values;

    // Parse start date (format: "YYYY-MM-DD")
    std::tm tm = {};
    std::istringstream ss(config.start_date);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    auto current_time = system_clock::from_time_t(std::mktime(&tm));

    double current_value = config.indicator_base_value;

    // FRED data is typically monthly, so generate monthly data points
    for (size_t i = 0; i < config.num_bars; ++i) {
        // Format date as YYYY-MM-DD
        auto time_t_val = system_clock::to_time_t(current_time);
        std::stringstream date_ss;
        date_ss << std::put_time(std::gmtime(&time_t_val), "%Y-%m-%d");
        observation_dates.push_back(date_ss.str());

        // Generate indicator value with trend
        double change = indicator_noise(rng);
        current_value *= (1.0 + change);
        values.push_back(current_value);

        // Move to next month (approximately 30 days)
        current_time += hours(24 * 30);
    }

    // Create DataFrame
    epoch_frame::DataFrame df;
    df.add_column("index", observation_dates);
    df.add_column("value", values);
    df = df.set_index("index");

    SPDLOG_DEBUG("Generated FRED data: {} {} observations", config.indicator_name, values.size());
    return df;
}

epoch_frame::DataFrame MockDataGenerator::GenerateBalanceSheetData(const GenerationConfig& config) {
    // Balance sheet data is quarterly
    uint64_t seed = config.seed ? config.seed : GenerateSeed(config.ticker + "_balance_sheet");
    std::mt19937_64 rng(seed);
    std::normal_distribution<double> growth(0.02, 0.01); // 2% quarterly growth avg

    std::vector<std::string> period_ends;
    std::vector<std::string> ciks, timeframes;
    std::vector<int> fiscal_years, fiscal_quarters;
    std::vector<double> cash, receivables, inventories, ppe_net;
    std::vector<double> accounts_payable, current_debt, long_term_debt, retained_earnings;
    std::vector<double> accrued_liabilities, deferred_revenue, other_current_assets, other_ltl, aoci;

    double base_assets = config.assets_base;

    // Generate quarterly data
    for (size_t i = 0; i < config.num_bars; ++i) {
        // Calculate fiscal quarter and year
        int year = 2024 + (i / 4);
        int quarter = (i % 4) + 1;

        fiscal_years.push_back(year);
        fiscal_quarters.push_back(quarter);

        // Period end date (end of quarter)
        std::stringstream period_ss;
        int month = quarter * 3; // Q1=Mar, Q2=Jun, Q3=Sep, Q4=Dec
        period_ss << year << "-" << std::setfill('0') << std::setw(2) << month << "-30";
        period_ends.push_back(period_ss.str());

        ciks.push_back("0001234567");  // Mock CIK
        timeframes.push_back("quarterly");

        // Generate balance sheet items (% of total assets)
        double assets = base_assets * (1.0 + growth(rng));
        cash.push_back(assets * 0.15);  // 15% cash
        receivables.push_back(assets * 0.20);  // 20% receivables
        inventories.push_back(assets * 0.10);  // 10% inventory
        ppe_net.push_back(assets * 0.30);  // 30% PP&E
        other_current_assets.push_back(assets * 0.05);

        // Liabilities
        accounts_payable.push_back(assets * 0.12);
        accrued_liabilities.push_back(assets * 0.08);
        current_debt.push_back(assets * 0.05);
        deferred_revenue.push_back(assets * 0.03);
        long_term_debt.push_back(assets * 0.25);
        other_ltl.push_back(assets * 0.07);

        // Equity
        retained_earnings.push_back(assets * 0.25);
        aoci.push_back(assets * 0.02);

        base_assets = assets;
    }

    // Create DataFrame with all balance sheet columns
    epoch_frame::DataFrame df;
    df.add_column("index", period_ends);
    df.add_column("timeframe", timeframes);
    df.add_column("cik", ciks);
    df.add_column("fiscal_year", fiscal_years);
    df.add_column("fiscal_quarter", fiscal_quarters);
    df.add_column("cash", cash);
    df.add_column("receivables", receivables);
    df.add_column("inventories", inventories);
    df.add_column("ppe_net", ppe_net);
    df.add_column("other_current_assets", other_current_assets);
    df.add_column("accounts_payable", accounts_payable);
    df.add_column("accrued_liabilities", accrued_liabilities);
    df.add_column("current_debt", current_debt);
    df.add_column("deferred_revenue", deferred_revenue);
    df.add_column("long_term_debt", long_term_debt);
    df.add_column("other_ltl", other_ltl);
    df.add_column("retained_earnings", retained_earnings);
    df.add_column("aoci", aoci);

    df = df.set_index("index");

    SPDLOG_DEBUG("Generated balance sheet data: {} quarters", period_ends.size());
    return df;
}

epoch_frame::DataFrame MockDataGenerator::GenerateIncomeStatementData(const GenerationConfig& config) {
    // Income statement data is quarterly
    uint64_t seed = config.seed ? config.seed : GenerateSeed(config.ticker + "_income_statement");
    std::mt19937_64 rng(seed);
    std::normal_distribution<double> revenue_growth(0.05, 0.02); // 5% quarterly growth
    std::uniform_real_distribution<double> margin_var(0.95, 1.05); // Margin variation

    std::vector<std::string> period_ends;
    std::vector<std::string> ciks, timeframes;
    std::vector<int> fiscal_years, fiscal_quarters;
    std::vector<double> revenue, cogs, gross_profit, operating_income;
    std::vector<double> rnd, sga, other_opex, ebt, income_tax, net_income;
    std::vector<double> ni_common, basic_eps, diluted_eps, other_income;
    std::vector<int64_t> basic_shares, diluted_shares;

    double base_revenue = config.revenue_base;
    int64_t shares_outstanding = 1000000000; // 1 billion shares

    for (size_t i = 0; i < config.num_bars; ++i) {
        int year = 2024 + (i / 4);
        int quarter = (i % 4) + 1;

        fiscal_years.push_back(year);
        fiscal_quarters.push_back(quarter);

        std::stringstream period_ss;
        int month = quarter * 3;
        period_ss << year << "-" << std::setfill('0') << std::setw(2) << month << "-30";
        period_ends.push_back(period_ss.str());

        ciks.push_back("0001234567");
        timeframes.push_back("quarterly");

        // Generate income statement
        double rev = base_revenue * (1.0 + revenue_growth(rng));
        revenue.push_back(rev);

        double cogs_val = rev * 0.60 * margin_var(rng);  // 60% COGS
        cogs.push_back(cogs_val);

        double gp = rev - cogs_val;
        gross_profit.push_back(gp);

        double rnd_val = rev * 0.10 * margin_var(rng);  // 10% R&D
        rnd.push_back(rnd_val);

        double sga_val = rev * 0.15 * margin_var(rng);  // 15% SG&A
        sga.push_back(sga_val);

        double other_opex_val = rev * 0.02;
        other_opex.push_back(other_opex_val);

        double op_income = gp - rnd_val - sga_val - other_opex_val;
        operating_income.push_back(op_income);

        double other_income_val = rev * 0.01;
        other_income.push_back(other_income_val);

        double ebt_val = op_income + other_income_val;
        ebt.push_back(ebt_val);

        double tax = ebt_val * 0.21;  // 21% tax rate
        income_tax.push_back(tax);

        double ni = ebt_val - tax;
        net_income.push_back(ni);
        ni_common.push_back(ni);

        // EPS
        basic_shares.push_back(shares_outstanding);
        diluted_shares.push_back(static_cast<int64_t>(shares_outstanding * 1.02)); // 2% dilution

        basic_eps.push_back(ni / shares_outstanding);
        diluted_eps.push_back(ni / (shares_outstanding * 1.02));

        base_revenue = rev;
    }

    // Create DataFrame
    epoch_frame::DataFrame df;
    df.add_column("index", period_ends);
    df.add_column("timeframe", timeframes);
    df.add_column("cik", ciks);
    df.add_column("fiscal_year", fiscal_years);
    df.add_column("fiscal_quarter", fiscal_quarters);
    df.add_column("revenue", revenue);
    df.add_column("cogs", cogs);
    df.add_column("gross_profit", gross_profit);
    df.add_column("rnd", rnd);
    df.add_column("sga", sga);
    df.add_column("other_opex", other_opex);
    df.add_column("operating_income", operating_income);
    df.add_column("other_income", other_income);
    df.add_column("ebt", ebt);
    df.add_column("income_tax", income_tax);
    df.add_column("net_income", net_income);
    df.add_column("ni_common", ni_common);
    df.add_column("basic_shares", basic_shares);
    df.add_column("diluted_shares", diluted_shares);
    df.add_column("basic_eps", basic_eps);
    df.add_column("diluted_eps", diluted_eps);

    df = df.set_index("index");

    SPDLOG_DEBUG("Generated income statement data: {} quarters", period_ends.size());
    return df;
}

epoch_frame::DataFrame MockDataGenerator::GenerateForm13FData(const GenerationConfig& config) {
    // Form 13F institutional holdings - quarterly filings
    uint64_t seed = config.seed ? config.seed : GenerateSeed(config.ticker + "_13f");
    std::mt19937_64 rng(seed);

    std::vector<std::string> period_ends, institution_names, security_types, investment_discretions;
    std::vector<double> shares_vals, value_vals;

    // Institution names (mock)
    std::vector<std::string> institutions = {
        "Vanguard Group Inc", "BlackRock Inc", "State Street Corp",
        "Fidelity Investments", "Berkshire Hathaway", "Capital Group",
        "Wellington Management", "Geode Capital", "Northern Trust", "Invesco"
    };

    // Generate quarterly 13F data for multiple institutions
    size_t num_quarters = config.num_bars / config.num_institutions;
    if (num_quarters == 0) num_quarters = 1;

    for (size_t q = 0; q < num_quarters; ++q) {
        int year = 2024 + (q / 4);
        int quarter = (q % 4) + 1;
        int month = quarter * 3;

        std::stringstream period_ss;
        period_ss << year << "-" << std::setfill('0') << std::setw(2) << month << "-30T00:00:00";
        std::string period = period_ss.str();

        // Each quarter, multiple institutions report holdings
        for (size_t i = 0; i < std::min(config.num_institutions, institutions.size()); ++i) {
            period_ends.push_back(period);
            institution_names.push_back(institutions[i]);

            // Generate position size
            std::uniform_real_distribution<double> shares_dist(1000000, 100000000); // 1M to 100M shares
            double shares = shares_dist(rng);
            shares_vals.push_back(shares);

            // Value = shares * ~price
            double price = config.initial_price * (1.0 + 0.1 * q / num_quarters); // Price appreciates over time
            value_vals.push_back(shares * price);

            security_types.push_back("SH"); // Common shares
            investment_discretions.push_back("SOLE"); // Sole voting authority
        }
    }

    // Create DataFrame
    epoch_frame::DataFrame df;
    df.add_column("index", period_ends);
    df.add_column("institution_name", institution_names);
    df.add_column("shares", shares_vals);
    df.add_column("value", value_vals);
    df.add_column("security_type", security_types);
    df.add_column("investment_discretion", investment_discretions);

    df = df.set_index("index");

    SPDLOG_DEBUG("Generated 13F data: {} institution-quarter records", period_ends.size());
    return df;
}

epoch_frame::DataFrame MockDataGenerator::GenerateInsiderTradingData(const GenerationConfig& config) {
    // Insider trading transactions - sporadic events
    uint64_t seed = config.seed ? config.seed : GenerateSeed(config.ticker + "_insider");
    std::mt19937_64 rng(seed);

    std::vector<std::string> transaction_dates, insider_names, transaction_codes;
    std::vector<double> shares_vals, price_vals;

    // Mock insider names
    std::vector<std::string> insiders = {
        "John Smith - CEO", "Jane Doe - CFO", "Bob Johnson - COO",
        "Alice Williams - Director", "Charlie Brown - VP Engineering"
    };

    // Transaction codes
    std::vector<std::string> codes = {"P", "S", "A", "M"}; // Purchase, Sale, Award, Exercise
    std::vector<double> code_probs = {0.2, 0.3, 0.3, 0.2}; // Probabilities

    // Generate insider transactions (sparse - not every day)
    std::istringstream ss(config.start_date);
    date::sys_days start_tp;
    ss >> date::parse("%Y-%m-%d", start_tp);
    auto current_time = system_clock::time_point(start_tp);

    std::uniform_real_distribution<double> trans_prob(0.0, 1.0);
    std::uniform_int_distribution<size_t> insider_idx(0, std::min(config.num_insiders, insiders.size()) - 1);
    std::uniform_real_distribution<double> shares_dist(1000, 100000); // 1K to 100K shares

    for (size_t i = 0; i < config.num_bars; ++i) {
        current_time += hours(24);

        // 5% chance of transaction on any given day
        if (trans_prob(rng) < 0.05) {
            auto time_t_val = system_clock::to_time_t(current_time);
            std::stringstream date_ss;
            date_ss << std::put_time(std::gmtime(&time_t_val), "%Y-%m-%dT%H:%M:%S");
            transaction_dates.push_back(date_ss.str());

            insider_names.push_back(insiders[insider_idx(rng)]);

            // Select transaction code based on probabilities
            double code_roll = trans_prob(rng);
            double cumulative = 0.0;
            std::string code = "P";
            for (size_t c = 0; c < codes.size(); ++c) {
                cumulative += code_probs[c];
                if (code_roll <= cumulative) {
                    code = codes[c];
                    break;
                }
            }
            transaction_codes.push_back(code);

            shares_vals.push_back(shares_dist(rng));
            price_vals.push_back(config.initial_price * (1.0 + 0.1 * i / config.num_bars));
        }
    }

    // Create DataFrame
    epoch_frame::DataFrame df;
    df.add_column("index", transaction_dates);
    df.add_column("insider_name", insider_names);
    df.add_column("transaction_code", transaction_codes);
    df.add_column("shares", shares_vals);
    df.add_column("price", price_vals);

    df = df.set_index("index");

    SPDLOG_DEBUG("Generated insider trading data: {} transactions", transaction_dates.size());
    return df;
}

epoch_frame::DataFrame MockDataGenerator::ResampleToIntraday(
    const epoch_frame::DataFrame& daily_data,
    const std::string& target_timeframe,
    uint64_t seed) {

    // This is a simplified resampling - in reality, we'd need more sophisticated logic
    // For now, we'll split each daily bar into multiple intraday bars

    auto tf_mins = ParseTimeframeMinutes(target_timeframe);
    if (!tf_mins) {
        throw std::runtime_error("Invalid target timeframe: " + target_timeframe);
    }

    // Calculate bars per day (assuming 6.5 hour trading day = 390 minutes)
    int bars_per_day = 390 / *tf_mins;
    if (bars_per_day == 0) {
        throw std::runtime_error("Timeframe too large for intraday: " + target_timeframe);
    }

    std::mt19937_64 rng(seed);
    std::normal_distribution<double> noise(0.0, 0.001); // Small intraday noise

    std::vector<std::string> timestamps;
    std::vector<double> opens, highs, lows, closes, vwaps;
    std::vector<int64_t> volumes;

    // TODO: Implement proper intraday resampling
    // For now, return empty DataFrame as placeholder
    // This will be implemented in a follow-up iteration

    epoch_frame::DataFrame result;
    result.add_column("index", timestamps);
    result.add_column("o", opens);
    result.add_column("h", highs);
    result.add_column("l", lows);
    result.add_column("c", closes);
    result.add_column("vw", vwaps);
    result.add_column("n", volumes);

    return result.set_index("index");
}

} // namespace epoch_script::runtime::test
