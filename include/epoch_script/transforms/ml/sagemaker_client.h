#pragma once

#include <aws/core/Aws.h>
#include <aws/sagemaker-runtime/SageMakerRuntimeClient.h>
#include <memory>

namespace epoch_script::transform {

/**
 * @brief Singleton AWS SageMaker client manager
 *
 * Manages the lifecycle of AWS SDK and SageMaker Runtime client.
 * Initialized once per application, similar to S3DBManager pattern.
 *
 * Thread-safe singleton that:
 * - Initializes AWS SDK on first access
 * - Creates SageMaker Runtime client for us-west-2
 * - Uses AWS credentials from environment variables
 * - Automatically cleans up on application exit
 */
class SageMakerClientManager {
public:
  // Singleton accessor
  static const SageMakerClientManager *Instance() {
    static SageMakerClientManager instance;
    return &instance;
  }

  /**
   * @brief Get the SageMaker Runtime client
   * @return Pointer to the AWS SageMaker Runtime client
   */
  const Aws::SageMakerRuntime::SageMakerRuntimeClient *GetClient() const {
    return m_client.get();
  }

private:
  SageMakerClientManager();
  ~SageMakerClientManager();

  // Delete copy/move constructors
  SageMakerClientManager(const SageMakerClientManager &) = delete;
  SageMakerClientManager &operator=(const SageMakerClientManager &) = delete;
  SageMakerClientManager(SageMakerClientManager &&) = delete;
  SageMakerClientManager &operator=(SageMakerClientManager &&) = delete;

  std::unique_ptr<Aws::SDKOptions> m_sdk_options;
  std::unique_ptr<Aws::SageMakerRuntime::SageMakerRuntimeClient> m_client;
};

} // namespace epoch_script::transform
