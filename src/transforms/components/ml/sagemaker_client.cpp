//
// AWS SageMaker Client Singleton Implementation
//

#include <epoch_script/transforms/ml/sagemaker_client.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <epoch_core/macros.h>
#include <spdlog/spdlog.h>

namespace epoch_script::transform {

SageMakerClientManager::SageMakerClientManager() {
  try {
    // Initialize AWS SDK
    m_sdk_options = std::make_unique<Aws::SDKOptions>();
    m_sdk_options->loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
    Aws::InitAPI(*m_sdk_options);

    // Configure client for us-west-2
    Aws::Client::ClientConfiguration client_config;
    client_config.region = "us-west-2";
    client_config.requestTimeoutMs = 30000;
    client_config.connectTimeoutMs = 5000;

    // Create SageMaker Runtime client
    // Will automatically use AWS credentials from environment variables:
    // - AWS_ACCESS_KEY_ID
    // - AWS_SECRET_ACCESS_KEY
    m_client = std::make_unique<Aws::SageMakerRuntime::SageMakerRuntimeClient>(
        client_config);

    // Verify client was created successfully
    AssertFromStream(m_client != nullptr,
                     "Failed to initialize SageMaker client");

    spdlog::info("AWS SageMaker client initialized successfully");
    spdlog::info("Region: us-west-2");

  } catch (const std::exception &e) {
    AssertFromStream(false,
                     fmt::format("Failed to initialize AWS SDK: {}", e.what()));
  }
}

SageMakerClientManager::~SageMakerClientManager() {
  // Shutdown AWS SDK on application exit
  if (m_sdk_options) {
    Aws::ShutdownAPI(*m_sdk_options);
    spdlog::debug("AWS SDK shutdown complete");
  }
}

} // namespace epoch_script::transform
