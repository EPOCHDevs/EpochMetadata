# Enum Support in EpochFlow

## Overview

EpochFlow has full support for enums across the entire codebase. Enums are automatically resolved based on parameter metadata, allowing you to use bare identifiers without quotes.

---

## How It Works

### 1. Enum Definition (`constants.h`)

Enums are defined using the `CREATE_ENUM` macro which automatically generates:
- Enum class with values
- `EnumWrapper` with `FromString()` / `ToString()` methods
- Glaze metadata for automatic serialization/deserialization

```cpp
CREATE_ENUM(CardColor,
            Default,  // Neutral/gray
            Primary,  // Brand color
            Info,     // Blue
            Success,  // Green
            Warning,  // Yellow/orange
            Error);   // Red
```

### 2. Parser Handling (AST)

When the parser encounters a bare identifier like `Success`:
- It's represented as a `Name` AST node with `id = "Success"`
- No type resolution happens at parse time

### 3. Compiler Conversion (`CallKwargsToGeneric`)

When converting to `glz::generic`:
```cpp
if (auto* name = dynamic_cast<const Name*>(value_expr.get())) {
    // Bare identifier - treat as string (e.g., icon=Signal)
    obj[key] = name->id;  // Stores "Signal" as string
}
```

### 4. Glaze Deserialization (Type-Safe)

Glaze automatically converts the string to the correct enum type based on metadata:
```cpp
CardColor color;
glz::read(color, "Success");  // Automatically converts "Success" → CardColor::Success
```

---

## Available Enums

### CardRenderType
How to render column values in cards.

```python
CardRenderType.Text        # Generic text/label
CardRenderType.Integer     # Integer numeric value
CardRenderType.Decimal     # Decimal/floating point
CardRenderType.Percent     # Percentage value
CardRenderType.Monetary    # Currency/money value
CardRenderType.Duration    # Duration in nanoseconds
CardRenderType.Badge       # Badge/pill element
CardRenderType.Timestamp   # Date/time display
CardRenderType.Boolean     # True/False indicator
```

### CardSlot
Position where card column should be rendered.

```python
CardSlot.PrimaryBadge      # Top-left badge
CardSlot.SecondaryBadge    # Top-right badge
CardSlot.Hero              # Center large element
CardSlot.Subtitle          # Below hero
CardSlot.Footer            # Bottom
CardSlot.Details           # "Show More" expandable section
```

### CardColor
Color scheme for cards (used in color_map).

```python
CardColor.Default          # Neutral/gray
CardColor.Primary          # Brand color
CardColor.Info             # Blue
CardColor.Success          # Green
CardColor.Warning          # Yellow/orange
CardColor.Error            # Red
```

### CardIcon
Icon displayed in card selector sidebar.

```python
CardIcon.Chart             # General analysis/charts
CardIcon.Gap               # Gap events
CardIcon.Signal            # Trading signals
CardIcon.Trade             # Trade executions
CardIcon.Position          # Open positions
CardIcon.Alert             # Alerts/notifications
CardIcon.TrendUp           # Bullish events
CardIcon.TrendDown         # Bearish events
CardIcon.Calendar          # Time-based events
CardIcon.Dollar            # P&L/financial
CardIcon.Candle            # Price action
CardIcon.Info              # General information
```

---

## Usage Examples

### Bare Identifiers (Recommended)

```python
# ✅ Clean - bare identifiers
CardColumnSchema(
    column_id="status",
    slot=Hero,
    render_type=Text,
    color_map={Success: ["ACTIVE"], Error: ["INACTIVE"]}
)

CardSchemaFilter(
    title="Signals",
    icon=Signal,
    select_key="is_signal",
    schemas=[...]
)
```

### String Literals (Also Supported)

```python
# ✅ Also works - string literals
CardColumnSchema(
    column_id="status",
    slot="Hero",
    render_type="Text",
    color_map={"Success": ["ACTIVE"], "Error": ["INACTIVE"]}
)

CardSchemaFilter(
    title="Signals",
    icon="Signal",
    select_key="is_signal",
    schemas=[...]
)
```

### Mixed (Valid)

```python
# ✅ Can mix bare identifiers and strings
CardColumnSchema(
    column_id="status",
    slot=Hero,                    # Bare identifier
    render_type="Text",           # String literal
    color_map={
        Success: ["ACTIVE"],      # Bare identifier key
        "Error": ["INACTIVE"]     # String literal key
    }
)
```

---

## Type Resolution

### Automatic Based on Metadata

The compiler knows what enum type to expect based on parameter metadata:

```typescript
// Metadata for CardColumnSchema
{
  "slot": {
    "type": "Enum<CardSlot>",  // Compiler knows Success → CardSlot::Success
    "required": true
  },
  "render_type": {
    "type": "Enum<CardRenderType>",  // Compiler knows Text → CardRenderType::Text
    "required": true
  }
}
```

### Context-Aware Resolution

Even though `Success` appears in multiple enums:
- `CardColor::Success` (Green)
- `SessionType::Success` (hypothetical)

The compiler resolves it correctly based on context:

