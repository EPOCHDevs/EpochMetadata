
---

## Callout Directives

Documentation uses directive syntax for callouts/admonitions. These are parsed and rendered with appropriate styling.

### Syntax

```markdown
:::type
Content here
:::
```

### Supported Types

**`:::tip`** - Best practices, helpful suggestions (green, checkmark icon)
```markdown
:::tip
EpochScript is declarative - you describe *what* you want, not *how* to compute it.
:::
```

**`:::warning`** - Important warnings, error cases (orange/yellow, warning icon)
```markdown
:::warning
Variables cannot be reassigned. Each variable can only be assigned once.
:::
```

**`:::note`** - Additional information, context (blue, info icon)
```markdown
:::note
Type errors are caught at compile time, before execution.
:::
```

**`:::info`** - Informational content (blue, info icon)
```markdown
:::info
Resource tags are available for Plus and Enterprise plans.
:::
```

**`:::danger`** - Critical warnings, breaking changes (red, danger icon)
```markdown
:::danger
Do NOT use this transform on daily timeframes. Intraday data required.
:::
```

### Rendering Specification

**Visual style** (matching LangSmith):
- **Border**: Left border (4px) in type color
- **Background**: Light tint of type color (5-10% opacity)
- **Icon**: Type-specific icon at top-left
- **Text**: Dark text, markdown formatting supported
- **Padding**: 16px all sides

