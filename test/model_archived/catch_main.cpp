//
// Created by adesola on 3/21/25.
//
#include <catch2/catch_session.hpp>
#include <epoch_frame/serialization.h>

int main(int argc, char *argv[]) {
  auto arrowComputeStatus = arrow::compute::Initialize();
  if (!arrowComputeStatus.ok()) {
    std::stringstream errorMsg;
    errorMsg << "arrow compute initialized failed: " << arrowComputeStatus
             << std::endl;
    throw std::runtime_error(errorMsg.str());
  }

  // your setup ...
  epoch_frame::ScopedS3 scoped_s3;
  const int result = Catch::Session().run(argc, argv);

  // your clean-up...

  return result;
}
