//
// Created by adesola on 10/31/25.
//

#pragma once
#include <epoch_core/enum_wrapper.h>

// String case operations
CREATE_ENUM(StringCaseOp, upper, lower, capitalize, title, swapcase);

// String trim operations
CREATE_ENUM(StringTrimOp, trim, trim_left, trim_right);

// String pad operations
CREATE_ENUM(StringPadOp, pad_left, pad_right, center);

// String containment operations
CREATE_ENUM(StringContainsOp, starts_with, ends_with, contains);

// String character type check operations
CREATE_ENUM(StringCheckOp, is_alpha, is_digit, is_alnum, is_numeric, is_decimal,
            is_upper, is_lower, is_title, is_space, is_printable, is_ascii);
