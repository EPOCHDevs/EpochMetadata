//
// Created by adesola on 4/24/25.
//

#pragma once
#include <epoch_script/transforms/runtime/transform_manager/itransform_manager.h>


namespace epoch_script {
  using ITransformManager = epoch_script::runtime::ITransformManager;
  using ITransformManagerPtr = std::unique_ptr<ITransformManager>;
  using TransformConfigurationPtr = epoch_script::runtime::TransformConfigurationPtr;

  void InitEpochScript();
}
