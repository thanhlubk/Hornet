#include <FancyWidgets/FParserXml.h>

// ── Replace with your actual custom-widget headers ───────────────────────────
#include <FancyWidgets/FGroupBox.h>
#include <FancyWidgets/FLabel.h>
#include <FancyWidgets/FLineEdit.h>
#include <FancyWidgets/FComboBox.h>
#include <FancyWidgets/FCheckBox.h>
#include <FancyWidgets/FPushButton.h>
// ────────────────────────────────────────────────────────────────────────────

#include <QDialog>
#include <QFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcFParserXml, "FParserXml")

// ============================================================================
//  Public API
// ============================================================================

QWidget *FParserXml::fromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_lastError = QString("Cannot open file: %1").arg(filePath);
        return nullptr;
    }
    QXmlStreamReader xml(&file);
    return parse(xml);
}

QWidget *FParserXml::fromString(const QString &xmlContent)
{
    QXmlStreamReader xml(xmlContent);
    return parse(xml);
}

QWidget *FParserXml::findById(const QString &id) const
{
    return m_idMap.value(id, nullptr);
}

QList<QWidget *> FParserXml::allWidgets() const
{
    return m_allWidgets;
}

// ============================================================================
//  Private – orchestration
// ============================================================================

QWidget *FParserXml::parse(QXmlStreamReader &xml)
{
    clearStore();
    m_lastError.clear();

    // Advance to the first start element – that is the root widget.
    while (!xml.atEnd() && !xml.hasError())
    {
        xml.readNext();
        if (xml.isStartElement())
        {
            QWidget *root = dispatchElement(xml, /*parentLayout=*/nullptr);
            if (xml.hasError())
                m_lastError = xml.errorString();
            return root; // may be nullptr for structural-only root (edge case)
        }
    }

    if (xml.hasError())
        m_lastError = xml.errorString();
    else
        m_lastError = "Empty or unreadable XML document.";

    return nullptr;
}

/**
 * Core dispatcher: reads the *current* start-element, builds the right widget,
 * and (if parentLayout != nullptr) adds it to that layout.
 *
 * Returns nullptr for purely structural elements (<layout>, <spacer>),
 * since those don't produce a QWidget.
 */
QWidget *FParserXml::dispatchElement(QXmlStreamReader &xml,
                                    QBoxLayout *parentLayout)
{
    Q_ASSERT(xml.isStartElement());

    const QString tag = xml.name().toString().toLower();
    QWidget *widget = nullptr;

    // ── Structural elements (manage layout/spacing, return no widget) ────────
    if (tag == "layout")
    {
        parseLayout(xml, parentLayout);
        return nullptr;
    }
    if (tag == "spacer")
    {
        parseSpacer(xml, parentLayout);
        return nullptr;
    }

    // ── Widget-producing elements ────────────────────────────────────────────
    if (tag == "dialog")
        widget = parseDialog(xml);
    else if (tag == "groupbox")
        widget = parseGroupBox(xml);
    else if (tag == "widget")
        widget = parsePlainWidget(xml);
    else if (tag == "label")
        widget = parseLabel(xml);
    else if (tag == "lineedit")
        widget = parseLineEdit(xml);
    else if (tag == "combobox")
        widget = parseComboBox(xml);
    else if (tag == "checkbox")
        widget = parseCheckBox(xml);
    else if (tag == "pushbutton")
        widget = parsePushButton(xml);
    else
    {
        qCWarning(lcFParserXml) << "Unknown element <" << tag << "> – skipping.";
        skipToEndElement(xml, tag);
    }

    // If we are inside a parent layout, add the new widget to it.
    if (widget && parentLayout)
        parentLayout->addWidget(widget);

    return widget;
}

// ============================================================================
//  Container parsers
// ============================================================================

QWidget *FParserXml::parseDialog(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attrs = xml.attributes();

    auto *dialog = new QDialog();
    dialog->setWindowTitle(attrs.value("title").toString());

    const int w = attrs.value("width").toInt();
    const int h = attrs.value("height").toInt();
    if (w > 0 && h > 0)
        dialog->setMinimumSize(w, h);

    const QString dir = attrs.value("layout").toString();
    QBoxLayout *layout = makeLayout(dir.isEmpty() ? "vertical" : dir);
    dialog->setLayout(layout);

    registerWidget(dialog, attrs.value("id").toString());
    parseChildren(xml, layout, "dialog");

    return dialog;
}

QWidget *FParserXml::parseGroupBox(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attrs = xml.attributes();

    auto *groupBox = new FGroupBox(attrs.value("title").toString());
    const QString dir = attrs.value("layout").toString();
    QBoxLayout *layout = makeLayout(dir.isEmpty() ? "vertical" : dir);
    groupBox->setLayout(layout);

    registerWidget(groupBox, attrs.value("id").toString());
    parseChildren(xml, layout, "groupbox");

    return groupBox;
}

QWidget *FParserXml::parsePlainWidget(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attrs = xml.attributes();

    auto *widget = new QWidget();
    const QString dir = attrs.value("layout").toString();
    QBoxLayout *layout = makeLayout(dir.isEmpty() ? "vertical" : dir);
    widget->setLayout(layout);

    registerWidget(widget, attrs.value("id").toString());
    parseChildren(xml, layout, "widget");

    return widget;
}

// ============================================================================
//  parseChildren – walks tokens until the parent's closing tag
// ============================================================================

