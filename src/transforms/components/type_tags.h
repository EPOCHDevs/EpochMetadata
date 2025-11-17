#pragma once
//
// Common type tag classes for typed transforms
// These serve as compile-time type discriminators
//

namespace epoch_script::transform {

// Type tags for the 4 supported data types
// Naming convention: {Type}Type
struct StringType {};
struct NumberType {};
struct BooleanType {};
struct TimestampType {};

} // namespace epoch_script::transform
