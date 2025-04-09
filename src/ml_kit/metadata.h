//
// Created by dewe on 9/9/24.
//

#pragma once
#include "../metadata_options.h"

CREATE_ENUM(RegistryClass, Transformer, Classifier, RegressionEstimator, Metric,
            CrossValidator, DLB_LossFunction, DLB_Optimizer, DLB_LRScheduler);

namespace epoch_metadata::ml_kit {
struct MLKitMetaData {
  std::string id;
  std::string name;
  RegistryClass class_;
  MetaDataOptionList args{};
  std::string desc{};
};
} // namespace epoch_metadata::ml_kit
