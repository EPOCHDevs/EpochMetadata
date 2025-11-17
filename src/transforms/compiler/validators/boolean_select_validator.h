//
// Created by Claude Code
// Validator for boolean_select transform
//

#pragma once

#include "special_node_validator.h"

namespace epoch_script
{
    class BooleanSelectValidator : public ISpecialNodeValidator {
    public:
        void ValidateInputs(const ValidationContext& ctx) const override;
        std::string GetName() const override { return "BooleanSelectValidator"; }
    };

    // Explicit registration function
    void RegisterBooleanSelectValidator();

} // namespace epoch_script
