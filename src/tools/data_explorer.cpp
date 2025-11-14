//
// Data Explorer Tool
// Loads all available DataCategory types and saves them as Arrow files for inspection
//

#include <iostream>
#include <string>
#include <filesystem>

#include <epoch_script/core/asset_id.h>
#include <epoch_script/core/timeframe.h>
#include <epoch_data_sdk/dataloader/options.hpp>
#include <epoch_data_sdk/dataloader/factory.hpp>
#include <epoch_data_sdk/common/enums.hpp>
#include <epoch_frame/dataframe.h>

namespace fs = std::filesystem;
using namespace data_sdk;
using namespace data_sdk::dataloader;

struct ExplorerConfig {
    std::string ticker = "AAPL";
    epoch_core::Exchange exchange = epoch_core::Exchange::NASDAQ;
    epoch_core::AssetClass asset_class = epoch_core::AssetClass::Stocks;
    std::string start_date = "2024-01-01";
    std::string end_date = "2024-12-31";
    std::string output_dir = ".";
};

void PrintUsage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  --ticker TICKER        Asset ticker (default: AAPL)\n"
              << "  --start-date YYYY-MM-DD Start date (default: 2024-01-01)\n"
              << "  --end-date YYYY-MM-DD   End date (default: 2024-12-31)\n"
              << "  --output-dir PATH       Output directory (default: current directory)\n"
              << "  --help                  Show this help\n";
}

ExplorerConfig ParseArgs(int argc, char* argv[]) {
    ExplorerConfig config;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            exit(0);
        } else if (arg == "--ticker" && i + 1 < argc) {
            config.ticker = argv[++i];
        } else if (arg == "--start-date" && i + 1 < argc) {
            config.start_date = argv[++i];
        } else if (arg == "--end-date" && i + 1 < argc) {
            config.end_date = argv[++i];
        } else if (arg == "--output-dir" && i + 1 < argc) {
            config.output_dir = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            PrintUsage(argv[0]);
            exit(1);
        }
    }

    return config;
}

void SaveDataFrame(const epoch_frame::DataFrame& df,
                   const std::string& category_name,
                   const std::string& ticker,
                   const std::string& output_dir) {

    fs::path output_path = fs::path(output_dir) / (category_name + "_" + ticker + ".arrow");

    try {
        df.to_arrow_ipc(output_path.string());
        std::cout << "  ✓ Saved " << output_path.filename() << " ("
                  << df.num_rows() << " rows, "
                  << df.num_columns() << " columns)\n";
    } catch (const std::exception& e) {
        std::cerr << "  ✗ Failed to save " << output_path.filename()
                  << ": " << e.what() << "\n";
    }
}

