// futures_continuation_tests.cpp
#include <catch2/catch_all.hpp>
#include <catch2/generators/catch_generators_adapters.hpp>
#include <trompeloeil.hpp>

#include "data/futures_continuation/continuations.h"
#include <epoch_frame/common.h>
#include <epoch_frame/factory/array_factory.h>
#include <epoch_frame/factory/dataframe_factory.h>
#include <epoch_frame/factory/index_factory.h>
#include <epoch_frame/series.h>

using namespace epoch_script;
using namespace epoch_script::data;
using namespace epoch_script::futures;
using namespace epoch_frame;
using namespace epoch_frame::factory;
using namespace std::string_literals;

/*──────────────────────────── helpers ────────────────────────────*/
namespace {
const auto &C = epoch_script::EpochStratifyXConstants::instance();

/* tiny 2‑row DF; contracts supplied by caller */
DataFrame make_df(std::vector<std::string> const &contracts,
                  std::vector<DateTime> const &dates) {
  FuturesConstructedBars bars;
  std::mt19937_64 rng(123456789);
  for (auto const &[contract, date] :
       std::ranges::views::zip(contracts, dates)) {
    std::uniform_real_distribution<double> dist(100, 1000);
    bars.h.emplace_back(dist(rng));

    dist = std::uniform_real_distribution<double>(0, bars.h.back());
    bars.l.emplace_back(dist(rng));

    dist = std::uniform_real_distribution<double>(bars.l.back(), bars.h.back());
    bars.c.emplace_back(dist(rng));
    bars.o.emplace_back(dist(rng));

    dist = std::uniform_real_distribution<double>(0, 1000);
    bars.v.emplace_back(dist(rng));
    bars.oi.emplace_back(dist(rng));

    bars.t.emplace_back(date.timestamp().value);
  }
  return make_dataframe(index::make_datetime_index(bars.t),
                        {array::make_array(std::vector{contracts}),
                         array::make_array(std::vector<double>{bars.o}),
                         array::make_array(std::vector<double>{bars.h}),
                         array::make_array(std::vector<double>{bars.l}),
                         array::make_array(std::vector<double>{bars.c}),
                         array::make_array(std::vector<double>{bars.v}),
                         array::make_array(std::vector<double>{bars.oi})},
                        {C.CONTRACT(), C.OPEN(), C.HIGH(), C.LOW(), C.CLOSE(),
                         C.VOLUME(), C.OPEN_INTEREST()});
}

/* helper to wire mocks into a real continuation */
std::unique_ptr<FuturesContinuation>
make_cont(std::unique_ptr<RolloverMethodBase> r,
          std::unique_ptr<AdjustmentMethodBase> a) {
  return std::make_unique<FuturesContinuation>(std::move(r), std::move(a));
}

std::vector<DateTime> make_dates(std::vector<int64_t> const &dates) {
  std::vector<DateTime> out;
  for (auto const &d : dates) {
    out.emplace_back(DateTime::fromtimestamp(d));
  }
  return out;
}

} // namespace

/*────────────────────────── Trompeloeil mocks ──────────────────────────*/
struct MockRoll : RolloverMethodBase {
  using Input = RolloverMethodBase::Input;
  explicit MockRoll(int off = 0) : RolloverMethodBase(off) {}
  MAKE_CONST_MOCK1(IsRollDate, bool(Input const &), override);
  MAKE_CONST_MOCK0(GetType, epoch_core::RolloverType(), override);
};

struct MockAdj : AdjustmentMethodBase {
  MAKE_MOCK3(AdjustContracts,
             DataFrame(FuturesConstructedBars const &,
                       FuturesConstructedBars const &,
                       std::vector<int64_t> const &),
             override);

