//
// Created by Claude Code
// Validator for first_non_null transform
//

#pragma once

#include "special_node_validator.h"

namespace epochflow
{
    class FirstNonNullValidator : public ISpecialNodeValidator {
    public:
        void ValidateInputs(const ValidationContext& ctx) const override;
        std::string GetName() const override { return "FirstNonNullValidator"; }
    };

} // namespace epochflow
