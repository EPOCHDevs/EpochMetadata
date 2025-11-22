#include <catch2/catch_test_macros.hpp>
#include <epoch_script/core/constants.h>
#include <epoch_script/transforms/core/registry.h>
#include "transforms/components/data_sources/sec_metadata.h"

using namespace epoch_script::transforms;
using namespace epoch_script::transform;

TEST_CASE("SEC Data Sources Metadata Registration", "[sec]") {
  SECTION("MakeSECDataSources returns two nodes") {
    auto metadataList = MakeSECDataSources();
    REQUIRE(metadataList.size() == 2);
  }

  SECTION("Form 13F Holdings node has correct basic properties") {
    auto metadataList = MakeSECDataSources();
    auto& form13f = metadataList[0];

    REQUIRE(form13f.id == "form13f_holdings");
    REQUIRE(form13f.name == "Form 13F Holdings");
    REQUIRE(form13f.category == epoch_core::TransformCategory::DataSource);
    REQUIRE(form13f.plotKind == epoch_core::TransformPlotKind::flag);
    REQUIRE(form13f.requiresTimeFrame == true);
    REQUIRE(form13f.isCrossSectional == false);
  }

  SECTION("Insider Trading node has correct basic properties") {
    auto metadataList = MakeSECDataSources();
    auto& insiderTrading = metadataList[1];

    REQUIRE(insiderTrading.id == "insider_trading");
    REQUIRE(insiderTrading.name == "Insider Trading");
    REQUIRE(insiderTrading.category == epoch_core::TransformCategory::DataSource);
    REQUIRE(insiderTrading.plotKind == epoch_core::TransformPlotKind::flag);
    REQUIRE(insiderTrading.requiresTimeFrame == true);
    REQUIRE(insiderTrading.isCrossSectional == false);
  }
}

