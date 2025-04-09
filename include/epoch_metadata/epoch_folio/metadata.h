//
// Created by adesola on 1/14/25.
//

#pragma once

#include <epoch_core/enum_wrapper.h>
#include <glaze/glaze.hpp>
#include <string>
#include <vector>

CREATE_ENUM(CppFolioChartType, Lines, Area, HeatMap, Histogram, HorizonatalBar,
            BoxPlots, Gantt);

CREATE_ENUM(CppFolioColumnType, Percent, Decimal, Integer, Date, DateTime,
            Notional, String, DayDuration);
CREATE_ENUM(CppFolioCategory, Return, Position, Transaction, RoundTrip);
CREATE_ENUM(CppFolioSubCategory, StrategyBenchmark, RiskAnalysis,
            ReturnsDistribution);

namespace epoch_metadata::cppFolio {
struct SubCategoryMetaData {
  epoch_core::CppFolioSubCategory value;
  std::string label;
  std::string desc;
};

struct CategoryMetaData {
  epoch_core::CppFolioCategory value;
  std::string label;
  std::vector<SubCategoryMetaData> subCategories;
};

std::vector<CategoryMetaData> GetCategoryMetaData();
} // namespace epoch_metadata::cppFolio