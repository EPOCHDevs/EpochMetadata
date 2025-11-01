//
// Created by Claude Code
// EpochFlow Special Node Validator Registry Implementation
//

#include "special_node_validator.h"

namespace epoch_stratifyx::epochflow
{
    SpecialNodeValidatorRegistry& SpecialNodeValidatorRegistry::Instance() {
        static SpecialNodeValidatorRegistry instance;
        return instance;
    }

    void SpecialNodeValidatorRegistry::Register(
        const std::string& transform_name,
        std::shared_ptr<ISpecialNodeValidator> validator) {
        validators_[transform_name] = validator;
    }

    bool SpecialNodeValidatorRegistry::HasValidator(
        const std::string& transform_name) const {
        return validators_.find(transform_name) != validators_.end();
    }

    std::shared_ptr<ISpecialNodeValidator> SpecialNodeValidatorRegistry::GetValidator(
        const std::string& transform_name) const {
        auto it = validators_.find(transform_name);
        return (it != validators_.end()) ? it->second : nullptr;
    }

    void SpecialNodeValidatorRegistry::ValidateIfNeeded(
        const ValidationContext& ctx) const {
        if (auto validator = GetValidator(ctx.component_name)) {
            validator->ValidateInputs(ctx);
        }
    }

} // namespace epoch_stratifyx::epochflow