TEST_CASE("Form 13F Holdings Configuration", "[sec][form13f]") {
  auto metadataList = MakeSECDataSources();
  auto& form13f = metadataList[0];

  SECTION("Has correct options") {
    REQUIRE(form13f.options.size() == 3);

    // filing_type option
    REQUIRE(form13f.options[0].id == "filing_type");
    REQUIRE(form13f.options[0].name == "Filing Type");
    REQUIRE(form13f.options[0].type == epoch_core::MetaDataOptionType::Select);
    REQUIRE(form13f.options[0].selectOption.size() == 4);

    // min_value option
    REQUIRE(form13f.options[1].id == "min_value");
    REQUIRE(form13f.options[1].name == "Minimum Position Value");
    REQUIRE(form13f.options[1].type == epoch_core::MetaDataOptionType::Decimal);

    // institution_cik option
    REQUIRE(form13f.options[2].id == "institution_cik");
    REQUIRE(form13f.options[2].name == "Institution CIK");
    REQUIRE(form13f.options[2].type == epoch_core::MetaDataOptionType::String);
  }

  SECTION("Has correct output fields") {
    REQUIRE(form13f.outputs.size() == 6);

    // Shares
    REQUIRE(form13f.outputs[0].id == "shares");
    REQUIRE(form13f.outputs[0].name == "Number of Shares Held");
    REQUIRE(form13f.outputs[0].type == epoch_core::IODataType::Decimal);
    REQUIRE(form13f.outputs[0].allowMultipleConnections == true);

    // Value
    REQUIRE(form13f.outputs[1].id == "value");
    REQUIRE(form13f.outputs[1].name == "Position Value (USD)");
    REQUIRE(form13f.outputs[1].type == epoch_core::IODataType::Decimal);

    // Security Type
    REQUIRE(form13f.outputs[2].id == "security_type");
    REQUIRE(form13f.outputs[2].name == "Security Type (SH/PRN)");
    REQUIRE(form13f.outputs[2].type == epoch_core::IODataType::String);

    // Investment Discretion
    REQUIRE(form13f.outputs[3].id == "investment_discretion");
    REQUIRE(form13f.outputs[3].name == "Investment Discretion (SOLE/SHARED/DFND)");
    REQUIRE(form13f.outputs[3].type == epoch_core::IODataType::String);

    // Institution Name
    REQUIRE(form13f.outputs[4].id == "institution_name");
    REQUIRE(form13f.outputs[4].name == "Institution Name");
    REQUIRE(form13f.outputs[4].type == epoch_core::IODataType::String);

    // Period End (now Timestamp type, filing_date removed as it's the index)
    REQUIRE(form13f.outputs[5].id == "period_end");
    REQUIRE(form13f.outputs[5].name == "Reporting Period End (Quarter End Date)");
    REQUIRE(form13f.outputs[5].type == epoch_core::IODataType::Timestamp);
  }

  SECTION("Has no input fields") {
    REQUIRE(form13f.inputs.empty());
  }

  SECTION("Has requiredDataSources set to output IDs") {
    REQUIRE(form13f.requiredDataSources.size() == 6);
    REQUIRE(std::find(form13f.requiredDataSources.begin(), form13f.requiredDataSources.end(), "shares") != form13f.requiredDataSources.end());
    REQUIRE(std::find(form13f.requiredDataSources.begin(), form13f.requiredDataSources.end(), "value") != form13f.requiredDataSources.end());
    REQUIRE(std::find(form13f.requiredDataSources.begin(), form13f.requiredDataSources.end(), "security_type") != form13f.requiredDataSources.end());
    REQUIRE(std::find(form13f.requiredDataSources.begin(), form13f.requiredDataSources.end(), "investment_discretion") != form13f.requiredDataSources.end());
    REQUIRE(std::find(form13f.requiredDataSources.begin(), form13f.requiredDataSources.end(), "institution_name") != form13f.requiredDataSources.end());
    REQUIRE(std::find(form13f.requiredDataSources.begin(), form13f.requiredDataSources.end(), "period_end") != form13f.requiredDataSources.end());
  }

  SECTION("Has correct tags") {
    REQUIRE(form13f.tags.size() == 6);

    // Check for specific tags
    auto hasSEC = std::find(form13f.tags.begin(), form13f.tags.end(), "sec") != form13f.tags.end();
    auto has13F = std::find(form13f.tags.begin(), form13f.tags.end(), "13f") != form13f.tags.end();
    auto hasInstitutional = std::find(form13f.tags.begin(), form13f.tags.end(), "institutional") != form13f.tags.end();
    auto hasHoldings = std::find(form13f.tags.begin(), form13f.tags.end(), "holdings") != form13f.tags.end();
    auto hasSmartMoney = std::find(form13f.tags.begin(), form13f.tags.end(), "smart-money") != form13f.tags.end();
    auto hasFundamentals = std::find(form13f.tags.begin(), form13f.tags.end(), "fundamentals") != form13f.tags.end();

    REQUIRE(hasSEC);
    REQUIRE(has13F);
    REQUIRE(hasInstitutional);
    REQUIRE(hasHoldings);
    REQUIRE(hasSmartMoney);
    REQUIRE(hasFundamentals);
  }

  SECTION("Has strategy metadata") {
    REQUIRE(!form13f.strategyTypes.empty());
    REQUIRE(form13f.strategyTypes.size() == 4);

    // Verify specific strategy types
    auto hasFundamentalAnalysis = std::find(form13f.strategyTypes.begin(),
                                            form13f.strategyTypes.end(),
                                            "fundamental-analysis") != form13f.strategyTypes.end();
    auto hasFollowSmartMoney = std::find(form13f.strategyTypes.begin(),
                                         form13f.strategyTypes.end(),
                                         "follow-smart-money") != form13f.strategyTypes.end();
    auto hasInstitutionalFlow = std::find(form13f.strategyTypes.begin(),
                                          form13f.strategyTypes.end(),
                                          "institutional-flow") != form13f.strategyTypes.end();

    REQUIRE(hasFundamentalAnalysis);
    REQUIRE(hasFollowSmartMoney);
    REQUIRE(hasInstitutionalFlow);

    REQUIRE(!form13f.assetRequirements.empty());
    REQUIRE(!form13f.usageContext.empty());
    REQUIRE(!form13f.limitations.empty());
  }

  SECTION("Has comprehensive description") {
    REQUIRE(!form13f.desc.empty());
    REQUIRE(form13f.desc.find("Form 13F") != std::string::npos);
    REQUIRE(form13f.desc.find("institutional holdings") != std::string::npos);
    REQUIRE(form13f.desc.find("$100M+") != std::string::npos);
  }

  SECTION("Usage context mentions key concepts") {
    REQUIRE(form13f.usageContext.find("smart-money") != std::string::npos);
    REQUIRE(form13f.usageContext.find("institutional ownership") != std::string::npos);
    REQUIRE(form13f.usageContext.find("quarterly") != std::string::npos);
  }

  SECTION("Limitations mention key constraints") {
    REQUIRE(form13f.limitations.find("45-day") != std::string::npos);
    REQUIRE(form13f.limitations.find("Quarterly") != std::string::npos);
    REQUIRE(form13f.limitations.find("long positions") != std::string::npos);
  }
}

