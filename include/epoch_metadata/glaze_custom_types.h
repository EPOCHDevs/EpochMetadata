//
// Created by adesola on 6/10/25.
//

#pragma once
#include <epoch_core/macros.h>
#include <epoch_frame/datetime.h>
#include <glaze/glaze.hpp>

namespace glz {
template <> struct to<JSON, epoch_frame::DateTime> {
  template <auto Opts>
  static void op(const epoch_frame::DateTime &x, auto &&...args) noexcept {
    const auto y = x.tz().empty() ? x : x.tz_convert("");
    serialize<JSON>::op<Opts>(y.repr(), args...);
  }
};

template <> struct from<JSON, epoch_frame::DateTime> {
  template <auto Opts>
  static void op(epoch_frame::DateTime &value, auto &&...args) {
    std::string human_readable;
    parse<JSON>::op<Opts>(human_readable, args...);
    value = epoch_frame::DateTime::from_str(human_readable);
  }
};

template <> struct to<JSON, epoch_frame::Date> {
  template <auto Opts>
  static void op(const epoch_frame::Date &x, auto &&...args) noexcept {
    serialize<JSON>::op<Opts>(x.repr(), args...);
  }
};

template <> struct from<JSON, epoch_frame::Date> {
  template <auto Opts>
  static void op(epoch_frame::Date &value, auto &&...args) {
    std::string human_readable;
    parse<JSON>::op<Opts>(human_readable, args...);
    value = epoch_frame::DateTime::from_date_str(human_readable).date();
  }
};

template <> struct meta<epoch_frame::Time> {
  using T = epoch_frame::Time;
  static constexpr auto read_x = [](T &value, glz::json_t const &time,
                                    glz::context &ctx) {
    if (time.is_string()) {
      for (auto const &[i, comp] : std::views::enumerate(
               std::views::split(time.as<std::string>(), ':'))) {
        std::string compStr{comp.begin(), comp.end()};

        switch (i) {
        case 0:
          value.hour = chrono_hour(std::stoi(compStr));
          break;
        case 1:
          value.minute = chrono_minute(std::stoi(compStr));
          break;
        case 2: {
          auto fraction = compStr.find('.');
          auto seconds = compStr.substr(0, fraction);
          value.second = chrono_second(std::stoi(seconds));

          if (fraction != std::string::npos) {
            value.microsecond =
                chrono_microsecond(std::stoi(seconds.substr(fraction + 1)));
          }
          break;
        }
        default:
          break;
        }
      }
    } else if (time.is_object()) {
      if (auto hour = time["hour"].get_if<double>()) {
        value.hour = chrono_hour(static_cast<int>(*hour));
      }
      if (auto minute = time["minute"].get_if<double>()) {
        value.minute = chrono_minute(static_cast<int>(*minute));
      }
      if (auto second = time["second"].get_if<double>()) {
        value.second = chrono_second(static_cast<int>(*second));
      }
      if (auto microsecond = time["microsecond"].get_if<double>()) {
        value.microsecond = chrono_microsecond(static_cast<int>(*microsecond));
      }
      if (auto tz = time["tz"].get_if<std::string>()) {
        value.tz = *tz;
      }
    } else {
      ctx.error = glz::error_code::constraint_violated;
      ctx.custom_error_message =
          "Invalid time format: " + time.dump().value_or("");
    }
  };

  static constexpr auto write_x = [](const T &value) -> glz::json_t {
    glz::json_t out;
    out["hour"] = value.hour.count();
    out["minute"] = value.minute.count();
    out["second"] = value.second.count();
    out["microsecond"] = value.microsecond.count();
    out["tz"] = value.tz;
    return out;
  };

  static constexpr auto value = glz::custom<read_x, write_x>;
};
} // namespace glz