void FParserXml::parseChildren(QXmlStreamReader &xml,
                              QBoxLayout *parentLayout,
                              const QString &parentTag)
{
    while (!xml.atEnd())
    {
        xml.readNext();

        if (xml.isEndElement() &&
            xml.name().toString().toLower() == parentTag.toLower())
        {
            return; // consumed the parent's end tag – hand control back
        }

        if (xml.isStartElement())
            dispatchElement(xml, parentLayout);
    }
}

// ============================================================================
//  Leaf widget parsers
// ============================================================================

QWidget *FParserXml::parseLabel(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attrs = xml.attributes();
    auto *w = new FLabel(attrs.value("text").toString());
    registerWidget(w, attrs.value("id").toString());
    skipToEndElement(xml, "label");
    return w;
}

QWidget *FParserXml::parseLineEdit(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attrs = xml.attributes();
    auto *w = new FLineEdit();

    const QString placeholder = attrs.value("placeholder").toString();
    if (!placeholder.isEmpty())
        w->setPlaceholderText(placeholder);

    const QString text = attrs.value("text").toString();
    if (!text.isEmpty())
        w->setText(text);

    const QString objName = attrs.value("name").toString();
    if (!objName.isEmpty())
        w->setObjectName(objName);

    registerWidget(w, attrs.value("id").toString());
    skipToEndElement(xml, "lineedit");
    return w;
}

QWidget *FParserXml::parseComboBox(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attrs = xml.attributes();
    auto *w = new FComboBox();

    const QString objName = attrs.value("name").toString();
    if (!objName.isEmpty())
        w->setObjectName(objName);

    const QStringList items = parseOptions(attrs.value("options").toString());
    if (!items.isEmpty())
        w->addItems(items);

    registerWidget(w, attrs.value("id").toString());
    skipToEndElement(xml, "combobox");
    return w;
}

QWidget *FParserXml::parseCheckBox(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attrs = xml.attributes();
    auto *w = new FCheckBox(attrs.value("text").toString());

    const QString checked = attrs.value("checked").toString().toLower();
    w->setChecked(checked == "true" || checked == "1");

    registerWidget(w, attrs.value("id").toString());
    skipToEndElement(xml, "checkbox");
    return w;
}

QWidget *FParserXml::parsePushButton(QXmlStreamReader &xml)
{
    QXmlStreamAttributes attrs = xml.attributes();
    auto *w = new FPushButton(attrs.value("text").toString());

    const QString objName = attrs.value("name").toString();
    if (!objName.isEmpty())
        w->setObjectName(objName);

    registerWidget(w, attrs.value("id").toString());
    skipToEndElement(xml, "pushbutton");
    return w;
}

// ============================================================================
//  Structural helpers (layout / spacer)
// ============================================================================

void FParserXml::parseLayout(QXmlStreamReader &xml, QBoxLayout *parent)
{
    QXmlStreamAttributes attrs = xml.attributes();
    const QString type = attrs.value("type").toString();
    QBoxLayout *layout = makeLayout(type.isEmpty() ? "horizontal" : type);

    if (parent)
        parent->addLayout(layout);

    parseChildren(xml, layout, "layout");
}

void FParserXml::parseSpacer(QXmlStreamReader &xml, QBoxLayout *parent)
{
    QXmlStreamAttributes attrs = xml.attributes();
    const QString type = attrs.value("type").toString().toLower();

    if (parent)
    {
        if (type == "stretch")
        {
            const int factor = attrs.value("factor").toInt();
            parent->addStretch(factor > 0 ? factor : 1);
        }
        else
        {
            // Fixed spacer
            int w = attrs.value("width").toInt();
            int h = attrs.value("height").toInt();
            if (w == 0 && h == 0)
            {
                const int factor = attrs.value("factor").toInt();
                w = h = factor;
            }
            parent->addSpacing(qMax(w, h));
        }
    }

    skipToEndElement(xml, "spacer");
}

// ============================================================================
//  Widget store
// ============================================================================

void FParserXml::registerWidget(QWidget *widget, const QString &id)
{
    if (!widget)
        return;

    m_allWidgets.append(widget);

    if (!id.isEmpty())
    {
        if (m_idMap.contains(id))
        {
            qCWarning(lcFParserXml)
                << "Duplicate id '" << id
                << "' – keeping first registration.";
        }
        else
        {
            m_idMap.insert(id, widget);
        }
    }
}

void FParserXml::clearStore()
{
    m_idMap.clear();
    m_allWidgets.clear();
}

// ============================================================================
//  Static helpers
// ============================================================================

QBoxLayout *FParserXml::makeLayout(const QString &direction)
{
    return direction.toLower() == "horizontal"
               ? static_cast<QBoxLayout *>(new QHBoxLayout())
               : static_cast<QBoxLayout *>(new QVBoxLayout());
}

QStringList FParserXml::parseOptions(const QString &raw)
{
    QString trimmed = raw.trimmed();
    if (trimmed.startsWith('{'))
        trimmed.remove(0, 1);
    if (trimmed.endsWith('}'))
        trimmed.chop(1);

    QStringList items;
    for (const QString &item : trimmed.split(','))
        items << item.trimmed();

    items.removeAll(QString());
    return items;
}

void FParserXml::skipToEndElement(QXmlStreamReader &xml, const QString &tag)
{
    // If the element was self-closing the reader is already on its end-element.
    if (xml.isEndElement() && xml.name().toString().toLower() == tag.toLower())
        return;

    while (!xml.atEnd())
    {
        xml.readNext();
        if (xml.isEndElement() &&
            xml.name().toString().toLower() == tag.toLower())
            return;
    }
}