TEST_CASE("Insider Trading Configuration", "[sec][insider]") {
  auto metadataList = MakeSECDataSources();
  auto& insiderTrading = metadataList[1];

  SECTION("Has correct options") {
    REQUIRE(insiderTrading.options.size() == 4);

    // filing_type option
    auto& filingTypeOption = insiderTrading.options[0];
    REQUIRE(filingTypeOption.id == "filing_type");
    REQUIRE(filingTypeOption.name == "Filing Type");
    REQUIRE(filingTypeOption.type == epoch_core::MetaDataOptionType::Select);
    REQUIRE(filingTypeOption.selectOption.size() == 4);

    // transaction_code option
    auto& transactionCodeOption = insiderTrading.options[1];
    REQUIRE(transactionCodeOption.id == "transaction_code");
    REQUIRE(transactionCodeOption.name == "Transaction Type");
    REQUIRE(transactionCodeOption.type == epoch_core::MetaDataOptionType::Select);
    REQUIRE(transactionCodeOption.selectOption.size() == 11);

    // Verify key transaction code options (removed "All" as user requested)
    auto hasPurchase = std::any_of(transactionCodeOption.selectOption.begin(),
                                   transactionCodeOption.selectOption.end(),
                                   [](const auto& opt) { return opt.value == "P"; });
    auto hasSale = std::any_of(transactionCodeOption.selectOption.begin(),
                              transactionCodeOption.selectOption.end(),
                              [](const auto& opt) { return opt.value == "S"; });
    auto hasAward = std::any_of(transactionCodeOption.selectOption.begin(),
                               transactionCodeOption.selectOption.end(),
                               [](const auto& opt) { return opt.value == "A"; });
    auto hasExercise = std::any_of(transactionCodeOption.selectOption.begin(),
                                  transactionCodeOption.selectOption.end(),
                                  [](const auto& opt) { return opt.value == "M"; });

    REQUIRE(hasPurchase);
    REQUIRE(hasSale);
    REQUIRE(hasAward);
    REQUIRE(hasExercise);

    // min_value option
    auto& minValueOption = insiderTrading.options[2];
    REQUIRE(minValueOption.id == "min_value");
    REQUIRE(minValueOption.name == "Minimum Transaction Value");
    REQUIRE(minValueOption.type == epoch_core::MetaDataOptionType::Decimal);

    // owner_name option
    auto& ownerNameOption = insiderTrading.options[3];
    REQUIRE(ownerNameOption.id == "owner_name");
    REQUIRE(ownerNameOption.name == "Insider Name");
    REQUIRE(ownerNameOption.type == epoch_core::MetaDataOptionType::String);
  }

  SECTION("Has correct output fields") {
    REQUIRE(insiderTrading.outputs.size() == 6);

    // Transaction Date (now Timestamp type)
    REQUIRE(insiderTrading.outputs[0].id == "transaction_date");
    REQUIRE(insiderTrading.outputs[0].name == "Transaction Date (When Trade Occurred)");
    REQUIRE(insiderTrading.outputs[0].type == epoch_core::IODataType::Timestamp);
    REQUIRE(insiderTrading.outputs[0].allowMultipleConnections == true);

    // Owner Name
    REQUIRE(insiderTrading.outputs[1].id == "owner_name");
    REQUIRE(insiderTrading.outputs[1].name == "Insider Name");
    REQUIRE(insiderTrading.outputs[1].type == epoch_core::IODataType::String);

    // Transaction Code
    REQUIRE(insiderTrading.outputs[2].id == "transaction_code");
    REQUIRE(insiderTrading.outputs[2].name == "Transaction Code (P/S/A/M)");
    REQUIRE(insiderTrading.outputs[2].type == epoch_core::IODataType::String);

    // Shares
    REQUIRE(insiderTrading.outputs[3].id == "shares");
    REQUIRE(insiderTrading.outputs[3].name == "Number of Shares");
    REQUIRE(insiderTrading.outputs[3].type == epoch_core::IODataType::Decimal);

    // Price
    REQUIRE(insiderTrading.outputs[4].id == "price");
    REQUIRE(insiderTrading.outputs[4].name == "Price Per Share");
    REQUIRE(insiderTrading.outputs[4].type == epoch_core::IODataType::Decimal);

    // Ownership After
    REQUIRE(insiderTrading.outputs[5].id == "ownership_after");
    REQUIRE(insiderTrading.outputs[5].name == "Ownership After Transaction");
    REQUIRE(insiderTrading.outputs[5].type == epoch_core::IODataType::Decimal);

    // Note: filing_date removed as it's the DataFrame index, not an output column
  }

  SECTION("Has no input fields") {
    REQUIRE(insiderTrading.inputs.empty());
  }

  SECTION("Has requiredDataSources set to output IDs") {
    REQUIRE(insiderTrading.requiredDataSources.size() == 6);
    REQUIRE(std::find(insiderTrading.requiredDataSources.begin(), insiderTrading.requiredDataSources.end(), "transaction_date") != insiderTrading.requiredDataSources.end());
    REQUIRE(std::find(insiderTrading.requiredDataSources.begin(), insiderTrading.requiredDataSources.end(), "owner_name") != insiderTrading.requiredDataSources.end());
    REQUIRE(std::find(insiderTrading.requiredDataSources.begin(), insiderTrading.requiredDataSources.end(), "transaction_code") != insiderTrading.requiredDataSources.end());
    REQUIRE(std::find(insiderTrading.requiredDataSources.begin(), insiderTrading.requiredDataSources.end(), "shares") != insiderTrading.requiredDataSources.end());
    REQUIRE(std::find(insiderTrading.requiredDataSources.begin(), insiderTrading.requiredDataSources.end(), "price") != insiderTrading.requiredDataSources.end());
    REQUIRE(std::find(insiderTrading.requiredDataSources.begin(), insiderTrading.requiredDataSources.end(), "ownership_after") != insiderTrading.requiredDataSources.end());
  }

  SECTION("Has correct tags") {
    REQUIRE(insiderTrading.tags.size() == 6);

    // Check for specific tags
    auto hasSEC = std::find(insiderTrading.tags.begin(), insiderTrading.tags.end(), "sec") != insiderTrading.tags.end();
    auto hasInsider = std::find(insiderTrading.tags.begin(), insiderTrading.tags.end(), "insider") != insiderTrading.tags.end();
    auto hasTrading = std::find(insiderTrading.tags.begin(), insiderTrading.tags.end(), "trading") != insiderTrading.tags.end();
    auto hasForm4 = std::find(insiderTrading.tags.begin(), insiderTrading.tags.end(), "form-4") != insiderTrading.tags.end();
    auto hasSmartMoney = std::find(insiderTrading.tags.begin(), insiderTrading.tags.end(), "smart-money") != insiderTrading.tags.end();
    auto hasSentiment = std::find(insiderTrading.tags.begin(), insiderTrading.tags.end(), "sentiment") != insiderTrading.tags.end();

    REQUIRE(hasSEC);
    REQUIRE(hasInsider);
    REQUIRE(hasTrading);
    REQUIRE(hasForm4);
    REQUIRE(hasSmartMoney);
    REQUIRE(hasSentiment);
  }

  SECTION("Has strategy metadata") {
    REQUIRE(!insiderTrading.strategyTypes.empty());
    REQUIRE(insiderTrading.strategyTypes.size() == 4);

    // Verify specific strategy types
    auto hasInsiderSentiment = std::find(insiderTrading.strategyTypes.begin(),
                                         insiderTrading.strategyTypes.end(),
                                         "insider-sentiment") != insiderTrading.strategyTypes.end();
    auto hasSmartMoney = std::find(insiderTrading.strategyTypes.begin(),
                                   insiderTrading.strategyTypes.end(),
                                   "smart-money") != insiderTrading.strategyTypes.end();
    auto hasSignalGeneration = std::find(insiderTrading.strategyTypes.begin(),
                                         insiderTrading.strategyTypes.end(),
                                         "signal-generation") != insiderTrading.strategyTypes.end();

    REQUIRE(hasInsiderSentiment);
    REQUIRE(hasSmartMoney);
    REQUIRE(hasSignalGeneration);

    REQUIRE(!insiderTrading.assetRequirements.empty());
    REQUIRE(!insiderTrading.usageContext.empty());
    REQUIRE(!insiderTrading.limitations.empty());
  }

  SECTION("Has comprehensive description") {
    REQUIRE(!insiderTrading.desc.empty());
    REQUIRE(insiderTrading.desc.find("insider trading") != std::string::npos);
    REQUIRE(insiderTrading.desc.find("Form") != std::string::npos);
    REQUIRE(insiderTrading.desc.find("2 business days") != std::string::npos);
  }

  SECTION("Usage context mentions key concepts") {
    bool hasInsiderActivity = (insiderTrading.usageContext.find("insider buying") != std::string::npos ||
                               insiderTrading.usageContext.find("insider purchases") != std::string::npos);
    REQUIRE(hasInsiderActivity);
    REQUIRE(insiderTrading.usageContext.find("bullish") != std::string::npos);
    REQUIRE(insiderTrading.usageContext.find("sentiment") != std::string::npos);
  }

  SECTION("Limitations mention key constraints") {
    REQUIRE(insiderTrading.limitations.find("2-day") != std::string::npos);
    REQUIRE(insiderTrading.limitations.find("10b5-1") != std::string::npos);
  }
}

TEST_CASE("SEC Data Sources atLeastOneInputRequired", "[sec]") {
  auto metadataList = MakeSECDataSources();

  SECTION("Form 13F Holdings has atLeastOneInputRequired = false") {
    REQUIRE(metadataList[0].atLeastOneInputRequired == false);
  }

  SECTION("Insider Trading has atLeastOneInputRequired = false") {
    REQUIRE(metadataList[1].atLeastOneInputRequired == false);
  }
}
