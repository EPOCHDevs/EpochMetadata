#pragma once

#include <epoch_frame/dataframe.h>
#include <epoch_frame/datetime.h>
#include <epoch_frame/index.h>
#include <vector>

namespace epochflow::transform::sessions_utils {
struct SessionState {
  std::vector<bool> active;
  std::vector<bool> opened;
  std::vector<bool> closed;
};

// Build active mask for a UTC index given a local SessionRange (start/end may
// carry tz)
SessionState BuildActiveMaskUTC(const epoch_frame::IndexPtr &utcIndex,
                                const epoch_frame::SessionRange &range);

// Slice a UTC DataFrame to the active session range via timezone-aware
// boundaries
epoch_frame::DataFrame
SliceBySessionUTC(const epoch_frame::DataFrame &dfUTC,
                  const epoch_frame::SessionRange &range);

} // namespace epochflow::transform::sessions_utils
