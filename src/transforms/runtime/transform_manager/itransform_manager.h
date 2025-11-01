//
// Created by adesola on 8/2/24.
//

#pragma once
#include <memory>
#include <stdexcept>
#include <vector>


namespace epochflow::transform {
    class TransformConfiguration;
    class ITransformBase;
}

namespace epoch_flow::runtime {
    using TransformConfigurationPtr =
        std::unique_ptr<epochflow::transform::TransformConfiguration>;

    struct ITransformManager {
        virtual ~ITransformManager() = default;

        [[nodiscard]] virtual const epochflow::transform::TransformConfiguration* GetExecutor() const = 0;

        [[nodiscard]] virtual const std::vector<TransformConfigurationPtr> *
        GetTransforms() const = 0;

        [[nodiscard]] virtual const epochflow::transform::
            TransformConfiguration *
            GetTransformConfigurationById(std::string const &name) const = 0;

        // Build actual transform instances from configurations in dependency order
        [[nodiscard]] virtual std::vector<std::unique_ptr<epochflow::transform::ITransformBase>>
        BuildTransforms() const = 0;

        template <class T = epochflow::transform::TransformConfiguration>
        const T *GetTTransformConfigurationById(std::string const &name) const {
            if (const auto *casted =
                    static_cast<const T *>(GetTransformConfigurationById(name)))
                return casted;
            throw std::invalid_argument("Failed to cast info for '" + name +
                                        "' to the requested type.");
        }
    };

    using ITransformManagerPtr = std::unique_ptr<ITransformManager>;
} // namespace epoch_flow::runtime
