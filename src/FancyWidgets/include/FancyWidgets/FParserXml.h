#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QBoxLayout>
#include <QXmlStreamReader>
#include <QString>
#include <QList>
#include <QMap>

// Forward-declare your custom widget types
class FGroupBox;
class FLabel;
class FLineEdit;
class FComboBox;
class FCheckBox;
class FPushButton;

/**
 * @brief FParserXml
 *
 * Reads an XML file (or raw XML string) and constructs a widget tree from
 * your custom F-widgets.  The root element can be ANY supported tag – the
 * parser does NOT assume a <dialog> wrapper.
 *
 * ─── Supported root / child elements ───────────────────────────────────────
 *
 *  <dialog>     QDialog.     Attrs: id, title, width, height, layout
 *  <groupbox>   FGroupBox.   Attrs: id, title, layout ("vertical"|"horizontal")
 *  <widget>     QWidget.     Attrs: id, layout ("vertical"|"horizontal")
 *  <label>      FLabel.      Attrs: id, text
 *  <lineedit>   FLineEdit.   Attrs: id, name, placeholder, text
 *  <combobox>   FComboBox.   Attrs: id, name, options="{A, B, C}"
 *  <checkbox>   FCheckBox.   Attrs: id, text, checked ("true"|"false")
 *  <pushbutton> FPushButton. Attrs: id, text, name
 *  <layout>     (structural only – not a widget). Attrs: type
 *  <spacer>     (structural only – not a widget). Attrs: type, factor, width, height
 *
 * ─── id & widget store ──────────────────────────────────────────────────────
 *
 *  Every widget that carries an `id` attribute is stored in an internal map.
 *  ALL created widgets (with or without id) are stored in an ordered list.
 *
 *  findById(id)        – O(1) lookup; only works for widgets that had an id.
 *  findById<T>(id)     – same, with a qobject_cast to T*.
 *  allWidgets()        – every widget in document (creation) order.
 *
 *  The store is cleared on each call to fromFile() / fromString(), so one
 *  FParserXml instance can be reused for multiple files.
 *
 * ─── Usage ──────────────────────────────────────────────────────────────────
 *
 *  FParserXml parser;
 *
 *  // Root is a full dialog
 *  QWidget *root = parser.fromFile("config.xml");
 *  auto *dlg = qobject_cast<QDialog *>(root);
 *  if (dlg) dlg->exec();
 *
 *  // Root is just a standalone combobox
 *  QWidget *root2 = parser.fromFile("combo_only.xml");
 *  auto *combo = qobject_cast<FComboBox *>(root2);
 *
 *  // Retrieve by id (O(1), typed)
 *  auto *okBtn = parser.findById<FPushButton>("btnOk");
 *
 *  // Iterate everything including id-less widgets
 *  for (QWidget *w : parser.allWidgets())
 *      w->setEnabled(false);
 */
class FANCYWIDGETS_EXPORT FParserXml
{
public:
    FParserXml() = default;

    // ── Parse entry-points ──────────────────────────────────────────────────

    /**
     * Parse an XML file. Returns the root widget, or nullptr on error.
     * Caller takes ownership of the returned widget.
     * The internal widget store is cleared before parsing begins.
     */
    QWidget *fromFile(const QString &filePath);

    /**
     * Parse a raw XML string. Returns the root widget, or nullptr on error.
     * The internal widget store is cleared before parsing begins.
     */
    QWidget *fromString(const QString &xmlContent);

    // ── Widget store ────────────────────────────────────────────────────────

    /**
     * Find a widget by its XML `id` attribute (O(1)).
     * Returns nullptr if no widget with that id was created in the last parse.
     */
    QWidget *findById(const QString &id) const;

    /**
     * Find a widget by id and cast it to T* via qobject_cast.
     * Returns nullptr if the id is unknown or the cast fails.
     *
     *   auto *btn = parser.findById<FPushButton>("btnOk");
     */
    template <typename T>
    T *findById(const QString &id) const
    {
        return qobject_cast<T *>(findById(id));
    }

    /**
     * All widgets created during the last parse, in document order.
     * Includes widgets that had no `id` attribute.
     */
    QList<QWidget *> allWidgets() const;

    /** Human-readable description of the last parse error, if any. */
    QString lastError() const { return m_lastError; }

private:
    // ── Orchestration ───────────────────────────────────────────────────────

    /// Internal driver; operates on an already-positioned reader.
    QWidget *parse(QXmlStreamReader &xml);

    /**
     * Inspect the current start-element tag and dispatch to the correct
     * parser. Returns nullptr for structural-only elements (<layout>, <spacer>).
     * When called as root, `parentLayout` is nullptr.
     */
    QWidget *dispatchElement(QXmlStreamReader &xml,
                             QBoxLayout *parentLayout);

    // ── Container parsers (own a layout, recurse via parseChildren) ─────────
    QWidget *parseDialog(QXmlStreamReader &xml);
    QWidget *parseGroupBox(QXmlStreamReader &xml);
    QWidget *parsePlainWidget(QXmlStreamReader &xml); // <widget layout="…">

    // ── Leaf widget parsers ──────────────────────────────────────────────────
    QWidget *parseLabel(QXmlStreamReader &xml);
    QWidget *parseLineEdit(QXmlStreamReader &xml);
    QWidget *parseComboBox(QXmlStreamReader &xml);
    QWidget *parseCheckBox(QXmlStreamReader &xml);
    QWidget *parsePushButton(QXmlStreamReader &xml);

    /**
     * Walk child elements of a container, adding each widget / layout to
     * `parentLayout`. Stops after consuming the closing tag of `parentTag`.
     */
    void parseChildren(QXmlStreamReader &xml,
                       QBoxLayout *parentLayout,
                       const QString &parentTag);

    // ── Structural helpers (produce layouts / spacers, not widgets) ──────────
    void parseLayout(QXmlStreamReader &xml, QBoxLayout *parent);
    void parseSpacer(QXmlStreamReader &xml, QBoxLayout *parent);

    // ── Widget store helpers ─────────────────────────────────────────────────

    /**
     * Register `widget` in the store.
     *   - Always appended to m_allWidgets (creation order).
     *   - If `id` is non-empty, also inserted into m_idMap.
     *     A duplicate id emits qWarning(); the first registration wins.
     */
    void registerWidget(QWidget *widget, const QString &id);

    void clearStore();

    // ── Misc helpers ─────────────────────────────────────────────────────────
    static QBoxLayout *makeLayout(const QString &direction);
    static QStringList parseOptions(const QString &raw);

    /// Advance the reader until (and including) the end-element named `tag`.
    static void skipToEndElement(QXmlStreamReader &xml, const QString &tag);

    // ── State ─────────────────────────────────────────────────────────────────
    QMap<QString, QWidget *> m_idMap; ///< id  →  widget  (id-bearing only)
    QList<QWidget *> m_allWidgets;    ///< all widgets, creation order
    QString m_lastError;
};
