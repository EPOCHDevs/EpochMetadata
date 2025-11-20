//
// Created by adesola
//

#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <stdexcept>
#include <epoch_data_sdk/model/asset/index_constituents.hpp>
#include <epoch_data_sdk/model/asset/asset.hpp>
#include <epoch_data_sdk/model/asset/asset_specification.hpp>
#include <epoch_data_sdk/model/builder/asset_builder.hpp>
#include <spdlog/spdlog.h>
#include <glaze/glaze.hpp>

namespace epoch_script::strategy {

struct AssetIDContainer {
  std::vector<std::string> raw_asset_ids;

  // Default constructor
  AssetIDContainer() = default;

  // Constructor from vector of asset IDs
  explicit AssetIDContainer(std::vector<std::string> asset_ids)
      : raw_asset_ids(std::move(asset_ids)) {}

  // Constructor from initializer list for convenience
  AssetIDContainer(std::initializer_list<std::string> asset_ids)
      : raw_asset_ids(asset_ids) {}

  // Resolves and validates asset IDs:
  // 1. Validates that all asset IDs are valid by attempting to create Asset objects
  // 2. For FX/Crypto assets without ^ prefix, prepends it if needed
  // 3. For IDs without dash separator, checks if they're index constituents
  //    and expands them to their constituent asset IDs
  // Returns the fully resolved list of asset IDs
  // Throws std::runtime_error if any asset ID is invalid
  std::vector<std::string> Resolve() const {
    std::unordered_set<std::string> resolvedSet;
    std::vector<std::string> resolved;

    for (const auto& id : raw_asset_ids) {
      // Check if this is potentially an index (no dash separator)
      if (id.find('-') == std::string::npos) {
        // Try to expand as index constituent
        const auto& indexDB = data_sdk::asset::IndexConstituentsDatabase::GetInstance();
        auto constituentsOpt = indexDB.GetConstituents(id);

        if (constituentsOpt.has_value()) {
          const auto& constituents = constituentsOpt.value();
          SPDLOG_INFO("AssetIDContainer: Expanding index {} to {} constituents",
                      id, constituents.size());

          for (const auto& constituentId : constituents) {
            // Validate and process each constituent
            std::string processedId = ValidateAndProcessAssetID(constituentId);
            if (resolvedSet.insert(processedId).second) {
              resolved.push_back(processedId);
            }
          }
          continue; // Skip adding the index itself
        }
      }

      // Not an index, validate and process as single asset
      std::string processedId = ValidateAndProcessAssetID(id);
      if (resolvedSet.insert(processedId).second) {
        resolved.push_back(processedId);
      }
    }

    return resolved;
  }

private:
  // Validates and processes a single asset ID:
  // - Prepends ^ for FX/Crypto assets if not already present
  // - Validates the asset ID by attempting to create an Asset object
  // - Throws std::runtime_error if the asset ID is invalid
  std::string ValidateAndProcessAssetID(const std::string& id) const {
    std::string processedId = id;

    // Check if ID needs ^ prefix for FX/Crypto
    // FX/Crypto pairs should start with ^, unless they already have it
    if (!id.empty() && id[0] != '^') {
      // Check if this is FX or Crypto by looking at the asset class suffix
      bool isFXOrCrypto = false;
      auto dashPos = id.find('-');

      if (dashPos != std::string::npos) {
        // Extract the asset class part after the dash
        std::string assetClass = id.substr(dashPos + 1);
        isFXOrCrypto = (assetClass == "FX" || assetClass == "Crypto");
      }

      // If it's FX or Crypto, prepend the ^ prefix
      if (isFXOrCrypto) {
        processedId = "^" + id;
        SPDLOG_DEBUG("AssetIDContainer: Added ^ prefix to {}, result: {}",
                    id, processedId);
      }
    }

    // Validate the processed asset ID by creating an Asset object
    try {
      auto asset = data_sdk::asset::MakeAsset(
          data_sdk::asset::AssetSpecificationQuery{processedId});
      // Return the canonical asset ID from the created asset
      return asset.GetID();
    } catch (const std::exception& e) {
      throw std::runtime_error(
          std::format("Invalid asset ID '{}': {}", processedId, e.what()));
    }
  }
};

} // namespace epoch_script::strategy

// Glaze serialization for AssetIDContainer
// Serialize directly as a string array instead of an object
template <>
struct glz::meta<epoch_script::strategy::AssetIDContainer> {
  using T = epoch_script::strategy::AssetIDContainer;
  static constexpr auto value = &T::raw_asset_ids;
};