```python
# Context: color_map expects CardColor
color_map={Success: ["ACTIVE"]}  # → CardColor::Success

# Context: slot expects CardSlot
slot=Hero  # → CardSlot::Hero

# Context: icon expects CardIcon
icon=Signal  # → CardIcon::Signal
```

---

## Error Handling

### Invalid Enum Value

```python
# ❌ Error: "Invalid enum value for CardSlot"
CardColumnSchema(
    column_id="test",
    slot=InvalidSlot,  # Not a valid CardSlot
    render_type=Text
)
```

Error message:
```
Invalid enum value for CardSlot: 'InvalidSlot'
Valid values: PrimaryBadge, SecondaryBadge, Hero, Subtitle, Footer, Details
```

### Ambiguous Enum (If Same Value in Multiple Enums)

Currently, **there is no ambiguity** because enum values are unique across different enum types in the codebase:
- `Success` only exists in `CardColor`
- `Hero` only exists in `CardSlot`
- `Signal` only exists in `CardIcon`

---

## For UI Agents (Monaco Editor)

### IntelliSense Configuration

Provide context-aware enum suggestions based on parameter name:

```javascript
// Detect parameter context
if (beforeCursor.includes('slot=')) {
    // Suggest CardSlot values
    suggestions = ['PrimaryBadge', 'SecondaryBadge', 'Hero', 'Subtitle', 'Footer', 'Details'];
}
else if (beforeCursor.includes('icon=')) {
    // Suggest CardIcon values
    suggestions = ['Chart', 'Gap', 'Signal', 'Trade', 'Position', 'Alert', ...];
}
else if (beforeCursor.includes('render_type=')) {
    // Suggest CardRenderType values
    suggestions = ['Text', 'Integer', 'Decimal', 'Percent', 'Monetary', ...];
}
else if (inColorMapKey) {
    // Suggest CardColor values
    suggestions = ['Default', 'Primary', 'Info', 'Success', 'Warning', 'Error'];
}
```

### Validation

```javascript
function validateEnum(value, parameterName) {
    const enumMap = {
        'slot': ['PrimaryBadge', 'SecondaryBadge', 'Hero', 'Subtitle', 'Footer', 'Details'],
        'icon': ['Chart', 'Gap', 'Signal', 'Trade', 'Position', 'Alert', 'TrendUp', 'TrendDown', 'Calendar', 'Dollar', 'Candle', 'Info'],
        'render_type': ['Text', 'Integer', 'Decimal', 'Percent', 'Monetary', 'Duration', 'Badge', 'Timestamp', 'Boolean'],
        // color_map keys are CardColor
    };

    const validValues = enumMap[parameterName];
    if (!validValues.includes(value)) {
        return `Invalid ${parameterName}: "${value}". Valid values: ${validValues.join(', ')}`;
    }
    return null;
}
```

---

## Implementation Details

### EnumWrapper Template

All enums use the `EnumClassT` template:

```cpp
template <class EnumClass, class NumType, const char *EnumClassAsStr, const char *elements>
struct EnumClassT {
    static EnumClass FromString(std::string const &enumClassAsString) {
        auto enumAsStringIt = m_data.first.find(enumClassAsString);
        if (enumAsStringIt == m_data.first.end()) {
            throw std::invalid_argument("Invalid enum value for " + std::string(EnumClassAsStr));
        }
        return enumAsStringIt->second;
    }

    static std::string ToString(EnumClass enumClass) {
        return m_data.second.at(enumClass);
    }
};
```

### Glaze Integration

```cpp
template <> struct glz::meta<epoch_core::CardColor> {
    using enum epoch_core::CardColor;
    static constexpr auto value = enumerate(
        Default, Primary, Info, Success, Warning, Error
    );
};
```

This enables automatic serialization/deserialization:
```cpp
CardColor color = CardColor::Success;
std::string json = glz::write_json(color);  // "Success"

CardColor parsed;
glz::read_json(parsed, "\"Success\"");  // CardColor::Success
```

---

## Best Practices

### 1. Use Bare Identifiers
```python
# ✅ Preferred - clean and type-safe
slot=Hero, icon=Signal, render_type=Decimal
```

### 2. Consistent Naming
All enum values use PascalCase:
- `Success` not `success`
- `PrimaryBadge` not `primary_badge`

### 3. Let Metadata Drive Resolution
Don't manually check enum types - let glaze deserializer handle it:
```cpp
// ❌ Don't do this
if (param_name == "slot") {
    slot = CardSlotWrapper::FromString(value);
} else if (param_name == "icon") {
    icon = CardIconWrapper::FromString(value);
}

// ✅ Do this - glaze handles it automatically
glz::read(card_schema, generic_obj);
```

---

## Summary

✅ **Enums are fully supported** across the entire EpochFlow codebase:

1. **Parser**: Bare identifiers preserved as strings
2. **Compiler**: Strings passed through to glaze
3. **Glaze**: Automatic type-safe conversion based on metadata
4. **Runtime**: Type-safe enum values in C++ structs

**Key Point**: You asked if we "know what type of enum based on the metadata" - **YES!** The glaze deserializer uses the C++ type information (from metadata/struct definitions) to automatically convert `"Success"` string → `CardColor::Success` enum value. No manual type checking needed!
