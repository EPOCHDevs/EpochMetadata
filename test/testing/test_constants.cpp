#include "test_constants.h"
#include <../../include/epochflow/core/time_frame.h>

namespace epoch_flow::runtime::test {

epochflow::TimeFrame TestTimeFrames::Daily() {
    return epochflow::TimeFrame("1d");
}

epochflow::TimeFrame TestTimeFrames::Hourly() {
    return epochflow::TimeFrame("1h");
}

epochflow::TimeFrame TestTimeFrames::Minute() {
    return epochflow::TimeFrame("1m");
}

} // namespace epoch_flow::runtime::test
