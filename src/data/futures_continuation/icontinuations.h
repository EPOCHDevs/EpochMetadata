//
// Created by adesola on 7/31/24.
//

#pragma once
#include "data/aliases.h"

namespace epoch_script::data {
struct IFuturesContinuationConstructor {
  using Ptr = std::unique_ptr<IFuturesContinuationConstructor>;
  virtual ~IFuturesContinuationConstructor() = default;

  virtual AssetDataFrameMap Build(AssetDataFrameMap const &) const = 0;
};
} // namespace epoch_script::data