//
// Created by adesola on 1/14/25.
//

#include "metadata.h"

namespace epoch_metadata::cppFolio {
std::vector<CategoryMetaData> GetCategoryMetaData() {
  return {CategoryMetaData{
              CppFolioCategory::Return,
              "Returns",
              {SubCategoryMetaData{CppFolioSubCategory::StrategyBenchmark,
                                   "Strategy Benchmarks", ""},
               SubCategoryMetaData{CppFolioSubCategory::RiskAnalysis,
                                   "Risk Analysis", ""},
               SubCategoryMetaData{CppFolioSubCategory::ReturnsDistribution,
                                   "Returns Distribution", ""}}},
          CategoryMetaData{CppFolioCategory::Position, "Positions", {}}};
}
} // namespace epoch_metadata::cppFolio