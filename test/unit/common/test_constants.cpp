#include "test_constants.h"
#include <epoch_script/core/time_frame.h>

namespace epoch_script::runtime::test {

epoch_script::TimeFrame TestTimeFrames::Daily() {
    return epoch_script::TimeFrame("1d");
}

epoch_script::TimeFrame TestTimeFrames::Hourly() {
    return epoch_script::TimeFrame("1h");
}

epoch_script::TimeFrame TestTimeFrames::Minute() {
    return epoch_script::TimeFrame("1m");
}

} // namespace epoch_script::runtime::test
