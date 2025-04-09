//
// Created by adesola on 12/16/24.
//
#include "transforms/registration.h"
#include "transforms/registry.h"
#include <catch.hpp>
#include "../common.h"

TEST_CASE("Transform MetaData Total Count is Correct") {
  using namespace epoch_metadata::transforms;
  RegisterTransformMetadata(epoch_metadata::DEFAULT_YAML_LOADER);

  REQUIRE(ITransformRegistry::GetInstance().GetMetaData().size() == 140);
}
