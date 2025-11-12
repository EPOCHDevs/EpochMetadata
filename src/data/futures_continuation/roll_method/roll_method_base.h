#pragma once
//
// Created by dewe on 12/4/23.
//
#include "epoch_core/macros.h"
#include <epoch_script/strategy/enums.h>
#include "epoch_script/core/symbol.h"
#include "epoch_script/core/bar_attribute.h"
#include <epoch_core/common_utils.h>
#include <epoch_frame/dataframe.h>
#include <epoch_frame/index.h>
#include <epoch_data_sdk/model/asset/asset.hpp>
#include <epoch_data_sdk/model/asset/contract_spec.hpp>

namespace epoch_script::futures {
namespace asset = data_sdk::asset;
using epoch_core::RolloverType;
inline epoch_frame::Date GetContractExpiration(std::string const &contract) {
  return asset::ContractInfo::MakeFuturesContractInfo(contract)
      .GetExpirationDate();
}

// TODO: Add Compound epoch_core::RolloverType

struct RolloverMethodBase {
  struct Input {
    epoch_frame::DataFrame frontData;
    epoch_frame::DataFrame backData;
    epoch_frame::Date currentDate;

    bool operator==(const Input &other) const {
      return frontData.equals(other.frontData) &&
             backData.equals(other.backData) &&
             currentDate == other.currentDate;
    }
  };

  explicit RolloverMethodBase(int offset = 0) : m_offset(offset) {}

  inline int GetOffset() const { return m_offset; }
  virtual epoch_core::RolloverType GetType() const = 0;

  virtual bool IsRollDate(const Input &input) const = 0;

  static asset::ContractInfo GetContract(epoch_frame::DataFrame const &data) {
    AssertFromFormat(data.num_rows() > 0, "No data to get contract from");
    return asset::ContractInfo::MakeFuturesContractInfo(
        data.iloc(
                0,
                epoch_script::EpochStratifyXConstants::instance().CONTRACT())
            .repr());
  }

  virtual ~RolloverMethodBase() = default;

private:
  int m_offset{0};
};
} // namespace epoch_script::futures