  MAKE_CONST_MOCK0(GetType, epoch_core::AdjustmentType(), override);
};
/*──────────────────────────── tests ─────────────────────────────*/
namespace {
struct Config {
  std::string caseName;
  std::vector<std::string> contracts;
  std::vector<DateTime> dates;
  std::vector<bool> expectedRolls;
  std::vector<std::string> expectedFrontContracts;
  std::vector<std::string> expectedBackContracts;
  std::vector<int64_t> expectedRolloverPoints;
  std::vector<DateTime> expectedFrontDates;
  std::vector<DateTime> expectedBackDates;
  bool expectAdjustments{false};
  bool expectThrow{false};
};

inline std::vector<Config> BuildConfigs() {
  Config cfg1{"Single contract – no roll",
              {"CLZ30", "CLZ30"},
              {"2025-01-01"__date, "2025-01-02"__date},
              {},
              {"CLZ30", "CLZ30"},
              {"CLZ30", "CLZ30"},
              {},
              {},
              {},
              false,
              false};

  Config cfg2{"Single Boundary Z Z F F",
              {"CLZ25", "CLZ25", "CLF26", "CLF26"},
              {"2025-01-01"__date, "2025-01-02"__date, "2025-01-03"__date,
               "2025-01-04"__date},
              {false, true},
              {"CLZ25", "CLF26", "CLF26", "CLF26"},
              {"CLF26", "CLF26", "CLF26", "CLF26"},
              {1},
              {},
              {},
              true,
              false};

  Config intra{
      "Intraday 1‑min Z→F roll",
      {"CLZ25", "CLZ25", "CLF26", "CLF26"},
      {"2025-01-01 09:00:00"__dt, "2025-01-01 09:01:00"__dt,
       "2025-01-01 09:02:00"__dt, "2025-01-01 09:03:00"__dt},
      {true},                               // IsRollDate is called once
      {"CLF26", "CLF26", "CLF26", "CLF26"}, // front
      {"CLF26", "CLF26", "CLF26", "CLF26"}, // back
      {0},                                  // roll on first timestamp
      {},
      {},   // dates default
      true, // expectAdjustments
      false // no throw
  };

  Config multiContractsPerDay{"Same day mixed symbols (Z,F)",
                              {"CLZ25", "CLF26", "CLZ25", "CLF26"},
                              {"2025-01-02"__date, "2025-01-02"__date,
                               "2025-01-03"__date, "2025-01-03"__date},
                              {true}, // IsRollDate check
                              {"CLF26", "CLF26"},
                              {"CLF26", "CLF26"},
                              {0},
                              {"2025-01-02"__date, "2025-01-03"__date},
                              {"2025-01-02"__date, "2025-01-03"__date},
                              true,
                              false};

  // Keep the longPath test case for testing with multiple contracts over time
  Config longPath{
      "Multiple contracts over many unique dates",
      {"CLZ25", "CLF26", "CLG26", "CLH26"}, // 4 contracts
      {"2025-01-02"__date, "2025-01-03"__date, "2025-01-04"__date,
       "2025-01-05"__date},                 // 4 unique dates
      {true, true, true},                   // Three roll checks
      {"CLF26", "CLG26", "CLH26", "CLH26"}, // Front contracts after each roll
      {"CLG26", "CLH26", "CLH26", "CLH26"}, // Back contracts after each roll
      {0, 1, 2},                            // Roll points
      {"2025-01-02"__date, "2025-01-03"__date, "2025-01-04"__date,
       "2025-01-05"__date}, // Expected front dates
      {"2025-01-02"__date, "2025-01-03"__date, "2025-01-04"__date,
       "2025-01-05"__date}, // Expected back dates
      true,                 // expectAdjustments
      false                 // no throw
  };

  return {cfg1, cfg2, intra, multiContractsPerDay, longPath};
}
} // namespace

TEST_CASE("Continuation – matrix of scenarios", "[continuation][matrix]") {
  static const auto configs = BuildConfigs();
  const auto &cfg = GENERATE(from_range(configs));

  DYNAMIC_SECTION(cfg.caseName) {
    auto roll = std::make_unique<MockRoll>();
    auto adj = std::make_unique<MockAdj>();

    std::vector<std::unique_ptr<trompeloeil::expectation>> expectations;
    trompeloeil::sequence seq;
    for (bool isRoll : cfg.expectedRolls) {
      expectations.emplace_back(
          NAMED_REQUIRE_CALL(*roll, IsRollDate(trompeloeil::_))
              .IN_SEQUENCE(seq)
              .RETURN(isRoll)
              .TIMES(1));
    }

    // default dates vectors if not supplied
    auto expectedFrontDates =
        cfg.expectedFrontDates.empty() ? cfg.dates : cfg.expectedFrontDates;
    auto expectedBackDates =
        cfg.expectedBackDates.empty() ? cfg.dates : cfg.expectedBackDates;

    if (cfg.expectAdjustments) {
      expectations.emplace_back(
          NAMED_REQUIRE_CALL(*adj,
                             AdjustContracts(trompeloeil::_, trompeloeil::_,
                                             cfg.expectedRolloverPoints))
              .LR_SIDE_EFFECT(REQUIRE(make_dates(_1.t) == expectedFrontDates);
                              REQUIRE(_1.s == cfg.expectedFrontContracts);
                              REQUIRE(make_dates(_2.t) == expectedBackDates);
                              REQUIRE(_2.s == cfg.expectedBackContracts);)
              .RETURN(DataFrame{})
              .TIMES(1));
    } else {
      expectations.emplace_back(NAMED_FORBID_CALL(
          *adj,
          AdjustContracts(trompeloeil::_, trompeloeil::_, trompeloeil::_)));
    }

    auto cont = make_cont(std::move(roll), std::move(adj));
    auto input = make_df(cfg.contracts, cfg.dates);

    if (cfg.expectThrow) {
      REQUIRE_THROWS(cont->BuildBars(input));
    } else {
      REQUIRE_NOTHROW(cont->BuildBars(input));
    }
  }
}

/*──────── edge / error cases ────────*/
TEST_CASE("Empty dataframe -> empty result & no calls") {
  auto roll = std::make_unique<MockRoll>();
  auto adj = std::make_unique<MockAdj>();
  FORBID_CALL(*roll, IsRollDate(trompeloeil::_));
  FORBID_CALL(*adj,
              AdjustContracts(trompeloeil::_, trompeloeil::_, trompeloeil::_));
  auto cont = make_cont(std::move(roll), std::move(adj));
  DataFrame empty;
  REQUIRE(cont->BuildBars(empty).empty());
}

TEST_CASE("Single contract – no roll, same size output") {
  auto cont =
      make_cont(std::make_unique<MockRoll>(), std::make_unique<MockAdj>());
  auto df =
      make_df({"CLZ30", "CLZ30"}, {"2025-01-01"__date, "2025-01-02"__date});
  auto result = cont->BuildBars(df);

  INFO(result);
  REQUIRE(cont->BuildBars(result).equals(result));
}