# Custom Type Constructors - IntelliSense Guide

This document provides complete type definitions and usage examples for custom type constructors in EpochFlow. Use this for Monaco Editor IntelliSense and AI coding agents.

---

## Table of Contents

1. [Time Constructor](#time-constructor)
2. [CardSchemaFilter Constructor](#cardschemafilter-constructor)
3. [CardSchemaSQL Constructor](#cardschemasql-constructor)
4. [SqlStatement Constructor](#sqlstatement-constructor)
5. [CardColumnSchema Constructor](#cardcolumnschema-constructor)
6. [Complete Examples](#complete-examples)

---

## Time Constructor

### Type Definition

```typescript
/**
 * Represents a specific time of day (hour, minute, second, microsecond)
 * Used for session times, time-based filters, and scheduling
 */
class Time {
    /**
     * @param hour - Hour of the day (0-23)
     * @param minute - Minute of the hour (0-59)
     * @param second - Second of the minute (0-59), default: 0
     * @param microsecond - Microsecond component (0-999999), default: 0
     * @param tz - Timezone string (e.g., "UTC", "America/New_York"), default: "UTC"
     */
    constructor(
        hour: number,
        minute: number,
        second?: number,
        microsecond?: number,
        tz?: string
    );
}
```

### Usage Examples

```python
# Basic time (9:30 AM UTC)
Time(hour=9, minute=30)

# Time with seconds
Time(hour=16, minute=0, second=0)

# Full precision with timezone
Time(hour=9, minute=30, second=0, microsecond=0, tz="America/New_York")

# Backward compatible string format (still supported)
"09:30:00"
```

### Monaco IntelliSense Schema

```json
{
  "Time": {
    "kind": "Class",
    "detail": "Time constructor for hour:minute:second precision",
    "documentation": "Creates a Time object representing a specific time of day. Used for session times and time-based operations.",
    "insertText": "Time(hour=${1:9}, minute=${2:30})",
    "parameters": [
      {
        "label": "hour",
        "documentation": "Hour of the day (0-23)",
        "type": "number",
        "required": true
      },
      {
        "label": "minute",
        "documentation": "Minute of the hour (0-59)",
        "type": "number",
        "required": true
      },
      {
        "label": "second",
        "documentation": "Second of the minute (0-59)",
        "type": "number",
        "required": false,
        "default": 0
      },
      {
        "label": "microsecond",
        "documentation": "Microsecond component (0-999999)",
        "type": "number",
        "required": false,
        "default": 0
      },
      {
        "label": "tz",
        "documentation": "Timezone string (e.g., 'UTC', 'America/New_York')",
        "type": "string",
        "required": false,
        "default": "UTC"
      }
    ]
  }
}
```

---

## CardColumnSchema Constructor

### Type Definition

```typescript
/**
 * Defines how a single DataFrame column should be rendered in a card
 */
class CardColumnSchema {
    /**
     * @param column_id - ID of the DataFrame column to display (e.g., "signal#result")
     * @param slot - Card slot position where this column will be rendered
     * @param render_type - How to render this column's value
     * @param color_map - Optional mapping of card colors to column values that trigger that color
     */
    constructor(
        column_id: string,
        slot: CardSlot,
        render_type: CardRenderType,
        color_map?: Record<CardColor, string[]>
    );
}

/**
 * Card slot positions
 */
enum CardSlot {
    PrimaryBadge = "PrimaryBadge",
    SecondaryBadge = "SecondaryBadge",
    Hero = "Hero",
    Subtitle = "Subtitle",
    Footer = "Footer",
    Details = "Details"
}

/**
 * Render types for column values
 */
enum CardRenderType {
    Text = "Text",
    Integer = "Integer",
    Decimal = "Decimal",
    Percent = "Percent",
    Monetary = "Monetary",
    Duration = "Duration",
    Badge = "Badge",
    Timestamp = "Timestamp",
    Boolean = "Boolean"
}

/**
 * Card color types
 */
enum CardColor {
    Success = "Success",
    Error = "Error",
    Warning = "Warning",
    Info = "Info",
    Primary = "Primary",
    Default = "Default"
}
```

### Usage Examples

```python
# Simple column schema
CardColumnSchema(
    column_id="signal#result",
    slot=Hero,
    render_type=Boolean
)

# With all options
CardColumnSchema(
    column_id="gap_size",
    slot=Hero,
    render_type=Decimal,
    color_map={
        Success: ["large"],
        Warning: ["medium"],
        Error: ["small"]
    }
)

# Multiple columns for a card
schemas = [
    CardColumnSchema(column_id="timestamp", slot=Footer, render_type=Timestamp),
    CardColumnSchema(column_id="value", slot=Hero, render_type=Decimal),
    CardColumnSchema(column_id="status", slot=PrimaryBadge, render_type=Badge)
]
```

### Monaco IntelliSense Schema

```json
{
  "CardColumnSchema": {
    "kind": "Class",
    "detail": "Card column rendering configuration",
    "documentation": "Defines how a DataFrame column should be displayed in a card slot with specific rendering and color rules.",
    "insertText": "CardColumnSchema(column_id=\"${1:column_id}\", slot=${2:Hero}, render_type=${3:Decimal})",
    "parameters": [
      {
        "label": "column_id",
        "documentation": "DataFrame column identifier (e.g., 'signal#result', 'gap_size')",
        "type": "string",
        "required": true
      },
      {
        "label": "slot",
        "documentation": "Card slot position",
        "type": "CardSlot",
        "enum": ["PrimaryBadge", "SecondaryBadge", "Hero", "Subtitle", "Footer", "Details"],
        "required": true
      },
      {
        "label": "render_type",
        "documentation": "How to render the column value",
        "type": "CardRenderType",
        "enum": ["Text", "Integer", "Decimal", "Percent", "Monetary", "Duration", "Badge", "Timestamp", "Boolean"],
        "required": true
      },
      {
        "label": "color_map",
        "documentation": "Maps card colors to column values that trigger that color",
        "type": "Record<CardColor, string[]>",
        "required": false
      }
    ]
  },
  "CardSlot": {
    "kind": "Enum",
    "values": ["PrimaryBadge", "SecondaryBadge", "Hero", "Subtitle", "Footer", "Details"]
  },
  "CardRenderType": {
    "kind": "Enum",
    "values": ["Text", "Integer", "Decimal", "Percent", "Monetary", "Duration", "Badge", "Timestamp", "Boolean"]
  },
  "CardColor": {
    "kind": "Enum",
    "values": ["Success", "Error", "Warning", "Info", "Primary", "Default"]
  }
}
```

---

## CardSchemaFilter Constructor

### Type Definition

```typescript
/**
 * Card selector configuration using boolean column filtering
 * Displays rows where a specific boolean column is true as clickable cards
 */
class CardSchemaFilter {
    /**
     * @param title - Title displayed above the card selector widget
     * @param icon - Icon displayed in collapsed sidebar view
     * @param select_key - Name of boolean DataFrame column used to filter rows
     * @param schemas - Array of column definitions specifying how each column renders
     */
    constructor(
        title: string,
        icon: CardIcon,
        select_key: string,
        schemas: CardColumnSchema[]
    );
}

/**
 * Available card icons
 */
enum CardIcon {
    Chart = "Chart",
    Gap = "Gap",
    Signal = "Signal",
    Trade = "Trade",
    Position = "Position",
    Alert = "Alert",
    TrendUp = "TrendUp",
    TrendDown = "TrendDown",
    Calendar = "Calendar",
    Dollar = "Dollar",
    Candle = "Candle",
    Info = "Info"
}
```

### Usage Examples

```python
# Basic gap report cards
CardSchemaFilter(
    title="Gap Alerts",
    icon=Gap,
    select_key="is_gap",
    schemas=[
        CardColumnSchema(column_id="gap_size", slot=Hero, render_type=Decimal),
        CardColumnSchema(column_id="timestamp", slot=Footer, render_type=Timestamp)
    ]
)

# Signal alerts with multiple fields
CardSchemaFilter(
    title="Trading Signals",
    icon=Signal,
    select_key="signal#result",
    schemas=[
        CardColumnSchema(column_id="signal_strength", slot=Hero, render_type=Percent),
        CardColumnSchema(column_id="signal_type", slot=PrimaryBadge, render_type=Badge),
        CardColumnSchema(column_id="timestamp", slot=Footer, render_type=Timestamp),
        CardColumnSchema(column_id="details", slot=Details, render_type=Text)
    ]
)

# Full example in context
src = market_data_source()
gaps = detect_gaps()(src.c)
card_selector_filter(
    card_schema=CardSchemaFilter(
        title="Price Gaps",
        icon=Gap,
        select_key="gaps#is_gap",
        schemas=[
            CardColumnSchema(column_id="gaps#gap_size", slot=Hero, render_type=Decimal),
            CardColumnSchema(column_id="gaps#timestamp", slot=Footer, render_type=Timestamp)
        ]
    )
)(gaps)
```

### Monaco IntelliSense Schema

```json
{
  "CardSchemaFilter": {
    "kind": "Class",
    "detail": "Card selector with boolean column filtering",
    "documentation": "Creates an interactive card selector widget where rows are filtered by a boolean column. Only rows where the select_key column is true are displayed as clickable cards.",
    "insertText": "CardSchemaFilter(\n\ttitle=\"${1:Card Title}\",\n\ticon=${2:Signal},\n\tselect_key=\"${3:column_name}\",\n\tschemas=[\n\t\t${4:CardColumnSchema(column_id=\"col1\", slot=Hero, render_type=Decimal)}\n\t]\n)",
    "parameters": [
      {
        "label": "title",
        "documentation": "Title displayed above the card selector widget",
        "type": "string",
        "required": true
      },
      {
        "label": "icon",
        "documentation": "Icon displayed in collapsed sidebar view to identify card type",
        "type": "CardIcon",
        "enum": ["Chart", "Gap", "Signal", "Trade", "Position", "Alert", "TrendUp", "TrendDown", "Calendar", "Dollar", "Candle", "Info"],
        "required": true
      },
      {
        "label": "select_key",
        "documentation": "Name of boolean DataFrame column used to filter rows (only rows where this column is true will be shown as cards)",
        "type": "string",
        "required": true
      },
      {
        "label": "schemas",
        "documentation": "Array of CardColumnSchema objects specifying how each DataFrame column should be rendered in the cards",
        "type": "CardColumnSchema[]",
        "required": true
      }
    ]
  },
  "CardIcon": {
    "kind": "Enum",
    "values": ["Chart", "Gap", "Signal", "Trade", "Position", "Alert", "TrendUp", "TrendDown", "Calendar", "Dollar", "Candle", "Info"]
  }
}
```

---

## CardSchemaSQL Constructor

### Type Definition

```typescript
/**
 * Card selector configuration using SQL query filtering
 * Displays rows returned by SQL query as clickable cards
 */
class CardSchemaSQL {
    /**
     * @param title - Title displayed above the card selector widget
     * @param icon - Icon displayed in collapsed sidebar view
     * @param sql - SQL query to filter/transform data (MUST use 'FROM self')
     * @param schemas - Array of column definitions specifying how each column renders
     */
    constructor(
        title: string,
        icon: CardIcon,
        sql: string | SqlStatement,
        schemas: CardColumnSchema[]
    );
}
```

### Usage Examples

```python
# Using SQL string directly
CardSchemaSQL(
    title="Filtered Signals",
    icon=Signal,
    sql="SELECT * FROM self WHERE signal_strength > 0.7",
    schemas=[
        CardColumnSchema(column_id="signal_strength", slot=Hero, render_type=Percent),
        CardColumnSchema(column_id="timestamp", slot=Footer, render_type=Timestamp)
    ]
)

# Using SqlStatement constructor
CardSchemaSQL(
    title="Top Gaps",
    icon=Gap,
    sql=SqlStatement(sql="SELECT * FROM self WHERE gap_size > 10 ORDER BY gap_size DESC LIMIT 10"),
    schemas=[
        CardColumnSchema(column_id="gap_size", slot=Hero, render_type=Decimal),
        CardColumnSchema(column_id="timestamp", slot=Footer, render_type=Timestamp)
    ]
)

# Complex SQL with aggregation
CardSchemaSQL(
    title="Daily Summary",
    icon=Chart,
    sql="""
        SELECT
            DATE(timestamp) as date,
            COUNT(*) as count,
            AVG(value) as avg_value,
            MAX(value) as max_value
        FROM self
        GROUP BY DATE(timestamp)
        ORDER BY date DESC
    """,
    schemas=[
        CardColumnSchema(column_id="date", slot=Hero, render_type=Timestamp),
        CardColumnSchema(column_id="count", slot=PrimaryBadge, render_type=Integer),
        CardColumnSchema(column_id="avg_value", slot=Subtitle, render_type=Decimal),
        CardColumnSchema(column_id="max_value", slot=Footer, render_type=Decimal)
    ]
)

# Full example in context
src = market_data_source()
signals = generate_signals()(src.c)
card_selector_sql(
    card_schema=CardSchemaSQL(
        title="Strong Signals",
        icon=Signal,
        sql="SELECT * FROM self WHERE SLOT0 > 0.8 ORDER BY SLOT0 DESC",
        schemas=[
            CardColumnSchema(column_id="SLOT0", slot=Hero, render_type=Percent)
        ]
    )
)(signals)
```

### Monaco IntelliSense Schema

```json
{
  "CardSchemaSQL": {
    "kind": "Class",
    "detail": "Card selector with SQL query filtering",
    "documentation": "Creates an interactive card selector widget where rows are filtered by a SQL query. The SQL query MUST use 'FROM self' to reference input data. Input columns are automatically renamed to SLOT0, SLOT1, SLOT2, etc.",
    "insertText": "CardSchemaSQL(\n\ttitle=\"${1:Card Title}\",\n\ticon=${2:Signal},\n\tsql=\"${3:SELECT * FROM self WHERE condition}\",\n\tschemas=[\n\t\t${4:CardColumnSchema(column_id=\"col1\", slot=Hero, render_type=Decimal)}\n\t]\n)",
    "parameters": [
      {
        "label": "title",
        "documentation": "Title displayed above the card selector widget",
        "type": "string",
        "required": true
      },
      {
        "label": "icon",
        "documentation": "Icon displayed in collapsed sidebar view to identify card type",
        "type": "CardIcon",
        "enum": ["Chart", "Gap", "Signal", "Trade", "Position", "Alert", "TrendUp", "TrendDown", "Calendar", "Dollar", "Candle", "Info"],
        "required": true
      },
      {
        "label": "sql",
        "documentation": "SQL query to filter/transform data (MUST use 'FROM self'). Input columns are automatically renamed to SLOT0, SLOT1, SLOT2, etc. based on connection order.",
        "type": "string | SqlStatement",
        "required": true,
        "validation": {
          "pattern": ".*FROM\\s+self.*",
          "message": "SQL query must use 'FROM self' to reference input data"
        }
      },
      {
        "label": "schemas",
        "documentation": "Array of CardColumnSchema objects specifying how each DataFrame column should be rendered in the cards",
        "type": "CardColumnSchema[]",
        "required": true
      }
    ]
  }
}
```

---

## SqlStatement Constructor

### Type Definition

```typescript
/**
 * Represents a validated SQL statement for data transformation
 * Used in transforms that accept SQL queries
 */
class SqlStatement {
    /**
     * @param sql - SQL query string (validation rules depend on context)
     */
    constructor(sql: string);
}
```

### Usage Examples

```python
# Basic SQL statement
SqlStatement(sql="SELECT * FROM self WHERE value > 100")

# Multi-output SQL (must alias outputs as output0, output1, etc.)
SqlStatement(sql="""
    SELECT
        high - low AS output0,
        (high + low) / 2 AS output1,
        timestamp
    FROM self
""")

# Using with sql_query transforms
query = sql_query_1(
    sql=SqlStatement(sql="SELECT close * volume AS dollar_volume, timestamp FROM self")
)

# Using in CardSchemaSQL
card_schema = CardSchemaSQL(
    title="Filtered Data",
    icon=Chart,
    sql=SqlStatement(sql="SELECT * FROM self WHERE condition = true ORDER BY timestamp DESC LIMIT 20"),
    schemas=[...]
)
```

### SQL Query Rules

#### For `sql_query_1` (1 Output)
- MUST SELECT exactly: `output0` and `timestamp`
- MUST use `FROM input`
- Input columns named: `input0`, `input1`, `input2`, etc.

```python
sql_query_1(sql=SqlStatement(sql="""
    SELECT
        input0 * input1 AS output0,
        timestamp
    FROM input
"""))
```

#### For `sql_query_2` (2 Outputs)
- MUST SELECT exactly: `output0`, `output1`, and `timestamp`

```python
sql_query_2(sql=SqlStatement(sql="""
    SELECT
        high - low AS output0,
        (high + low) / 2 AS output1,
        timestamp
    FROM input
"""))
```

#### For `sql_query_3` (3 Outputs)
- MUST SELECT exactly: `output0`, `output1`, `output2`, and `timestamp`

#### For `sql_query_4` (4 Outputs)
- MUST SELECT exactly: `output0`, `output1`, `output2`, `output3`, and `timestamp`

#### For `CardSchemaSQL`
- MUST use `FROM self` (not `FROM input`)
- Input columns named: `SLOT0`, `SLOT1`, `SLOT2`, etc.

```python
CardSchemaSQL(
    sql=SqlStatement(sql="SELECT * FROM self WHERE SLOT0 > threshold")
)
```

### Monaco IntelliSense Schema

```json
{
  "SqlStatement": {
    "kind": "Class",
    "detail": "SQL statement constructor",
    "documentation": "Creates a validated SQL statement for data transformation. Validation rules depend on the transform context (e.g., must use 'FROM input' for sql_query transforms, 'FROM self' for CardSchemaSQL).",
    "insertText": "SqlStatement(sql=\"${1:SELECT * FROM input}\")",
    "parameters": [
      {
        "label": "sql",
        "documentation": "SQL query string. Use 'FROM input' for sql_query transforms (columns: input0, input1, ...). Use 'FROM self' for CardSchemaSQL (columns: SLOT0, SLOT1, ...).",
        "type": "string",
        "required": true
      }
    ],
    "contextRules": {
      "sql_query_1": {
        "template": "SELECT <expr> AS output0, timestamp FROM input",
        "required": ["output0", "timestamp"],
        "from": "input"
      },
      "sql_query_2": {
        "template": "SELECT <expr1> AS output0, <expr2> AS output1, timestamp FROM input",
        "required": ["output0", "output1", "timestamp"],
        "from": "input"
      },
      "sql_query_3": {
        "template": "SELECT ... AS output0, ... AS output1, ... AS output2, timestamp FROM input",
        "required": ["output0", "output1", "output2", "timestamp"],
        "from": "input"
      },
      "sql_query_4": {
        "template": "SELECT ... AS output0, ... AS output1, ... AS output2, ... AS output3, timestamp FROM input",
        "required": ["output0", "output1", "output2", "output3", "timestamp"],
        "from": "input"
      },
      "CardSchemaSQL": {
        "template": "SELECT * FROM self WHERE ...",
        "from": "self"
      }
    }
  }
}
```

---

## Complete Examples

### Example 1: Gap Detection with Cards

```python
# Setup data source
src = market_data_source()

# Detect gaps
gaps = detect_gaps(threshold=10)(src.c)

# Create card selector for gaps
gap_cards = card_selector_filter(
    card_schema=CardSchemaFilter(
        title="Price Gaps",
        icon=Gap,
        select_key="gaps#is_gap",
        schemas=[
            CardColumnSchema(
                column_id="gaps#gap_size",
                slot=Hero,
                render_type=Decimal
            ),
            CardColumnSchema(
                column_id="gaps#gap_direction",
                slot=PrimaryBadge,
                render_type=Badge,
                color_map={
                    Success: ["up"],
                    Error: ["down"]
                }
            ),
            CardColumnSchema(
                column_id="gaps#timestamp",
                slot=Footer,
                render_type=Timestamp
            )
        ]
    )
)(gaps)
```

### Example 2: Signal Alerts with SQL Filtering

```python
# Generate signals
src = market_data_source()
rsi = rsi_indicator(period=14)(src.c)
signal = (rsi.result > 70) | (rsi.result < 30)

# Create card selector with SQL filtering for strong signals only
strong_signals = card_selector_sql(
    card_schema=CardSchemaSQL(
        title="Strong RSI Signals",
        icon=Signal,
        sql=SqlStatement(sql="""
            SELECT * FROM self
            WHERE (SLOT0 > 80 OR SLOT0 < 20)
            ORDER BY ABS(SLOT0 - 50) DESC
            LIMIT 10
        """),
        schemas=[
            CardColumnSchema(
                column_id="SLOT0",
                slot=Hero,
                render_type=Decimal
            ),
            CardColumnSchema(
                column_id="timestamp",
                slot=Footer,
                render_type=Timestamp
            )
        ]
    )
)(rsi.result, signal)
```

### Example 3: Trading Session Times

```python
# Define custom session times
custom_session = sessions(
    session=SessionRange(
        start=Time(hour=9, minute=30, second=0),
        end=Time(hour=16, minute=0, second=0)
    )
)

# Or use named sessions
london_session = sessions(session=London)
```

### Example 4: Advanced SQL with Multiple Outputs

```python
src = market_data_source()

# Calculate multiple metrics with SQL
metrics = sql_query_3(
    sql=SqlStatement(sql="""
        SELECT
            input0 - input1 AS output0,           -- high - low (range)
            (input0 + input1) / 2 AS output1,      -- (high + low) / 2 (midpoint)
            input0 - input2 AS output2,            -- high - close (upper wick)
            timestamp
        FROM input
    """)
)(src.h, src.l, src.c)

# Access outputs
range_val = metrics.output0
midpoint = metrics.output1
upper_wick = metrics.output2
```

---

## Monaco Editor Configuration

### Complete Type Definitions for Autocomplete

```javascript
const epochFlowCustomTypes = {
  // Enums
  CardSlot: ['PrimaryBadge', 'SecondaryBadge', 'Hero', 'Subtitle', 'Footer', 'Details'],
  CardRenderType: ['Text', 'Integer', 'Decimal', 'Percent', 'Monetary', 'Duration', 'Badge', 'Timestamp', 'Boolean'],
  CardIcon: ['Chart', 'Gap', 'Signal', 'Trade', 'Position', 'Alert', 'TrendUp', 'TrendDown', 'Calendar', 'Dollar', 'Candle', 'Info'],
  CardColor: ['Success', 'Error', 'Warning', 'Info', 'Primary', 'Default'],

  // Constructors
  constructors: [
    {
      name: 'Time',
      signature: 'Time(hour: int, minute: int, second?: int, microsecond?: int, tz?: string)',
      parameters: ['hour', 'minute', 'second', 'microsecond', 'tz'],
      snippet: 'Time(hour=${1:9}, minute=${2:30})$0'
    },
    {
      name: 'CardColumnSchema',
      signature: 'CardColumnSchema(column_id: string, slot: CardSlot, render_type: CardRenderType, color_map?: dict)',
      parameters: ['column_id', 'slot', 'render_type', 'color_map'],
      snippet: 'CardColumnSchema(column_id="${1:column_id}", slot=${2:Hero}, render_type=${3:Decimal})$0'
    },
    {
      name: 'CardSchemaFilter',
      signature: 'CardSchemaFilter(title: string, icon: CardIcon, select_key: string, schemas: CardColumnSchema[])',
      parameters: ['title', 'icon', 'select_key', 'schemas'],
      snippet: 'CardSchemaFilter(\n\ttitle="${1:title}",\n\ticon=${2:Signal},\n\tselect_key="${3:select_key}",\n\tschemas=[\n\t\t${4:CardColumnSchema(...)}\n\t]\n)$0'
    },
    {
      name: 'CardSchemaSQL',
      signature: 'CardSchemaSQL(title: string, icon: CardIcon, sql: string | SqlStatement, schemas: CardColumnSchema[])',
      parameters: ['title', 'icon', 'sql', 'schemas'],
      snippet: 'CardSchemaSQL(\n\ttitle="${1:title}",\n\ticon=${2:Signal},\n\tsql="${3:SELECT * FROM self}",\n\tschemas=[\n\t\t${4:CardColumnSchema(...)}\n\t]\n)$0'
    },
    {
      name: 'SqlStatement',
      signature: 'SqlStatement(sql: string)',
      parameters: ['sql'],
      snippet: 'SqlStatement(sql="${1:SELECT * FROM input}")$0'
    }
  ]
};

// Register with Monaco
monaco.languages.registerCompletionItemProvider('python', {
  provideCompletionItems: (model, position) => {
    const suggestions = [];

    // Add constructor suggestions
    epochFlowCustomTypes.constructors.forEach(ctor => {
      suggestions.push({
        label: ctor.name,
        kind: monaco.languages.CompletionItemKind.Constructor,
        insertText: ctor.snippet,
        insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
        documentation: ctor.signature,
        detail: `EpochFlow ${ctor.name} Constructor`
      });
    });

    // Add enum value suggestions when inside enum context
    // (detect based on parameter name, e.g., slot=, icon=, render_type=)
    const line = model.getLineContent(position.lineNumber);
    const beforeCursor = line.substring(0, position.column - 1);

    if (beforeCursor.includes('slot=')) {
      epochFlowCustomTypes.CardSlot.forEach(value => {
        suggestions.push({
          label: value,
          kind: monaco.languages.CompletionItemKind.EnumMember,
          insertText: value
        });
      });
    }

    if (beforeCursor.includes('icon=')) {
      epochFlowCustomTypes.CardIcon.forEach(value => {
        suggestions.push({
          label: value,
          kind: monaco.languages.CompletionItemKind.EnumMember,
          insertText: value
        });
      });
    }

    if (beforeCursor.includes('render_type=')) {
      epochFlowCustomTypes.CardRenderType.forEach(value => {
        suggestions.push({
          label: value,
          kind: monaco.languages.CompletionItemKind.EnumMember,
          insertText: value
        });
      });
    }

    return { suggestions };
  }
});
```

---

## Validation Rules for AI Agents

When generating code, ensure:

1. **Time objects**: All time values are 0-based (0-23 hours, 0-59 minutes/seconds)
2. **CardColumnSchema**:
   - `column_id` matches actual DataFrame column names
   - `slot` is one of the valid CardSlot enum values
   - `render_type` matches the column data type
3. **CardSchemaFilter**:
   - `select_key` references a boolean column in the DataFrame
   - At least one schema in `schemas` array
4. **CardSchemaSQL**:
   - SQL uses `FROM self` (not `FROM input`)
   - Input columns referenced as `SLOT0`, `SLOT1`, etc.
   - SQL is syntactically valid DuckDB SQL
5. **SqlStatement**:
   - For `sql_query_N`: use `FROM input`, outputs named `output0`...`outputN-1`
   - For `CardSchemaSQL`: use `FROM self`, columns named `SLOT0`, `SLOT1`, etc.

---

## Quick Reference Table

| Constructor | Primary Use Case | Key Parameters | Notes |
|------------|------------------|----------------|-------|
| `Time` | Session times, time filters | hour, minute | 24-hour format, UTC default |
| `CardColumnSchema` | Define card column rendering | column_id, slot, render_type | Part of card schema |
| `CardSchemaFilter` | Card selector with boolean filter | title, icon, select_key, schemas | Filters by boolean column |
| `CardSchemaSQL` | Card selector with SQL filter | title, icon, sql, schemas | Uses `FROM self` |
| `SqlStatement` | SQL transformations | sql | Context-dependent rules |

---

## Error Messages and Troubleshooting

Common errors and solutions:

```python
# ❌ Error: "Failed to parse Time constructor"
Time(hour=25, minute=30)  # Hour out of range

# ✅ Correct
Time(hour=9, minute=30)


# ❌ Error: "Unknown CardColumnSchema parameter"
CardColumnSchema(column_id="test", slot=Hero, type=Decimal)  # Wrong param name

# ✅ Correct
CardColumnSchema(column_id="test", slot=Hero, render_type=Decimal)


# ❌ Error: "SQL must use 'FROM self'"
CardSchemaSQL(..., sql="SELECT * FROM input", ...)  # Wrong table name

# ✅ Correct
CardSchemaSQL(..., sql="SELECT * FROM self", ...)


# ❌ Error: "Must alias outputs as output0"
sql_query_1(sql="SELECT value FROM input")  # Missing output alias

# ✅ Correct
sql_query_1(sql="SELECT value AS output0, timestamp FROM input")
```

---

This documentation should provide complete context for Monaco Editor IntelliSense and AI coding agents to generate correct EpochFlow code with custom type constructors.
