//
// Created by adesola on 1/14/25.
//

#pragma once

#include <epoch_lab_shared/enum_wrapper.h>
#include <string>
#include <vector>
#include <glaze/glaze.hpp>


CREATE_ENUM(CppFolioChartType,
        Lines,
        Area,
        HeatMap,
        Histogram,
        HorizonatalBar,
        BoxPlots,
        Gantt);

CREATE_ENUM(CppFolioColumnType, Percent, Decimal, Integer, Date, DateTime, Notional, String, DayDuration);
CREATE_ENUM(CppFolioCategory, Return, Position, Transaction, RoundTrip);
CREATE_ENUM(CppFolioSubCategory, StrategyBenchmark, RiskAnalysis,  ReturnsDistribution);


namespace metadata::cppFolio {
    struct SubCategoryMetaData {
        CppFolioSubCategory value;
        std::string label;
        std::string desc;
    };

    struct CategoryMetaData {
        CppFolioCategory value;
        std::string label;
        std::vector<SubCategoryMetaData> subCategories;
    };

    std::vector<CategoryMetaData> GetCategoryMetaData();
}