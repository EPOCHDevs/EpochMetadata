//
// Created by adesola on 12/16/24.
//
#include "transforms/registration.h"
#include <catch.hpp>


TEST_CASE("Transform MetaData Total Count is Correct")
{
    using namespace metadata::transforms;
    REQUIRE(ITransformRegistry::GetInstance().GetMetaData().size() == 140);
}
