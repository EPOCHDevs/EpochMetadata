//
// Created by dewe on 7/22/23.
//
#include "catch.hpp"
#include <epoch_metadata/model/asset/asset_database.h"
#include <epoch_metadata/model/common/constants.h"

using namespace epoch_flow;
TEST_CASE("Asset Specification Database Test", "[asset_spec_db]") {
    // Load the AssetSpecificationDatabase
    const auto &db =
        asset::AssetSpecificationDatabase::GetInstance();

    REQUIRE_NOTHROW(db.GetAssetSpecification(
        SymbolConstants::instance().AAPL, epoch_core::AssetClass::Stocks,
        epoch_core::Exchange::NASDAQ, epoch_core::CountryCurrency::USD));
    REQUIRE_NOTHROW(db.GetAssetSpecification(SymbolConstants::instance().ES,
    epoch_core::AssetClass::Futures, epoch_core::Exchange::GBLX,
                                             epoch_core::CountryCurrency::USD));
    REQUIRE_THROWS(db.GetAssetSpecification(
        SymbolConstants::instance().AAPL, epoch_core::AssetClass::Stocks,
        epoch_core::Exchange::CME, epoch_core::CountryCurrency::USD));
}
