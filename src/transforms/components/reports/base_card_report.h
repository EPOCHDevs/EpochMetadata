#pragma once

#include <epochflow/transforms/components/reports/ireport.h>
#include <epoch_dashboard/tearsheet/card_builder.h>
#include <epoch_dashboard/tearsheet/scalar_converter.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/series_or_scalar.h>
#include <epoch_frame/scalar.h>

namespace epochflow::reports {

class BaseCardReport : public IReporter {
public:
  explicit BaseCardReport(epochflow::transform::TransformConfiguration config)
      : IReporter(std::move(config), true) {}

protected:
  void generateTearsheet(const epoch_frame::DataFrame &normalizedDf) const override;

  // Shared helper methods
  std::string GetCategory() const;
  std::string GetTitle() const;
  uint32_t GetGroup() const;
  uint32_t GetGroupSize() const;
  epoch_proto::EpochFolioDashboardWidget GetWidgetType() const;

  // Virtual method for aggregation - subclasses provide specific defaults
  virtual std::string GetAggregation() const = 0;
};


} // namespace epochflow::reports