# FParserXml — Developer Guide

## Table of Contents

1. [Overview](#overview)
2. [Architecture at a Glance](#architecture-at-a-glance)
3. [Full Parse Flow](#full-parse-flow)
4. [Widget Store — id vs no-id](#widget-store--id-vs-no-id)
5. [Supported XML Elements](#supported-xml-elements)
6. [Usage Examples](#usage-examples)
7. [How to Add a New Widget](#how-to-add-a-new-widget)
8. [How to Add a New Container](#how-to-add-a-new-container)
9. [How to Add a New Structural Element](#how-to-add-a-new-structural-element)
10. [Quick-Reference Checklist](#quick-reference-checklist)

---

## Overview

`FParserXml` reads an XML file (or raw XML string) and builds a live Qt widget
tree from your custom F-widgets (`FGroupBox`, `FLabel`, `FComboBox`, …).

Key properties:

- The **root element can be anything** — `<dialog>`, `<combobox>`, `<groupbox>`, etc.  
  There is no forced `<dialog>` wrapper.
- Every created widget is stored internally.  
  Widgets with an `id` attribute can be retrieved in **O(1)** by id.  
  Widgets without an `id` are still reachable by iterating `allWidgets()`.
- One `FParserXml` instance can be reused; the store is cleared on each `fromFile()` / `fromString()` call.

---

## Architecture at a Glance

```
FParserXml
│
├── Public API
│   ├── fromFile(path)       →  QWidget*
│   ├── fromString(xml)      →  QWidget*
│   ├── findById(id)         →  QWidget*        O(1), id-bearing only
│   ├── findById<T>(id)      →  T*              typed convenience
│   ├── allWidgets()         →  QList<QWidget*> all, in document order
│   └── lastError()          →  QString
│
├── Orchestration (private)
│   ├── parse()              – clears store, finds first start-element
│   └── dispatchElement()    – THE central router: tag → correct parser
│
├── Container parsers (own a QBoxLayout, recurse)
│   ├── parseDialog()
│   ├── parseGroupBox()
│   └── parsePlainWidget()
│
├── Leaf widget parsers (create widget, register, return)
│   ├── parseLabel()
│   ├── parseLineEdit()
│   ├── parseComboBox()
│   ├── parseCheckBox()
│   └── parsePushButton()
│
├── Structural helpers (layout / spacing, NO widget returned)
│   ├── parseLayout()
│   └── parseSpacer()
│
└── Internal helpers
    ├── parseChildren()      – token loop, calls dispatchElement per child
    ├── registerWidget()     – adds to m_allWidgets; optionally to m_idMap
    ├── clearStore()
    ├── makeLayout()
    ├── parseOptions()
    └── skipToEndElement()
```

---

## Full Parse Flow

The diagram below traces a complete `fromFile()` call on the sample XML.

```
fromFile("config.xml")
│
└─► parse(QXmlStreamReader)
    │  clearStore()
    │  advance reader to first <startElement>  →  <dialog …>
    │
    └─► dispatchElement(xml, parentLayout=nullptr)
        │  tag == "dialog"
        │
        └─► parseDialog(xml)
            │  new QDialog, setWindowTitle, setMinimumSize
            │  QBoxLayout *rootLayout = makeLayout("vertical")
            │  dialog->setLayout(rootLayout)
            │  registerWidget(dialog, "mainDialog")   ← stored in idMap + allWidgets
            │
            └─► parseChildren(xml, rootLayout, "dialog")
                │  loop: readNext() tokens
                │
                ├─► <groupbox id="mainGroup" …>
                │   └─► dispatchElement → parseGroupBox()
                │       │  new FGroupBox, inner QBoxLayout
                │       │  registerWidget(groupBox, "mainGroup")
                │       └─► parseChildren(xml, inner, "groupbox")
                │           │
                │           ├─► <groupbox id="idGroup" …>   (recurses the same way)
                │           │   └─► parseGroupBox → parseChildren
                │           │       ├─► <layout type="horizontal">
                │           │       │   └─► dispatchElement → parseLayout()
                │           │       │       │  new QHBoxLayout, parent->addLayout()
                │           │       │       └─► parseChildren(xml, hLayout, "layout")
                │           │       │           ├─► <label id="lblUserName" …>
                │           │       │           │   └─► parseLabel()
                │           │       │           │       new FLabel, registerWidget()
                │           │       │           │       skipToEndElement("label")
                │           │       │           └─► <lineedit id="editUserName" …>
                │           │       │               └─► parseLineEdit()
                │           │       │                   new FLineEdit, registerWidget()
                │           │       │                   skipToEndElement("lineedit")
                │           │       └─► (second <layout> for Password row — same pattern)
                │           │
                │           ├─► <groupbox id="accessGroup" …>   (recurses)
                │           ├─► <groupbox id="limitGroup"  …>   (recurses)
                │           │
                │           └─► <checkbox text="Enable Debug Mode">   ← NO id
                │               └─► parseCheckBox()
                │                   registerWidget(w, "")   ← allWidgets only, not idMap
                │
                ├─► <spacer type="stretch" factor="1">
                │   └─► dispatchElement → parseSpacer()
                │       rootLayout->addStretch(1)
                │       skipToEndElement("spacer")
                │
                └─► <layout type="horizontal">
                    └─► dispatchElement → parseLayout()
                        │  new QHBoxLayout, rootLayout->addLayout()
                        └─► parseChildren(xml, hLayout, "layout")
                            ├─► <spacer type="stretch" factor="1">
                            ├─► <pushbutton id="btnOk"     …>
                            └─► <pushbutton id="btnCancel" …>

    dispatchElement returns QDialog*
parse() returns QDialog*
fromFile() returns QDialog*
```

### Key rule every parser follows

```
Container parser              Leaf parser
─────────────────────         ─────────────────────────────────
1. Read attrs                 1. Read attrs
2. Create widget              2. Create widget
3. Create inner layout        3. Apply all attrs to widget
4. widget->setLayout(layout)  4. registerWidget(w, id)
5. registerWidget(w, id)      5. skipToEndElement(tag)
6. parseChildren(…, tag)      6. return w
7. return widget
```

The container parser relies on `parseChildren` to consume its closing tag.  
The leaf parser calls `skipToEndElement` explicitly to do the same.  
This guarantee keeps the reader always positioned *past* the current element
when control returns to the caller.

---

## Widget Store — id vs no-id

```
XML attribute      m_idMap            m_allWidgets
─────────────      ──────────────     ─────────────────────────────────
id="btnOk"         "btnOk" → ptr  ✔   ptr appended  ✔
(no id)            —              ✘   ptr appended  ✔
```

Retrieval:

```cpp
// Fast, typed — only for widgets that had id= in XML
auto *btn = parser.findById<FPushButton>("btnOk");

// Fallback — loop through everything (includes id-less widgets)
for (QWidget *w : parser.allWidgets()) {
    if (auto *cb = qobject_cast<FCheckBox *>(w))
        cb->setChecked(false);
}
```

Duplicate ids emit a `qCWarning` and the **first** registration wins.

---

## Supported XML Elements

### Container elements (have children, own a layout)

| Tag | Qt type | Attributes |
|---|---|---|
| `<dialog>` | `QDialog` | `id`, `title`, `width`, `height`, `layout` |
| `<groupbox>` | `FGroupBox` | `id`, `title`, `layout` |
| `<widget>` | `QWidget` | `id`, `layout` |

`layout` accepts `"vertical"` (default) or `"horizontal"`.

### Leaf widget elements (single widget, no children)

| Tag | F-widget | Attributes |
|---|---|---|
| `<label>` | `FLabel` | `id`, `text` |
| `<lineedit>` | `FLineEdit` | `id`, `name`, `placeholder`, `text` |
| `<combobox>` | `FComboBox` | `id`, `name`, `options="{A, B, C}"` |
| `<checkbox>` | `FCheckBox` | `id`, `text`, `checked` (`"true"`/`"false"`) |
| `<pushbutton>` | `FPushButton` | `id`, `text`, `name` |

### Structural elements (affect layout only, return no widget)

| Tag | Effect | Attributes |
|---|---|---|
| `<layout>` | Inserts a nested `QHBoxLayout` / `QVBoxLayout` | `type` (`"horizontal"`/`"vertical"`) |
| `<spacer>` | `addStretch(n)` or `addSpacing(n)` | `type` (`"stretch"`/`"fixed"`), `factor`, `width`, `height` |

---

## Usage Examples

```cpp
FParserXml parser;

// ── Full dialog from file ─────────────────────────────────────────────────
QWidget *root = parser.fromFile(":/ui/config.xml");
if (!root) {
    qWarning() << parser.lastError();
    return;
}
qobject_cast<QDialog *>(root)->exec();

// ── Standalone widget (root is not a dialog) ──────────────────────────────
QWidget *combo = parser.fromFile(":/ui/role_combo.xml");
someLayout->addWidget(combo);

// ── Retrieve by id (fast, typed) ──────────────────────────────────────────
auto *okBtn    = parser.findById<FPushButton>("btnOk");
auto *roleCombo = parser.findById<FComboBox>("comboRole");
auto *userEdit  = parser.findById<FLineEdit>("editUserName");

if (okBtn)
    connect(okBtn, &FPushButton::clicked, dialog, &QDialog::accept);

// ── Iterate all widgets (including those without id) ──────────────────────
for (QWidget *w : parser.allWidgets())
    w->setFont(appFont);
```

---

## How to Add a New Widget

Follow these steps every time you create a new F-widget (e.g. `FSlider`).  
Touch exactly **4 places**, all inside `FParserXml.h` / `FParserXml.cpp`.

### Step 1 — Forward-declare the class in `FParserXml.h`

```cpp
// At the top of FParserXml.h, with the other forward declarations
class FSlider;          // ← add this
```

### Step 2 — Declare the private parser method in `FParserXml.h`

```cpp
// Inside the "Leaf widget parsers" section
QWidget *parseSlider(QXmlStreamReader &xml);    // ← add this
```

### Step 3 — Register the tag in `dispatchElement()` in `FParserXml.cpp`

```cpp
// Inside dispatchElement(), in the widget-producing block:
else if (tag == "slider")     widget = parseSlider(xml);   // ← add this line
```

### Step 4 — Implement the parser in `FParserXml.cpp`

```cpp
QWidget *FParserXml::parseSlider(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attrs = xml.attributes();

    auto *w = new FSlider(Qt::Horizontal);  // or whatever constructor

    // Read any custom attributes
    const int minVal = attrs.value("min").toInt();
    const int maxVal = attrs.value("max").toInt();
    const int value  = attrs.value("value").toInt();

    if (maxVal > minVal) {
        w->setRange(minVal, maxVal);
        w->setValue(value);
    }

    registerWidget(w, attrs.value("id").toString());  // always call this
    skipToEndElement(xml, "slider");                  // always call this last
    return w;
}
```

### Corresponding XML

```xml
<slider id="volumeSlider" min="0" max="100" value="50"/>
```

That's it — 4 touches and the new widget is fully integrated.

---

## How to Add a New Container

A container is a widget that **owns an inner layout and has child elements**
(like `<groupbox>` or `<widget>`). Use this pattern when your new F-widget
needs to hold other widgets inside it.

### Step 1 — Forward-declare in `FParserXml.h`

```cpp
class FCard;   // ← add this
```

### Step 2 — Declare parser in `FParserXml.h`

```cpp
// Inside "Container parsers" section
QWidget *parseCard(QXmlStreamReader &xml);   // ← add this
```

### Step 3 — Register tag in `dispatchElement()` in `FParserXml.cpp`

```cpp
else if (tag == "card")   widget = parseCard(xml);   // ← add this
```

### Step 4 — Implement in `FParserXml.cpp`

The critical difference from a leaf parser: call `parseChildren` instead of
`skipToEndElement`, and pass the inner layout and the tag name.

```cpp
QWidget *FParserXml::parseCard(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attrs = xml.attributes();

    auto *card         = new FCard(attrs.value("title").toString());
    const QString dir  = attrs.value("layout").toString();
    QBoxLayout *layout = makeLayout(dir.isEmpty() ? "vertical" : dir);
    card->setLayout(layout);

    registerWidget(card, attrs.value("id").toString());

    // Recurse into children — pass the inner layout and THIS tag name
    parseChildren(xml, layout, "card");   // ← NOT skipToEndElement

    return card;
}
```

### Corresponding XML

```xml
<card id="profileCard" title="User Profile" layout="vertical">
    <label id="lblName" text="Full Name:"/>
    <lineedit id="editName"/>
</card>
```

---

## How to Add a New Structural Element

A structural element affects layout but **creates no widget** (like `<spacer>`
or `<layout>`). It returns `nullptr` from `dispatchElement`.

### Step 1 — Declare in `FParserXml.h`

```cpp
// Inside "Structural helpers" section
void parseGrid(QXmlStreamReader &xml, QBoxLayout *parent);   // ← add this
```

### Step 2 — Handle the `nullptr` return in `dispatchElement()` in `FParserXml.cpp`

```cpp
// At the top of dispatchElement(), in the structural block:
if (tag == "grid") {
    parseGrid(xml, parentLayout);
    return nullptr;   // ← structural: no widget produced
}
```

### Step 3 — Implement in `FParserXml.cpp`

```cpp
void FParserXml::parseGrid(QXmlStreamReader &xml, QBoxLayout *parent)
{
    // Build whatever layout/widget structure you need
    // then either recurse with parseChildren or call skipToEndElement
    QXmlStreamAttributes attrs = xml.attributes();

    auto *gridLayout = new QGridLayout();
    if (parent) {
        auto *container = new QWidget();
        container->setLayout(gridLayout);
        parent->addWidget(container);
    }

    parseChildren(xml, /* adapt as needed */, "grid");
}
```

---

## Quick-Reference Checklist

| What you're adding | Files to touch | Methods to update |
|---|---|---|
| **New leaf widget** | `.h` + `.cpp` | forward-declare, declare method, `dispatchElement`, implement `parseXxx` |
| **New container widget** | `.h` + `.cpp` | forward-declare, declare method, `dispatchElement`, implement `parseXxx` with `parseChildren` |
| **New structural element** | `.h` + `.cpp` | declare method, early-return `nullptr` in `dispatchElement`, implement with `skipToEndElement` or `parseChildren` |
| **New attribute on existing widget** | `.cpp` only | inside the existing `parseXxx` function |
| **Rename a tag in XML** | `.cpp` only | the string in `dispatchElement` (`tag == "old"` → `tag == "new"`) |

The **only** file that routes tags to parsers is `dispatchElement()` in
`FParserXml.cpp`. If a tag is not listed there, the parser logs a warning and
skips it safely — nothing crashes.
