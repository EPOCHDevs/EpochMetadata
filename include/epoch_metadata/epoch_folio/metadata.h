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
CREATE_ENUM(CppFolioCategory, StrategyBenchmark, RiskAnalysis, ReturnsDistribution, Positions, Transactions, RoundTrip);

namespace epoch_metadata::cppFolio {


struct CategoryMetaData {
  epoch_core::CppFolioCategory value;
  std::string label;
  std::string desc;
};

std::vector<CategoryMetaData> GetCategoryMetaData();
} // namespace epoch_metadata::cppFolio