void PrintDataFrameSummary(const epoch_frame::DataFrame& df, const std::string& category_name) {
    std::cout << "\n" << category_name << ":\n";
    std::cout << "  Rows: " << df.num_rows() << "\n";
    std::cout << "  Columns: " << df.num_columns() << " - [";

    auto cols = df.column_names();
    for (size_t i = 0; i < std::min(cols.size(), size_t(10)); i++) {
        std::cout << cols[i];
        if (i < std::min(cols.size(), size_t(10)) - 1) std::cout << ", ";
    }
    if (cols.size() > 10) std::cout << ", ...";
    std::cout << "]\n";

    if (df.num_rows() > 0) {
        // Print date range from index
        auto index = df.index();
        if (index.size() > 0) {
            std::cout << "  Date range: " << index.at(0) << " to "
                      << index.at(index.size() - 1) << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    auto config = ParseArgs(argc, argv);

    std::cout << "\n=== Data Explorer ===\n";
    std::cout << "Ticker: " << config.ticker << "\n";
    std::cout << "Date Range: " << config.start_date << " to " << config.end_date << "\n";
    std::cout << "Output Directory: " << config.output_dir << "\n\n";

    // Create output directory if it doesn't exist
    fs::create_directories(config.output_dir);

    // Create asset ID
    epoch_core::AssetID asset_id(config.ticker, config.asset_class, config.exchange);

    // Setup data loader options
    DataloaderOption options;
    options.SetDateRange(config.start_date, config.end_date);
    options.SetDataloaderAssets({asset_id});
    options.SetStrategyAssets({asset_id});
    options.SetDataProvider(DataProvider::Polygon);

    // Create loader
    auto loader_result = MakeDataloader(options);
    if (!loader_result) {
        std::cerr << "Failed to create dataloader: " << loader_result.error() << "\n";
        return 1;
    }
    auto loader = std::move(*loader_result);

    try {
        // 1. Load MinuteBars
        std::cout << "Loading MinuteBars...\n";
        auto minute_result = loader->LoadAssetBars(asset_id, DataCategory::MinuteBars);
        if (minute_result.has_value()) {
            PrintDataFrameSummary(*minute_result, "MinuteBars");
            SaveDataFrame(*minute_result, "MinuteBars", config.ticker, config.output_dir);
        } else {
            std::cout << "  ✗ " << minute_result.error() << "\n";
        }

        // 2. Load DailyBars
        std::cout << "\nLoading DailyBars...\n";
        auto daily_result = loader->LoadAssetBars(asset_id, DataCategory::DailyBars);
        if (daily_result.has_value()) {
            PrintDataFrameSummary(*daily_result, "DailyBars");
            SaveDataFrame(*daily_result, "DailyBars", config.ticker, config.output_dir);
        } else {
            std::cout << "  ✗ " << daily_result.error() << "\n";
        }

        // 3. Load Financials - different statement types
        std::cout << "\nLoading Financials (Balance Sheet)...\n";
        FinancialsConfig bs_config(FinancialsStatementType::BalanceSheet);
        auto bs_result = loader->LoadAssetBars(asset_id, DataCategory::Financials,
                                               bs_config.ToParameters());
        if (bs_result.has_value()) {
            PrintDataFrameSummary(*bs_result, "Financials_BalanceSheet");
            SaveDataFrame(*bs_result, "Financials_BalanceSheet", config.ticker, config.output_dir);
        } else {
            std::cout << "  ✗ " << bs_result.error() << "\n";
        }

        std::cout << "\nLoading Financials (Income Statement)...\n";
        FinancialsConfig is_config(FinancialsStatementType::IncomeStatement);
        auto is_result = loader->LoadAssetBars(asset_id, DataCategory::Financials,
                                               is_config.ToParameters());
        if (is_result.has_value()) {
            PrintDataFrameSummary(*is_result, "Financials_IncomeStatement");
            SaveDataFrame(*is_result, "Financials_IncomeStatement", config.ticker, config.output_dir);
        } else {
            std::cout << "  ✗ " << is_result.error() << "\n";
        }

        std::cout << "\nLoading Financials (Cash Flow)...\n";
        FinancialsConfig cf_config(FinancialsStatementType::CashFlow);
        auto cf_result = loader->LoadAssetBars(asset_id, DataCategory::Financials,
                                               cf_config.ToParameters());
        if (cf_result.has_value()) {
            PrintDataFrameSummary(*cf_result, "Financials_CashFlow");
            SaveDataFrame(*cf_result, "Financials_CashFlow", config.ticker, config.output_dir);
        } else {
            std::cout << "  ✗ " << cf_result.error() << "\n";
        }

        // 4. Load MacroEconomics - key indicators
        std::cout << "\nLoading MacroEconomics (CPI)...\n";
        MacroEconomicsConfig cpi_config(MacroEconomicsIndicator::CPI);
        auto cpi_result = loader->LoadAssetBars(asset_id, DataCategory::MacroEconomics,
                                                cpi_config.ToParameters());
        if (cpi_result.has_value()) {
            PrintDataFrameSummary(*cpi_result, "MacroEconomics_CPI");
            SaveDataFrame(*cpi_result, "MacroEconomics_CPI", config.ticker, config.output_dir);
        } else {
            std::cout << "  ✗ " << cpi_result.error() << "\n";
        }

        std::cout << "\nLoading MacroEconomics (Fed Funds)...\n";
        MacroEconomicsConfig fedfunds_config(MacroEconomicsIndicator::FedFunds);
        auto fedfunds_result = loader->LoadAssetBars(asset_id, DataCategory::MacroEconomics,
                                                     fedfunds_config.ToParameters());
        if (fedfunds_result.has_value()) {
            PrintDataFrameSummary(*fedfunds_result, "MacroEconomics_FedFunds");
            SaveDataFrame(*fedfunds_result, "MacroEconomics_FedFunds", config.ticker, config.output_dir);
        } else {
            std::cout << "  ✗ " << fedfunds_result.error() << "\n";
        }

        std::cout << "\nLoading MacroEconomics (Unemployment)...\n";
        MacroEconomicsConfig unemp_config(MacroEconomicsIndicator::Unemployment);
        auto unemp_result = loader->LoadAssetBars(asset_id, DataCategory::MacroEconomics,
                                                  unemp_config.ToParameters());
        if (unemp_result.has_value()) {
            PrintDataFrameSummary(*unemp_result, "MacroEconomics_Unemployment");
            SaveDataFrame(*unemp_result, "MacroEconomics_Unemployment", config.ticker, config.output_dir);
        } else {
            std::cout << "  ✗ " << unemp_result.error() << "\n";
        }

        // 5. Load News
        std::cout << "\nLoading News...\n";
        auto news_result = loader->LoadAssetBars(asset_id, DataCategory::News);
        if (news_result.has_value()) {
            PrintDataFrameSummary(*news_result, "News");
            SaveDataFrame(*news_result, "News", config.ticker, config.output_dir);
        } else {
            std::cout << "  ✗ " << news_result.error() << "\n";
        }

        // 6. Load Dividends
        std::cout << "\nLoading Dividends...\n";
        auto div_result = loader->LoadAssetBars(asset_id, DataCategory::Dividends);
        if (div_result.has_value()) {
            PrintDataFrameSummary(*div_result, "Dividends");
            SaveDataFrame(*div_result, "Dividends", config.ticker, config.output_dir);
        } else {
            std::cout << "  ✗ " << div_result.error() << "\n";
        }

        // 7. Load Splits
        std::cout << "\nLoading Splits...\n";
        auto split_result = loader->LoadAssetBars(asset_id, DataCategory::Splits);
        if (split_result.has_value()) {
            PrintDataFrameSummary(*split_result, "Splits");
            SaveDataFrame(*split_result, "Splits", config.ticker, config.output_dir);
        } else {
            std::cout << "  ✗ " << split_result.error() << "\n";
        }

        std::cout << "\n=== Exploration Complete ===\n";
        std::cout << "Arrow files saved to: " << config.output_dir << "\n";
        std::cout << "Run: python scripts/inspect_data_categories.py\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
