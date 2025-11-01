# Validator System Quick Start

## 🚀 Testing the Validators

### Run Interactive Demo
```bash
./test/epochflow/test_validators_interactive.sh
```

### Run Python Demo
```bash
python3 test/epochflow/test_validators.py
```

### Run Compiler Tests
```bash
cmake-build-debug/bin/epoch_metadata_test "*EpochFlow Compiler*"
```

## 📝 Example Usage

### first_non_null

```python
# ✅ Valid - Multiple fallback values
clean_price = first_non_null()(adjusted_close, raw_close, 0.0)

# ❌ Error - No inputs
result = first_non_null()()
# Error: 'first_non_null' requires at least 1 input
```

### conditional_select

```python
# ✅ Valid - Alternating conditions and values
regime = conditional_select()(
    rsi < 30, "oversold",
    rsi > 70, "overbought",
    "neutral"  # optional default
)

# ❌ Error - Only 1 input
result = conditional_select()(condition)
# Error: requires at least 2 inputs (condition, value)

# ❌ Error - Non-boolean condition
result = conditional_select()(src.c, 10, src.o, 20)
# Error: input at position 0 must be Boolean, got Decimal
```

## ➕ Adding a New Validator

### 1. Create Header (`my_validator.h`)
```cpp
#pragma once
#include "special_node_validator.h"

namespace epoch_stratifyx::epochflow {
    class MyValidator : public ISpecialNodeValidator {
    public:
        void ValidateInputs(const ValidationContext& ctx) const override;
        std::string GetName() const override { return "MyValidator"; }
    };
}
```

### 2. Implement (`my_validator.cpp`)
```cpp
#include "my_validator.h"
#include "../type_checker.h"

namespace epoch_stratifyx::epochflow {
    void MyValidator::ValidateInputs(const ValidationContext& ctx) const {
        // Your validation logic here
        if (ctx.args.size() != 3) {
            throw std::runtime_error(
                "'my_transform' requires exactly 3 inputs for node '" +
                ctx.target_node_id + "'");
        }
    }

    // Auto-register
    REGISTER_SPECIAL_VALIDATOR("my_transform", MyValidator)
}
```

### 3. Update CMakeLists.txt
```cmake
target_sources(epoch_metadata PRIVATE
    # ... existing sources ...
    compiler/validators/my_validator.cpp
)
```

### 4. Build and Test
```bash
cmake --build cmake-build-debug --target libepoch_metadata.so -j 4
cmake --build cmake-build-debug --target epoch_metadata_test -j 4
```

## 📚 Documentation

- **Full Documentation**: `src/epochflow/compiler/validators/README.md`
- **System Overview**: `VALIDATOR_SYSTEM.md`
- **This Quick Start**: `VALIDATOR_QUICK_START.md`

## ✨ Key Points

- ✅ Compile-time validation
- ✅ Zero runtime cost
- ✅ Modular design
- ✅ Easy to extend
- ✅ Clear error messages

## 🎯 Test Cases Location

```
test/epochflow/compiler/test_cases/
├── first_non_null_no_inputs/          # Error test
├── first_non_null_valid/              # Success test
├── conditional_select_insufficient_inputs/  # Error test
├── conditional_select_invalid_condition_type/  # Error test
└── conditional_select_valid/          # Success test
```

## 🐛 Debugging Tips

1. **Check registration**: Validator must use `REGISTER_SPECIAL_VALIDATOR` macro
2. **Build order**: Rebuild library first, then tests
3. **Test discovery**: Tests auto-discovered from `test_cases/` directory
4. **Error format**: Expected errors in `expected.json` with `{"error": "..."}`

## 🔗 Related Files

- **Interface**: `src/epochflow/compiler/validators/special_node_validator.h`
- **Registry**: `src/epochflow/compiler/validators/special_node_validator.cpp`
- **Integration**: `src/epochflow/compiler/node_builder.cpp` (lines 370-389)
- **Validators**: `src/epochflow/compiler/validators/*_validator.{h,cpp}`
