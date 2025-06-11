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

  template <> struct to<JSON, epoch_frame::Time> {
    template <auto Opts>
    static void op(const epoch_frame::Time &x, auto &&...args) noexcept {
      serialize<JSON>::op<Opts>(x.repr(), args...);
    }
  };

  template <> struct from<JSON, epoch_frame::Time> {
    template <auto Opts>
    static void op(epoch_frame::Time &value, auto &&...args) {
      std::string human_readable;
      parse<JSON>::op<Opts>(human_readable, args...);
      for (auto const& [i, comp] : std::views::enumerate(std::views::split(human_readable, ':'))) {
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
              value.microsecond = chrono_microsecond(std::stoi(seconds.substr(fraction+1)));
            }
            break;
          }
          default:
            break;
        }
      }
    }
  };
}