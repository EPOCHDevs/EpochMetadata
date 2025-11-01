#include <epoch_script/transforms/core/sessions_utils.h>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/series_factory.h>
#include <epoch_frame/index.h>

namespace epoch_script::transform::sessions_utils {

SessionState BuildActiveMaskUTC(const epoch_frame::IndexPtr &utcIndex,
                                const epoch_frame::SessionRange &range) {
  SessionState state;
  state.active.resize(utcIndex->size(), false);
  state.opened.resize(utcIndex->size(), false);
  state.closed.resize(utcIndex->size(), false);

  // Determine session tz: prefer range.start/end tz, fallback to UTC
  const std::string sessionTz =
      !range.start.tz.empty()
          ? range.start.tz
          : (!range.end.tz.empty() ? range.end.tz : std::string("UTC"));

  // Cache computed [startUTC, endUTC] per local date to avoid recompute
  std::optional<epoch_frame::Date> cachedDate;
  epoch_frame::DateTime cachedStartUTC;
  epoch_frame::DateTime cachedEndUTC;

  for (size_t i = 0; i < utcIndex->size(); ++i) {
    const auto dtUTC = utcIndex->at(i).to_datetime();
    const auto dtLocal = dtUTC.tz_convert(sessionTz);
    const auto localDate = dtLocal.date();

    if (!cachedDate || *cachedDate != localDate) {
      cachedDate = localDate;
      // Build local start/end DateTime for this date
      auto startLocal = epoch_frame::DateTime{localDate, range.start};
      auto endLocal = epoch_frame::DateTime{localDate, range.end};
      if (range.end < range.start) {
        endLocal = epoch_frame::DateTime{localDate + chrono_days(1), range.end};
      }
      // Convert to UTC once
      cachedStartUTC = startLocal.tz_convert("UTC");
      cachedEndUTC = endLocal.tz_convert("UTC");
    }
    state.opened[i] = (cachedStartUTC == dtUTC);
    state.closed[i] = (cachedEndUTC == dtUTC);

    state.active[i] = (cachedStartUTC <= dtUTC && dtUTC <= cachedEndUTC);
  }

  return state;
}

epoch_frame::DataFrame
SliceBySessionUTC(const epoch_frame::DataFrame &dfUTC,
                  const epoch_frame::SessionRange &range) {
  if (dfUTC.empty())
    return dfUTC;
  auto mask = epoch_frame::make_series(
      dfUTC.index(),
      epoch_frame::factory::array::make_array(
          BuildActiveMaskUTC(dfUTC.index(), range).active),
      "__session_active");
  return dfUTC.loc(mask);
}

} // namespace epoch_script::transform::sessions_utils
