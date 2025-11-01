#pragma once

#include "generic_function.h"
#include "validation_error.h"
#include <expected>

namespace epochflow::strategy {

/// Validates a GenericFunction and returns any validation issues found
ValidationIssues ValidateGenericFunction(const GenericFunction &function,
                                         epoch_core::GenericFunctionType type);

/// Validates the type field of a GenericFunction
std::optional<MetaDataOptionList>
ValidateGenericFunctionType(const std::string &type,
                            epoch_core::GenericFunctionType functionType,
                            ValidationIssues &issues);

/// Validates the arguments mapping of a GenericFunction
void ValidateGenericFunctionArgs(MetaDataArgDefinitionMapping args,
                                 const MetaDataOptionList &options,
                                 std::string const &functionType,
                                 ValidationIssues &issues);

/// Optimization functions for GenericFunction
/// Optimizes the GenericFunction by applying default values, clamping values,
/// and other improvements
GenericFunction OptimizeGenericFunction(const GenericFunction &function,
                                        epoch_core::GenericFunctionType type);

/// Individual optimization phases for GenericFunction
void ApplyDefaultGenericFunctionOptions(GenericFunction &function,
                                        const MetaDataOptionList &options);
void ClampGenericFunctionOptionValues(GenericFunction &function,
                                      const MetaDataOptionList &options);

} // namespace epochflow::strategy
