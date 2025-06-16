#pragma once

#include "generic_function.h"
#include "validation_error.h"
#include <expected>

namespace epoch_metadata::strategy {

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

} // namespace epoch_metadata::strategy
