# EpochScript Special Node Validator System

## Overview

The Special Node Validator system provides compile-time validation for transforms with special requirements (e.g., VARARGS transforms like `first_non_null` and `conditional_select`). This modular system allows validators to be registered per-transform without modifying the core compiler logic.

## Architecture

```
┌─────────────────────────────────────────────────┐
│          ISpecialNodeValidator                  │
│  (Interface for all validators)                 │
└───────────────┬─────────────────────────────────┘
                │
        ┌───────┴───────┐
        │               │
┌───────▼─────────┐  ┌──▼──────────────────┐
│  FirstNonNull   │  │ ConditionalSelect   │
│   Validator     │  │    Validator        │
└─────────────────┘  └─────────────────────┘
        │                    │
        └──────────┬─────────┘
                   │
        ┌──────────▼──────────────┐
        │ SpecialNodeValidator    │
        │      Registry           │
        │  (Singleton, O(1))      │
        └─────────────────────────┘
                   │
        ┌──────────▼──────────────┐
        │   NodeBuilder           │
        │  WireInputs() calls     │
        │  registry validation    │
        └─────────────────────────┘
```

## Key Components

### 1. Interface (`special_node_validator.h`)

```cpp
class ISpecialNodeValidator {
public:
    virtual ~ISpecialNodeValidator() = default;
    virtual void ValidateInputs(const ValidationContext& ctx) const = 0;
    virtual std::string GetName() const = 0;
};
```

### 2. Registry (`special_node_validator.h/cpp`)

Singleton registry that maps transform names to validators:

```cpp
class SpecialNodeValidatorRegistry {
    static SpecialNodeValidatorRegistry& Instance();
    void Register(const std::string& transform_name,
                 std::shared_ptr<ISpecialNodeValidator> validator);
    void ValidateIfNeeded(const ValidationContext& ctx) const;
};
```

### 3. Validators

Each validator implements `ISpecialNodeValidator`:

- **FirstNonNullValidator**: Validates at least 1 input for `first_non_null`
- **ConditionalSelectValidator**: Validates at least 2 inputs and boolean conditions for `conditional_select`

## Adding a New Validator

### Step 1: Create Header File

```cpp
// my_transform_validator.h
#pragma once
#include "special_node_validator.h"

namespace epoch_stratifyx::epoch_script {
    class MyTransformValidator : public ISpecialNodeValidator {
    public:
        void ValidateInputs(const ValidationContext& ctx) const override;
        std::string GetName() const override { return "MyTransformValidator"; }
    };
}
```

### Step 2: Implement Validation Logic

```cpp
// my_transform_validator.cpp
#include "my_transform_validator.h"
#include "../type_checker.h"

namespace epoch_stratifyx::epoch_script {
    void MyTransformValidator::ValidateInputs(const ValidationContext& ctx) const {
        // Example: require exactly 3 inputs
        if (ctx.args.size() != 3) {
            throw std::runtime_error(
                "'my_transform' requires exactly 3 inputs for node '" +
                ctx.target_node_id + "'");
        }

        // Example: first input must be Boolean
        auto& first_arg = ctx.args[0];
        DataType type = ctx.type_checker.GetNodeOutputType(
            first_arg.node_id, first_arg.handle);

        if (type != DataType::Boolean && type != DataType::Any) {
            throw std::runtime_error(
                "'my_transform' input 0 must be Boolean for node '" +
                ctx.target_node_id + "'");
        }
    }

    // Auto-register
    REGISTER_SPECIAL_VALIDATOR("my_transform", MyTransformValidator)
}
```

### Step 3: Add to CMakeLists.txt

```cmake
target_sources(epoch_metadata PRIVATE
    # ... existing sources ...
    compiler/validators/my_transform_validator.cpp
)
```

That's it! The validator will automatically be registered and called during compilation.

## Validation Context

Validators receive a `ValidationContext` with:

