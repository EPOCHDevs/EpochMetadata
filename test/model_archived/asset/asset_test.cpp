//
// Created by dewe on 7/22/23.
//
#include <epoch_metadata/model/asset/asset.h" // Include the header file that defines ContractInfo
#include <catch2/catch_test_macros.hpp>
#include <epoch_metadata/model/common/constants.h>

using namespace epoch_flow;
using namespace epoch_flow::asset;
using namespace std::chrono_literals;

TEST_CASE("Test MakeFuturesContractInfo", "[ContractInfo]") {
  const auto currentDate = epoch_frame::DateTime::now();

  // Test with a valid futures contract symbol
  const std::string contract1("XYZF22");
  ContractInfo info1 = ContractInfo::MakeFuturesContractInfo(contract1);
  auto expirationDate = info1.GetExpirationDate();
  REQUIRE(expirationDate.day == 15d);
  REQUIRE(expirationDate.year == 2022y);
  REQUIRE(expirationDate.month ==
          std::chrono::January); // Assuming 'F' corresponds to January
  REQUIRE(info1.GetSymbol() == contract1);

  // Test with another valid futures contract symbol
  const std::string contract2("ABCN23");
  info1 = ContractInfo::MakeFuturesContractInfo(contract2);
  expirationDate = info1.GetExpirationDate();
  REQUIRE(expirationDate.day == 15d);
  REQUIRE(expirationDate.year == 2023y);
  REQUIRE(expirationDate.month ==
          std::chrono::July); // Assuming 'N' corresponds to July
  REQUIRE(info1.GetSymbol() == contract2);
}