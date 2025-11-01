#pragma once

#include <epoch_metadata/transforms/itransform.h>
#include <epoch_metadata/transforms/transform_configuration.h>
#include <epoch_metadata/transforms/registration.h>
#include <epoch_metadata/transforms/transform_registry.h>
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
    static std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>>
    BuildFromConfigurations(
        const epoch_metadata::transform::TransformConfigurationList& configs) {

        std::vector<std::unique_ptr<epoch_metadata::transform::ITransformBase>> transforms;
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
    static std::unique_ptr<epoch_metadata::transform::ITransformBase>
    BuildFromConfiguration(const epoch_metadata::transform::TransformConfiguration& config) {
        return MAKE_TRANSFORM(config);
    }
};

} // namespace epoch_flow::runtime::test
