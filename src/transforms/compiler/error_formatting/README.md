# Error Formatting System

This directory contains organized error formatters for the EpochScript compiler.

## Purpose

Provides consistent, well-structured error messages with rich context to help users debug their code quickly.

## Structure

- **error_formatter.h/cpp** - Base class with common formatting utilities
- **argument_error.h/cpp** - Argument count and input validation errors
- **type_error.h/cpp** - Type mismatch errors for functions and operators
- **component_error.h/cpp** - Component-related errors (unknown components, handles, etc.)
- **all_errors.h** - Convenience header to include all formatters

## Usage

### Example 1: Argument Count Error

```cpp
#include "error_formatting/argument_error.h"

// Instead of:
ThrowError("Too many positional inputs for 'node_3'");

// Use (with implicit string conversion):
ThrowError(error_formatting::ArgumentCountError(
    target_node_id, component_name,
    input_ids.size(), args.size(),
    input_ids, args
));
```

**Output:**
```
Too many positional inputs for 'node_3'
  Component: sma()
  Expected: 1 argument(s) [SLOT]
  Received: 3 argument(s) [src.c, number_1.result, bool_true_0.result]
(line 5, col 10)
```

### Example 2: Type Mismatch Error

```cpp
#include "error_formatting/type_error.h"

ThrowError(error_formatting::TypeMismatchError(
    component_name,
    error_formatting::TypeMismatchError::ArgumentKind::Keyword,
    "period",
    DataType::Integer,
    DataType::String,
    handle
));
```

**Output:**
```
Type error calling 'ema()'
  Keyword argument 'period':
    Expected type: Integer
    Received type: String
    Source: text_0.result
(line 3, col 15)
```

### Example 3: Unknown Component Error

```cpp
#include "error_formatting/component_error.h"

ThrowError(error_formatting::UnknownComponentError("smaa"));
```

### How It Works

All error formatters support:
1. **Implicit string conversion** - Can be passed directly to `ThrowError()`
2. **Stream operator** - Can use `std::cout << error;`
3. **Explicit Format()** - Can call `error.Format(line, col)` for custom location

**Output:**
```
Unknown component 'smaa()'
  This component is not registered or does not exist.
  Check the component name for typos or verify it's included in the system.
(line 2, col 5)
```

## Adding New Error Formatters

1. Create a new class inheriting from `ErrorFormatter`
2. Implement the `Format(int line, int col)` method
3. Add the new files to `CMakeLists.txt`
4. Include in `all_errors.h` if needed

## Design Principles

- **Consistency**: All errors follow the same structure
- **Context**: Provide enough information to understand the problem
- **Clarity**: Use simple language and avoid jargon
- **Actionability**: Help users understand how to fix the issue
