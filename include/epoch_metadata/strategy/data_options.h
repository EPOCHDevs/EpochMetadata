//
// Created by adesola on 1/27/25.
//

#pragma once
#include "epoch_metadata/strategy/enums.h"
#include "epoch_metadata/strategy/generic_function.h"
#include <string>
#include <vector>

namespace epoch_metadata::strategy {
struct DataOption {
  std::vector<std::string> assets;
  std::string source;
  std::optional<TemplatedGenericFunction<epoch_core::RolloverType>>
      futures_continuation;
    std::filesystem::path cache_dir{std::filesystem::temp_directory_path()};
};
} // namespace epoch_stratifyx