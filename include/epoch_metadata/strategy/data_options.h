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
  // Check for EPOCH_DATA_CACHE_DIR (explicit data cache env var)
  auto dataCacheDir = getenv("EPOCH_DATA_CACHE_DIR");
  if (dataCacheDir) {
    return std::filesystem::path{dataCacheDir};
  }
  SPDLOG_WARN("EPOCH_DATA_CACHE_DIR not set in environment, using default 'cache/data'");
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