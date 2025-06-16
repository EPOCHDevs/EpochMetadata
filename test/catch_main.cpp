//
// Created by adesola on 3/21/25.
//
#include "common.h"
#include "catch2/catch_session.hpp"
#include "epoch_metadata/strategy/registration.h"

int main( int argc, char* argv[] ) {
    epoch_metadata::strategy::RegisterStrategyMetadata(epoch_metadata::DEFAULT_YAML_LOADER, epoch_metadata::LoadAIGeneratedStrategies());
    // your setup ...
    int result = Catch::Session().run( argc, argv );

    // your clean-up...

    return result;
}