**Color scheme**:
- `tip`: Green (#10B981 border, #D1FAE5 background)
- `warning`: Yellow/Orange (#F59E0B border, #FEF3C7 background)
- `note`: Blue (#3B82F6 border, #DBEAFE background)
- `info`: Blue (#3B82F6 border, #DBEAFE background)
- `danger`: Red (#EF4444 border, #FEE2E2 background)

**Icons**:
- `tip`: Checkmark or lightbulb
- `warning`: Exclamation triangle
- `note`: Info circle (i)
- `info`: Info circle (i)
- `danger`: Alert octagon

### Parsing Logic

1. Detect `:::type` opening
2. Capture all content until `:::`
3. Parse markdown within content
4. Render with appropriate styling and icon
5. Support nested markdown (code blocks, lists, bold, etc.)

### Examples

**Simple:**
```markdown
:::tip
Use positive lag only: `src.c[1]` for 1 bar ago.
:::
```

**With code block:**
```markdown
:::warning
Variables cannot be reassigned.

```epochscript
price = src.c
price = src.o  # ERROR: Variable 'price' already bound
```
:::
```

**With markdown formatting:**
```markdown
:::note
The lag operator `[N]` is the **ONLY** form of indexing in EpochScript.
:::
```

---

## Migration from Blockquotes

Old blockquote style:
```markdown
> **Warning:** Content here
```

New directive style:
```markdown
:::warning
Content here
:::
```

All documentation should use directive syntax for consistency and easier parsing.

---

## Grid Directives

Grid layouts use directive syntax for overview pages with navigable cards.

### Syntax

```markdown
:::grid
[
  {
    "title": "Card Title",
    "description": "Card description text",
    "link": "./path/to/page.md",
    "icon": "icon-name",
    "category": "Category Label"
  },
  {
    "title": "Another Card",
    "description": "Another description",
    "link": "./another-page.md",
    "icon": "icon-name",
    "category": "Category Label"
  }
]
:::
```

### Properties

Each grid item requires:
- **title** (required): Card heading (2-5 words)
- **description** (required): Card body text (1-2 sentences)
- **link** (required): Relative path to target page
- **icon** (optional): Icon identifier (code, package, settings, chart-line, etc.)
- **category** (optional): Section label for grouping cards

### Rendering Specification

**Grid layout**:
- **Columns**: 3 columns on desktop, 2 on tablet, 1 on mobile
- **Gap**: 24px between cards
- **Card styling**: White background, 1px border (#E5E7EB), rounded corners (8px)
- **Hover**: Lift effect (shadow increase, 2px translateY)
- **Padding**: 24px inside each card

**Card structure**:
- **Icon**: Top-left, 40×40px, colored by category
- **Category**: Small label above title, uppercase, 11px, gray (#6B7280)
- **Title**: 18px, bold, dark (#111827)
- **Description**: 14px, regular, gray (#4B5563)
- **Link**: Entire card clickable

**Icon set** (lucide-react or similar):
- `code` - Brackets icon
- `package` - Package/box icon
- `settings` - Gear icon
- `chart-line` - Line chart icon
- `book-open` - Open book icon
- `trending-up` - Upward trend icon
- `alert-circle` - Circle with exclamation

### Examples

**Simple grid:**
```markdown
:::grid
[
  {
    "title": "Getting Started",
    "description": "Quick introduction to EpochScript",
    "link": "./getting-started.md",
    "icon": "code"
  },
  {
    "title": "API Reference",
    "description": "Complete transform catalog",
    "link": "./reference/index.md",
    "icon": "book-open"
  }
]
:::
```

**Categorized grid:**
```markdown
:::grid
[
  {
    "title": "Language Fundamentals",
    "description": "Syntax, types, operators, and critical language limitations",
    "link": "./language/index.md",
    "icon": "code",
    "category": "Core Concepts"
  },
  {
    "title": "Core Concepts",
    "description": "Timeframes, sessions, and cross-sectional analysis",
    "link": "./concepts/index.md",
    "icon": "package",
    "category": "Core Concepts"
  },
  {
    "title": "Visualization",
    "description": "Auto-plotting, sidebar cards, event markers, and dashboards",
    "link": "./visualization/index.md",
    "icon": "chart-line",
    "category": "Features"
  }
]
:::
```

### Frontmatter

Pages with grid directives should have:
```yaml
---
page_type: overview
layout: grid
order: 1
category: Documentation
description: Page description for SEO
parent: ../index.md
---
```

**Note**: `grid_items` property in frontmatter is deprecated. Use inline `:::grid` directives instead.

---

## Code Block Syntax

All code examples should use `epochscript` syntax highlighting, not `python`.

### Syntax

````markdown
```epochscript
close = src.c
fast_ma = ema(period=20)(close)
signal = fast_ma > slow_ma
```
````

### Rendering Specification

- **Language**: Always `epochscript` for EpochScript code examples
- **Syntax highlighting**: Use EpochScript grammar rules
- **Theme**: Match documentation theme (light/dark mode support)
- **Line numbers**: Optional, based on UI preference
- **Copy button**: Include copy-to-clipboard button

### Invalid

❌ Do NOT use `python` for EpochScript code:
````markdown
```python
close = src.c  # WRONG - this is EpochScript, not Python
```
````

---

## Image Directives

Images use directive syntax for responsive rendering and proper attribution.

### Syntax

```markdown
:::image
source: /path/to/image.png
alt: Description of image
caption: Optional caption text
width: 800px
:::
```

### Properties

- **source** (required): Relative or absolute path to image
- **alt** (required): Accessibility text describing image content
- **caption** (optional): Text displayed below image
- **width** (optional): Max width (default: 100% of container)
- **height** (optional): Max height
- **align** (optional): `left`, `center`, `right` (default: center)

### Rendering Specification

**Visual style**:
- **Container**: Centered with max-width constraint
- **Border**: Optional 1px light border (#E5E7EB)
- **Shadow**: Subtle shadow (0 1px 3px rgba(0,0,0,0.1))
- **Caption**: Gray text (#6B7280), centered, 14px, below image
- **Responsive**: Scale down on mobile, maintain aspect ratio
- **Lazy loading**: Load images as they enter viewport

### Examples

**Simple image:**
```markdown
:::image
source: ./screenshots/dashboard.png
alt: Strategy dashboard showing equity curve and metrics
:::
```

**With caption and size:**
```markdown
:::image
source: ./images/architecture.svg
alt: System architecture diagram
caption: EpochScript compilation and execution pipeline
width: 600px
:::
```

**Screenshot with attribution:**
```markdown
:::image
source: ./screenshots/langsmith-callouts.png
alt: LangSmith documentation callout styles
caption: Callout rendering in LangSmith docs (reference)
align: center
:::
```

### Migration from Markdown

Old markdown syntax:
```markdown
![Alt text](./path/to/image.png)
```

New directive syntax:
```markdown
:::image
source: ./path/to/image.png
alt: Alt text
:::
```
