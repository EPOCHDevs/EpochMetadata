#pragma once

#include <../../include/epochflow/transforms/core/itransform.h>
#include <../../include/epochflow/transforms/core/transform_configuration.h>
#include <../../include/epochflow/transforms/core/registration.h>
#include <../../include/epochflow/transforms/core/transform_registry.h>
#include <memory>
#include <vector>

namespace epoch_flow::runtime::test {

/**
 * @brief Utility class for building transforms from configurations in tests
 *
 * This helper simplifies the creation of transform instances from configurations,
 * making it easier to construct DataFlowRuntimeOrchestrator instances in tests.
 */
class TransformBuilder {
public:
    /**
     * @brief Build transform instances from a list of configurations
     *
     * @param configs List of transform configurations
     * @return Vector of transform instances ready to be passed to DataFlowRuntimeOrchestrator
     */
    static std::vector<std::unique_ptr<epochflow::transform::ITransformBase>>
    BuildFromConfigurations(
        const epochflow::transform::TransformConfigurationList& configs) {

        std::vector<std::unique_ptr<epochflow::transform::ITransformBase>> transforms;
        transforms.reserve(configs.size());

        for (const auto& config : configs) {
            transforms.push_back(MAKE_TRANSFORM(config));
        }

        return transforms;
    }

    /**
     * @brief Build a single transform from a configuration
     *
     * @param config Transform configuration
     * @return Transform instance
     */
    static std::unique_ptr<epochflow::transform::ITransformBase>
    BuildFromConfiguration(const epochflow::transform::TransformConfiguration& config) {
        return MAKE_TRANSFORM(config);
    }
};

} // namespace epoch_flow::runtime::test
