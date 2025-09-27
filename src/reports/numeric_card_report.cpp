#include "numeric_card_report.h"

namespace epoch_metadata::reports {

std::string NumericCardReport::GetAggregation() const {
  auto options = m_config.GetOptions();
  if (options.contains("agg") && options["agg"].IsType(epoch_core::MetaDataOptionType::Select)) {
    return options["agg"].GetSelectOption();
  }
  return "mean";  // Default for numeric
}

} // namespace epoch_metadata::reports