```cpp
struct ValidationContext {
    const std::vector<ValueHandle>& args;        // Positional arguments
    const std::unordered_map<std::string, ValueHandle>& kwargs;  // Keyword arguments
    const std::string& target_node_id;           // Node being created
    const std::string& component_name;           // Transform type
    TypeChecker& type_checker;                   // For type checking
    CompilationContext& context;                 // Full compilation context
};
```

## Example Validators

### FirstNonNullValidator

Validates that `first_non_null` has at least 1 input:

```cpp
void FirstNonNullValidator::ValidateInputs(const ValidationContext& ctx) const {
    if (ctx.args.empty()) {
        throw std::runtime_error(
            "'first_non_null' requires at least 1 input for node '" +
            ctx.target_node_id + "'");
    }
}
```

### ConditionalSelectValidator

Validates that `conditional_select` has:
- At least 2 inputs (condition + value minimum)
- Even-indexed inputs are Boolean (conditions)

```cpp
void ConditionalSelectValidator::ValidateInputs(const ValidationContext& ctx) const {
    if (ctx.args.size() < 2) {
        throw std::runtime_error(
            "'conditional_select' requires at least 2 inputs (condition, value) for node '" +
            ctx.target_node_id + "'");
    }

    // Validate alternating boolean conditions at even indices
    size_t num_pairs = ctx.args.size() / 2;
    for (size_t i = 0; i < num_pairs; i++) {
        size_t cond_idx = i * 2;
        const auto& cond_handle = ctx.args[cond_idx];
        DataType cond_type = ctx.type_checker.GetNodeOutputType(
            cond_handle.node_id, cond_handle.handle);

        if (cond_type != DataType::Boolean && cond_type != DataType::Any) {
            throw std::runtime_error(
                "'conditional_select' input at position " + std::to_string(cond_idx) +
                " must be Boolean (condition) for node '" + ctx.target_node_id +
                "', got " + TypeChecker::DataTypeToString(cond_type));
        }
    }
}
```

## Test Cases

Test cases are in `test/epoch_script/compiler/test_cases/`:

### Error Cases (Should Fail Compilation)
- `first_non_null_no_inputs` - No inputs provided
- `conditional_select_insufficient_inputs` - Only 1 input (need at least 2)
- `conditional_select_invalid_condition_type` - Non-boolean condition

### Valid Cases (Should Compile Successfully)
- `first_non_null_valid` - 3 compatible inputs
- `conditional_select_valid` - Multiple condition/value pairs with default

## Running Tests

```bash
# Run all compiler tests
cmake-build-debug/bin/epoch_metadata_test "*EpochScript Compiler*"

# Interactive validator demonstration
test/epoch_script/test_validators_interactive.sh

# Python demonstration script
python3 test/epoch_script/test_validators.py
```

## Benefits

1. **Modular**: Each validator is self-contained
2. **Scalable**: Adding validators doesn't modify core compiler
3. **Fast**: O(1) registry lookup, zero runtime cost
4. **Clear Errors**: Precise error messages at compile time
5. **Extensible**: Just implement interface and register

## Integration with Compiler

The validator system is integrated into `NodeBuilder::WireInputs()`:

```cpp
// Check if last input allows multiple connections (variadic inputs)
bool last_input_allows_multi = false;
if (!inputs.empty()) {
    last_input_allows_multi = inputs.back().allowMultipleConnections;
}

// Special node validation (VARARGS-specific rules)
if (last_input_allows_multi) {
    try {
        ValidationContext val_ctx{
            args, kwargs, target_node_id, component_name,
            type_checker_, context_
        };
        SpecialNodeValidatorRegistry::Instance().ValidateIfNeeded(val_ctx);
    } catch (const std::exception& e) {
        ThrowError(e.what());
    }
}
```

This runs **before** input wiring, catching errors early with clear messages.

## Future Extensions

Potential validators to add:

1. **SQL Query Validators**: Validate query structure, output counts
2. **Report Validators**: Validate report configuration
3. **Chart Validators**: Validate chart parameters
4. **Cross-Sectional Validators**: Validate multi-asset requirements
5. **Time-Frame Validators**: Validate time-frame compatibility

Each can be added by simply creating a new validator class and registering it!
