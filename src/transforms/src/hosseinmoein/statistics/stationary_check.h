//
// Created by adesola on 5/16/25.
//

#pragma once

#include "../common_utils.h"
#include "epoch_metadata/transforms/itransform.h"
#include <DataFrame/DataFrameFinancialVisitors.h>
#include <epoch_frame/factory/dataframe_factory.h>

CREATE_ENUM(KpssType, Level, Trend);

namespace epoch_metadata::transform {

// Stationary check using hmdf::StationaryCheckVisitor over a rolling window
class ADFStationaryCheck final : public ITransform {
public:
  explicit ADFStationaryCheck(const TransformConfiguration &config)
      : ITransform(config),
        m_window(config.GetOptionValue("window").GetInteger()),
        m_adfLag(config.GetOptionValue("adf_lag").GetInteger()),
        m_adfWithTrend(config.GetOptionValue("adf_with_trend").GetBoolean()) {}

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    using namespace epoch_frame;
    const auto input = df[GetInputId()];
    const auto series =
        input.rolling_apply({.window_size = m_window})
            .apply([&](Series const &x) {
              hmdf::StationaryCheckVisitor<double> visitor(
                  hmdf::stationary_test::adf,
                  {.adf_lag = m_adfLag, .adf_with_trend = m_adfWithTrend});
              const SeriesSpan<> span{x};
              run_visit(x, visitor, span);
              return Scalar{visitor.get_adf_statistic()};
            });
    return series.to_frame(GetOutputId("result"));
  }

private:
  int64_t m_window;
  size_t m_adfLag;
  bool m_adfWithTrend;
};

class KPSSStationaryCheck final : public ITransform {
public:
  explicit KPSSStationaryCheck(const TransformConfiguration &config)
      : ITransform(config),
        m_window(config.GetOptionValue("window").GetInteger()) {
    SetCriticalValues(
        config.GetOptionValue("type").GetSelectOption<KpssType>());
  }

  [[nodiscard]] epoch_frame::DataFrame
  TransformData(epoch_frame::DataFrame const &df) const override {
    using namespace epoch_frame;
    const auto input = df[GetInputId()];
    const auto series = input.rolling_apply({.window_size = m_window})
                            .apply([&](Series const &x) {
                              hmdf::StationaryTestParams params;
                              params.critical_values[0] = m_criticalValues[0];
                              params.critical_values[1] = m_criticalValues[1];
                              params.critical_values[2] = m_criticalValues[2];
                              params.critical_values[3] = m_criticalValues[3];
                              hmdf::StationaryCheckVisitor<double> visitor(
                                  hmdf::stationary_test::kpss, params);

                              const SeriesSpan<> span{x};
                              run_visit(x, visitor, span);
                              return Scalar{visitor.get_kpss_statistic()};
                            });
    return series.to_frame(GetOutputId("result"));
  }

private:
  int64_t m_window;
  std::array<double, 4> m_criticalValues;

  void SetCriticalValues(KpssType type) {
    switch (type) {
    case KpssType::Level:
      m_criticalValues = {0.739, 0.463, 0.347, 0.263};
      break;
    case KpssType::Trend:
      m_criticalValues = {0.119, 0.146, 0.146, 0.146};
      break;
    default:
      throw std::invalid_argument("Invalid KpssType");
    }
  }
};

} // namespace epoch_metadata::transform
