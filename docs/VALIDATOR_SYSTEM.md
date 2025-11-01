# EpochFlow Validator System Implementation

## Summary

Implemented a modular, extensible validation system for EpochFlow transforms with special requirements. The system provides compile-time validation for VARARGS transforms without modifying core compiler logic.

## What Was Built

### Core System

1. **Validator Interface** (`special_node_validator.h/cpp`)
   - `ISpecialNodeValidator` interface for all validators
   - `SpecialNodeValidatorRegistry` singleton for O(1) validator lookup
   - `REGISTER_SPECIAL_VALIDATOR` macro for auto-registration
   - `ValidationContext` struct with full compilation context

2. **Concrete Validators**
   - `FirstNonNullValidator` - validates `first_non_null` has ≥1 input
   - `ConditionalSelectValidator` - validates `conditional_select` has ≥2 inputs and Boolean conditions

3. **Compiler Integration**
   - Modified `NodeBuilder::WireInputs()` to call validators
   - Runs before input wiring to catch errors early
   - Clean error messages propagated through `ThrowError()`

### Test Infrastructure

1. **Test Cases** (5 new test directories)
   - `first_non_null_no_inputs` - Error: no inputs
   - `conditional_select_insufficient_inputs` - Error: only 1 input
   - `conditional_select_invalid_condition_type` - Error: non-boolean condition
   - `first_non_null_valid` - Success: 3 inputs
   - `conditional_select_valid` - Success: multiple condition/value pairs

2. **Test Scripts**
   - `test_validators.py` - Python demonstration script
   - `test_validators_interactive.sh` - Interactive bash test with colored output
   - Both show test cases and expected outcomes

3. **Documentation**
   - `validators/README.md` - Complete validator system documentation
   - Examples for adding new validators
   - Integration guide

## Architecture

```
Transform Compilation Flow:
┌─────────────────┐
│  EpochFlow DSL  │
│  Python Script  │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  AST Compiler   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Node Builder   │
│  WireInputs()   │
└────────┬────────┘
         │
         ▼
┌──────────────────────────┐
│ VARARGS Transform?       │──No──▶ Continue wiring
│ (allowMultiple=true)     │
└────────┬─────────────────┘
         │ Yes
         ▼
┌──────────────────────────┐
│ Validator Registry       │
│ O(1) lookup by name      │
└────────┬─────────────────┘
         │
         ▼
┌──────────────────────────┐
│ Validator found?         │──No──▶ Continue wiring
└────────┬─────────────────┘
         │ Yes
         ▼
┌──────────────────────────┐
│ Run Validator            │
│ ValidateInputs(ctx)      │
└────────┬─────────────────┘
         │
    ┌────┴────┐
    │         │
 Valid?    Invalid?
    │         │
    ▼         ▼
Continue   Throw Error
 Wiring    (Compilation fails)
```

## Files Created

```
src/epochflow/compiler/validators/
├── special_node_validator.h         # Interface + Registry
├── special_node_validator.cpp       # Registry implementation
├── first_non_null_validator.h       # FirstNonNull validator
├── first_non_null_validator.cpp
├── conditional_select_validator.h   # ConditionalSelect validator
├── conditional_select_validator.cpp
└── README.md                        # Documentation

test/epochflow/
├── test_validators.py               # Python demo script
├── test_validators_interactive.sh   # Interactive bash script
└── compiler/test_cases/
    ├── first_non_null_no_inputs/
    ├── conditional_select_insufficient_inputs/
    ├── conditional_select_invalid_condition_type/
    ├── first_non_null_valid/
    └── conditional_select_valid/
```

## Files Modified

```
src/epochflow/compiler/node_builder.cpp
  - Added validator call in WireInputs()
  - Runs for all VARARGS transforms
  - ~15 lines added

src/epochflow/CMakeLists.txt
  - Added 3 validator source files
  - ~3 lines added
```

## Usage Example

### Adding a New Validator

```cpp
// 1. Create header
class MyValidator : public ISpecialNodeValidator {
    void ValidateInputs(const ValidationContext& ctx) const override;
    std::string GetName() const override { return "MyValidator"; }
};

// 2. Implement validation
void MyValidator::ValidateInputs(const ValidationContext& ctx) const {
    if (ctx.args.size() != 3) {
        throw std::runtime_error("'my_transform' requires 3 inputs");
    }
}

// 3. Auto-register
REGISTER_SPECIAL_VALIDATOR("my_transform", MyValidator)
```

That's it! No changes to compiler core needed.

## Key Benefits

1. **Modular Design**
   - Each validator is self-contained
   - No if/else chains in compiler
   - Clean separation of concerns

2. **Zero Runtime Cost**
   - O(1) registry lookup
   - Compile-time validation only
   - No performance impact

3. **Developer Experience**
   - Clear error messages at compile time
   - Errors caught before execution
   - Easy to add new validators

4. **Extensibility**
   - Just implement interface
   - Auto-registration via macro
   - No core compiler modifications

## Validation Examples

### first_non_null

```python
# ❌ Error: No inputs
result = first_non_null()()
# Compile error: 'first_non_null' requires at least 1 input

# ✅ Valid: Multiple inputs
result = first_non_null()(primary, backup, default)
```

### conditional_select

```python
# ❌ Error: Only 1 input
result = conditional_select()(condition)
# Compile error: requires at least 2 inputs (condition, value)

# ❌ Error: Non-boolean condition
result = conditional_select()(src.c, 10, src.o, 20)
# Compile error: input at position 0 must be Boolean, got Decimal

# ✅ Valid: Alternating conditions and values
result = conditional_select()(
    oversold, "oversold",
    overbought, "overbought",
    "neutral"  # default
)
```

## Testing

### Run Tests

```bash
# All compiler tests
cmake-build-debug/bin/epoch_metadata_test "*EpochFlow Compiler*"

# Interactive demo
./test/epochflow/test_validators_interactive.sh

# Python demo
python3 test/epochflow/test_validators.py
```

### Test Results

- ✅ All validators compile successfully
- ✅ Error cases properly caught at compile time
- ✅ Valid cases compile without errors
- ✅ Clear, actionable error messages

## Future Extensions

The system is ready for additional validators:

1. **SQL Validators** - Query structure, output counts
2. **Report Validators** - Configuration validation
3. **Chart Validators** - Parameter validation
4. **Time-Frame Validators** - Compatibility checks
5. **Cross-Sectional Validators** - Multi-asset requirements

Each requires only:
1. Create validator class
2. Implement `ValidateInputs()`
3. Use `REGISTER_SPECIAL_VALIDATOR` macro

## Performance Impact

- **Compile Time**: Negligible (O(1) hash lookup)
- **Runtime**: Zero (validation happens at compile time)
- **Memory**: Minimal (small registry of validator pointers)

## Conclusion

The validator system provides a scalable, maintainable solution for compile-time validation of transforms with special requirements. It integrates cleanly with the existing compiler infrastructure while remaining completely extensible for future needs.
