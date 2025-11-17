#pragma once
//
// Created by dewe on 6/18/23.
//
#include "adjustments/adjustments.h"
#include "epoch_frame/scalar.h"
#include "icontinuations.h"
#include "roll_method/last_trading_day.h"

namespace epoch_script::data {
using namespace futures;

std::unique_ptr<futures::RolloverMethodBase> MakeRolloverMethod(RolloverType,
                                                                int offset);

std::unique_ptr<futures::AdjustmentMethodBase>
    MakeAdjustmentMethod(AdjustmentType);

struct ContractView {
  std::string symbol;
  std::optional<epoch_frame::Date> expiryDate;

  std::strong_ordering operator<=>(const ContractView &other) const {
    if (!expiryDate && !other.expiryDate) {
      return std::strong_ordering::equal;
    }
    return *expiryDate <=> *other.expiryDate;
  }
};

struct ContractViewPair {
  ContractView front;
  ContractView back;
};

struct IFuturesContinuation {
  virtual ~IFuturesContinuation() = default;
  virtual epoch_frame::DataFrame
  BuildBars(const epoch_frame::DataFrame &df) const = 0;
};
using IFuturesContinuationPtr = std::unique_ptr<IFuturesContinuation>;


class FuturesContinuation : public IFuturesContinuation {
public:

  ADD_MAKER(FuturesContinuation)

  explicit FuturesContinuation(std::unique_ptr<RolloverMethodBase> rollover,
                               std::unique_ptr<AdjustmentMethodBase> adjustment)
      : m_rolloverMethod(std::move(rollover)),
        m_adjustmentMethod(std::move(adjustment)) {}

  epoch_frame::DataFrame BuildBars(const epoch_frame::DataFrame &df) const;

  RolloverType GetRolloverType() const noexcept {
    return m_rolloverMethod->GetType();
  }

  int GetOffset() const noexcept { return m_rolloverMethod->GetOffset(); }

  AdjustmentType GetAdjustmentType() const noexcept {
    return m_adjustmentMethod->GetType();
  }

private:
  std::unique_ptr<RolloverMethodBase> m_rolloverMethod;
  std::unique_ptr<AdjustmentMethodBase> m_adjustmentMethod;
};

class FuturesContinuationConstructor final
    : public IFuturesContinuationConstructor {
public:
  using InputType = std::unique_ptr<FuturesContinuation>;

  ADD_MAKER(FuturesContinuationConstructor)

  FuturesContinuationConstructor(IFuturesContinuationPtr input);

  AssetDataFrameMap Build(AssetDataFrameMap const &) const override final;

private:
  IFuturesContinuationPtr m_barConstructors;
};

} // namespace epoch_script::data