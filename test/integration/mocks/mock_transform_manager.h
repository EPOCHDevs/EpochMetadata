#pragma once

#include "transforms/runtime/transform_manager/itransform_manager.h"
#include <epochflow/transforms/core/itransform.h>
#include <epochflow/transforms/core/transform_configuration.h>
#include <memory>
#include <vector>

namespace epoch_flow::runtime::test {

/**
 * @brief Simple mock transform manager for testing
 *
 * This is a basic implementation of ITransformManager that holds a vector
 * of mock transforms. It doesn't need Trompeloeil since we're just wrapping
 * existing mocks.
 *
 * Example usage:
 * @code
 * auto manager = std::make_unique<MockTransformManager>();
 * auto mock1 = CreateSimpleMockTransform("transform1", dailyTF);
 * auto mock2 = CreateSimpleMockTransform("transform2", dailyTF);
 * manager->AddTransform(std::move(mock1));
 * manager->AddTransform(std::move(mock2));
 *
 * auto orchestrator = DataFlowRuntimeOrchestrator(assets, std::move(manager));
 * @endcode
 */
class MockTransformManager : public epoch_flow::runtime::ITransformManager {
public:
    MockTransformManager() = default;

    /**
     * @brief Add a transform to the manager
     * @param transform Transform instance to add
     */
    void AddTransform(std::unique_ptr<epochflow::transform::ITransformBase> transform) {
        std::cerr << "DEBUG MockTransformManager: AddTransform called (before: " << m_transforms.size() << " transforms)\n";
        // Just store transform instance - orchestrator will query it via interface methods
        m_transforms.push_back(std::move(transform));
        std::cerr << "DEBUG MockTransformManager: AddTransform done (after: " << m_transforms.size() << " transforms)\n";
    }

    // ITransformManager interface implementation
    [[nodiscard]] const epochflow::transform::TransformConfiguration*
    GetExecutor() const override {
        throw std::runtime_error("GetExecutor() not used in tests - orchestrator uses interface methods");
    }

    [[nodiscard]] const std::vector<epoch_flow::runtime::TransformConfigurationPtr>*
    GetTransforms() const override {
        throw std::runtime_error("GetTransforms() not used in tests - orchestrator uses interface methods");
    }

    [[nodiscard]] const epochflow::transform::TransformConfiguration*
    GetTransformConfigurationById(const std::string& name) const override {
        throw std::runtime_error("GetTransformConfigurationById() not used in tests - orchestrator uses interface methods");
    }

    [[nodiscard]] std::vector<std::unique_ptr<epochflow::transform::ITransformBase>>
    BuildTransforms() const override {
        std::cerr << "DEBUG MockTransformManager: BuildTransforms() called with " << m_transforms.size() << " transforms\n";
        auto result = std::move(m_transforms);
        std::cerr << "DEBUG MockTransformManager: Returning " << result.size() << " transforms, m_transforms now has " << m_transforms.size() << "\n";
        return result;
    }

private:
    mutable std::vector<std::unique_ptr<epochflow::transform::ITransformBase>> m_transforms;
};

/**
 * @brief Helper to create a MockTransformManager from a vector of transforms
 *
 * @param transforms Vector of transform unique_ptrs to add to the manager
 * @return Ready-to-use mock transform manager
 */
inline std::unique_ptr<MockTransformManager> CreateMockTransformManager(
    std::vector<std::unique_ptr<epochflow::transform::ITransformBase>> transforms) {
    auto manager = std::make_unique<MockTransformManager>();
    for (auto& transform : transforms) {
        manager->AddTransform(std::move(transform));
    }
    return manager;
}

} // namespace epoch_flow::runtime::test
