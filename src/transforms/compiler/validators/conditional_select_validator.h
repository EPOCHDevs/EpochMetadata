//
// Created by Claude Code
// Validator for conditional_select transform
//

#pragma once

#include "special_node_validator.h"

namespace epoch_script
{
    class ConditionalSelectValidator : public ISpecialNodeValidator {
    public:
        void ValidateInputs(const ValidationContext& ctx) const override;
        std::string GetName() const override { return "ConditionalSelectValidator"; }
    };

} // namespace epoch_script
