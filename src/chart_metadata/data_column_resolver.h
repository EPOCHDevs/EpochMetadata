#pragma once

#include <string>
#include <vector>
#include "epoch_script/transforms/core/transform_configuration.h"


namespace epoch_script::chart_metadata {

// Constants
constexpr const char *INDEX_COLUMN = "index";

/**
 * @brief Resolves data columns required for chart series based on transform
 * configuration
 *
 * This class is responsible for determining which data columns a transform
 * needs for visualization, including handling special cases for multi-output
 * indicators.
 */
class DataColumnResolver {
public:
  /**
   * @brief Resolve data columns for a transform
   * @param cfg The transform configuration
   * @return Vector of column identifiers needed for the chart series
   */
  static std::vector<std::string>
  ResolveColumns(const epoch_script::transform::TransformConfiguration &cfg);
};

} // namespace epoch_script::chart_metadata
