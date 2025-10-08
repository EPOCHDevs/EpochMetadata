//
// Created by adesola on 1/27/25.
//

#pragma once
#include "epoch_metadata/strategy/enums.h"
#include "epoch_metadata/strategy/generic_function.h"
#include <string>
#include <vector>

namespace epoch_metadata::strategy {

inline std::filesystem::path GetDefaultCacheDir() {
  // Check for CACHE_DIR (new standard env var)
  auto standardCacheDir = getenv("CACHE_DIR");
  if (standardCacheDir) {
    return std::filesystem::path{standardCacheDir};
  }
  SPDLOG_WARN("CACHE_DIR not set in environment");
  return std::filesystem::path{"cache/data"};
}

struct DataOption {
  std::vector<std::string> assets;
  std::string source;
  std::optional<TemplatedGenericFunction<epoch_core::RolloverType>>
      futures_continuation;
  std::filesystem::path cache_dir{GetDefaultCacheDir()};
};
} // namespace epoch_metadata::strategy