//
// Created by Claude Code
// Validator for first_non_null transform
//
// Validates that first_non_null has at least 1 input.
// Type compatibility is handled by the existing type checker.
//

#include "first_non_null_validator.h"
#include "../type_checker.h"
#include <stdexcept>

namespace epoch_stratifyx::epochflow
{
    void FirstNonNullValidator::ValidateInputs(const ValidationContext& ctx) const {
        // Validate at least 1 input
        if (ctx.args.empty()) {
            throw std::runtime_error(
                "'first_non_null' requires at least 1 input for node '" +
                ctx.target_node_id + "'");
        }

        // Type compatibility is handled by existing type checker
        // All inputs should be compatible with each other (enforced by Arrow's coalesce)
    }

    // Auto-register this validator
    REGISTER_SPECIAL_VALIDATOR("first_non_null", FirstNonNullValidator)

} // namespace epoch_stratifyx::epochflow
