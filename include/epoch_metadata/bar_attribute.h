#pragma once
//
// Created by dewe on 1/10/23.
//
#include "epoch_metadata/time_frame.h"
#include <cstdint>
#include <decimal.hh>
#include <epoch_frame/datetime.h>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace epoch_metadata {
/// The BarAttribute struct is used to represent different types of
/// bar attributes such as "Open", "High", "Low", "Close", "Volume", "Ask",
/// "Bid", "AskVolume", and "BidVolume".
struct BarAttribute {
  enum Type : uint8_t {
    Open = 0,
    High = 1,
    Low = 2,
    Close = 3,
    Volume = 4,
    Ask = 5,
    Bid = 6,
    AskVolume = 7,
    BidVolume = 8,
    Price = 9,
    Underlying = 10,
    Delta = 11,
    Gamma = 12,
    Vega = 13,
    Theta = 14,
    Rho = 15,
    ImpliedVolatility = 16,
    OpenInterest = 17,
    TimeStamp = 18,
    Contract = 19
  };

  template <Type type>
  using GetTypeT = std::conditional_t<
      type == Contract, std::string,
      std::conditional_t<type == TimeStamp, int64_t, double>>;

  explicit BarAttribute(std::string const &p)
      : m_attribute(p), m_type(getType(p)) {}

  explicit BarAttribute(const char *p)
      : m_attribute(p), m_type(getType(std::string(p))) {}

  explicit BarAttribute(Type ts) : m_attribute(fromType(ts)), m_type(ts) {}

  static bool isValidBarAttribute(std::string const &);

  inline std::string operator()() const { return m_attribute; }

  BarAttribute &operator=(std::string const &_ts);

  BarAttribute &operator=(const char *_ts);

  Type getType(std::string const &x);

  static std::string fromType(Type type);

  static Type GetType(std::string const &barAttributeName) {
    return attribute_map.at(barAttributeName);
  }

  [[nodiscard]] inline Type GetType() const { return m_type; }

private:
  std::string m_attribute;
  Type m_type;

  static std::map<std::string, Type> initAttribute_map();

  static std::map<Type, std::string> initattribute_map_reverse();

  inline static std::map<std::string, Type> attribute_map{initAttribute_map()};

  inline static std::map<Type, std::string> attribute_map_reverse{
      initattribute_map_reverse()};
};

using BarAttributes = std::vector<BarAttribute>;

struct EpochStratifyXConstants {

  static const EpochStratifyXConstants &instance() {
    static EpochStratifyXConstants instance;
    return instance;
  }

  const BarAttribute OPEN{"o"};
  const BarAttribute CLOSE{"c"};
  const BarAttribute HIGH{"h"};
  const BarAttribute LOW{"l"};
  const BarAttribute ASK{"ap"};
  const BarAttribute BID{"bp"};
  const BarAttribute VOLUME{"v"};
  const BarAttribute ASK_VOLUME{"av"};
  const BarAttribute BID_VOLUME{"bv"};
  const BarAttribute PRICE{"p"};
  const BarAttribute CONTRACT{"s"};
  const BarAttribute OPEN_INTEREST{"oi"};
  const BarAttribute IV{"iv"};
  const BarAttribute DELTA{"delta"};
  const BarAttribute GAMMA{"gamma"};
  const BarAttribute VEGA{"vega"};
  const BarAttribute THETA{"theta"};
  const BarAttribute RHO{"rho"};
  const BarAttribute TIMESTAMP{"t"};

  const std::filesystem::path HOME{secure_getenv("HOME")};
  const epoch_metadata::TimeFrame DAILY_FREQUENCY{
      epoch_frame::factory::offset::days(1)};
  const epoch_metadata::TimeFrame MINUTE_FREQUENCY{
      epoch_frame::factory::offset::minutes(1)};

  const decimal::Decimal STOCK_IM_PERCENTAGE = decimal::Decimal("0.5");
  const decimal::Decimal SHORT_STOCK_MM_PERCENTAGE = decimal::Decimal("0.30");
  const decimal::Decimal LONG_STOCK_MM_PERCENTAGE = decimal::Decimal("0.25");
  const decimal::Decimal REGT_PERCENTAGE = decimal::Decimal("0.25");
  const decimal::Decimal FX_MARGIN_PERCENTAGE = decimal::Decimal("0.02");
  const decimal::Decimal FUTURES_IM_PERCENTAGE = decimal::Decimal("0.12");
  const decimal::Decimal FUTURES_MM_PERCENTAGE = decimal::Decimal("0.10");

  const decimal::Decimal zero = decimal::Decimal("0");
  const decimal::Decimal twoDecimalPlaces = decimal::Decimal("0.01");
  const decimal::Decimal fourDecimalPlaces = decimal::Decimal("0.0001");
  const decimal::Decimal nan = decimal::Decimal("NaN");
  const decimal::Decimal positiveInfinity = decimal::Decimal("+Inf");
  const decimal::Decimal negativeInfinity = decimal::Decimal("-Inf");
  const decimal::Decimal epsilon =
      decimal::Decimal(std::to_string(std::numeric_limits<double>::epsilon()));
  const decimal::Decimal quarter{"0.25"};
  const decimal::Decimal _16Point67{"16.67"};
  const decimal::Decimal five{"5.00"};
  const decimal::Decimal _2Point5{"2.50"};
  const decimal::Decimal point3{"0.3"};
  const decimal::Decimal point5{"0.5"};
  const decimal::Decimal one{"1.0"};
  const decimal::Decimal point4{"0.4"};
  const decimal::Decimal point33{"0.33"};

  const decimal::Decimal tenPercent = decimal::Decimal("0.1");
  const decimal::Decimal twentyPercent = decimal::Decimal("0.2");
};
} // namespace epoch_metadata