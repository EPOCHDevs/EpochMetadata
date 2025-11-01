#include "test_constants.h"
#include <epoch_metadata/time_frame.h>

namespace epoch_flow::runtime::test {

epoch_metadata::TimeFrame TestTimeFrames::Daily() {
    return epoch_metadata::TimeFrame("1d");
}

epoch_metadata::TimeFrame TestTimeFrames::Hourly() {
    return epoch_metadata::TimeFrame("1h");
}

epoch_metadata::TimeFrame TestTimeFrames::Minute() {
    return epoch_metadata::TimeFrame("1m");
}

} // namespace epoch_flow::runtime::test
