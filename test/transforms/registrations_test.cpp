//
// Created by adesola on 12/16/24.
//
#include "../common.h"
#include "epoch_metadata/transforms/registration.h"
#include "epoch_metadata/transforms/registry.h"
#include <catch.hpp>

TEST_CASE("Transform MetaData Total Count is Correct") {
  using namespace epoch_metadata::transforms;
  RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  REQUIRE(ITransformRegistry::GetInstance().GetMetaData().size() == 226);
}
