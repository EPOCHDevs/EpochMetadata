//
// Created by adesola on 3/21/25.
//
#include "catch2/catch_session.hpp"
#include "common.h"
#include "epoch_metadata/strategy/registration.h"

int main(int argc, char *argv[]) {
  epoch_metadata::strategy::RegisterStrategyMetadata(
      epoch_metadata::DEFAULT_YAML_LOADER,
      epoch_metadata::LoadAIGeneratedResources(
          epoch_metadata::AI_GENERATED_ALGORITHMS_DIR),
      epoch_metadata::LoadAIGeneratedResources(
          epoch_metadata::AI_GENERATED_STRATEGIES_DIR));
  // your setup ...
  int result = Catch::Session().run(argc, argv);

  // your clean-up...

  return result;
}
