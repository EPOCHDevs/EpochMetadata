#include "any_card_report.h"

namespace epoch_script::reports {

std::string AnyCardReport::GetAggregation() const {
  auto options = m_config.GetOptions();
  if (options.contains("agg") && options["agg"].IsType(epoch_core::MetaDataOptionType::Select)) {
    return options["agg"].GetSelectOption();
  }
  return "last";  // Default for any type
}

} // namespace epoch_script::reports