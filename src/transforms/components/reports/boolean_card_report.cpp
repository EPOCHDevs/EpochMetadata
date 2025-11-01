#include "boolean_card_report.h"

namespace epochflow::reports {

std::string BooleanCardReport::GetAggregation() const {
  auto options = m_config.GetOptions();
  if (options.contains("agg") && options["agg"].IsType(epoch_core::MetaDataOptionType::Select)) {
    return options["agg"].GetSelectOption();
  }
  return "any";  // Default for boolean
}

} // namespace epochflow::reports