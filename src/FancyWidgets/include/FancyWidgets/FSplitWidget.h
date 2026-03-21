#pragma once
#include "FancyWidgetsExport.h"
#include "FThemeableWidget.h"
#include <QWidget>
#include <Qt>
#include "FViewHost.h"
#include <unordered_map>
#include <QWidget>
#include <QMetaType>

// remove this in mordern Qt
// Q_DECLARE_METATYPE(QWidget*)

class FComboBox;
class QVBoxLayout;
class QPushButton;
class QSplitter;

class FANCYWIDGETS_EXPORT FSplitWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FSplitWidget(QWidget *parent = nullptr);

    // When this widget is a leaf node, returns its FViewHost.
    // Returns nullptr if the widget is blank or currently holds a splitter.
    FViewHost* viewHostWidget() const;
    QWidget* viewWidget() const; // returns current view widget for this leaf, else nullptr

    QList<FSplitWidget*> childLeaf() const;
    FSplitWidget* root();

    // Current active leaf (for THIS subtree; if you call on a child, you get the active
    // leaf of that child's subtree)
    FSplitWidget* activeLeaf();

    void setViewPool(std::unordered_map<QWidget*, QString>* pool);
    void updateCombo(); // call on any leaf; it runs from root()

signals:
    // Emitted right *before* the internal split logic runs
    void beforeSplit(FSplitWidget* node, Qt::Orientation orientation);
    void activeLeafChanged(FSplitWidget* node);

public slots:
    // Create the top-most node if there is none.
    // If a node already exists (leaf or splitter), this does nothing.
    void initLeaf();

private slots:
    void splitHorizontally();
    void splitVertically();
    void closeLeaf();
    void comboIndexChanged(int index);

private:
    // Actually builds a single leaf (controls + FViewHost) in this widget.
    void createLeafInternal();

    // Return this widget to the blank initial state (no leaf).
    void clearToBlank();
    bool isBlank() const;

    void makeSplit(Qt::Orientation orientation, QWidget *newViewWidget);
    QWidget *createViewWidget();

    // Helper used by a child leaf to collapse its parent splitter
    void collapseSplitter(QSplitter *splitter, FSplitWidget *keep, FSplitWidget *remove);

    void setActiveLeaf(FSplitWidget* leaf);

    void updateComboForLeaf(const QList<QWidget*>& unusedWidgets);

    QVBoxLayout *m_pMainLayout = nullptr;
    QWidget *m_pCentralWidget = nullptr;  // leaf or QSplitter
    QWidget *m_pControlWidget = nullptr; // the button row
    FComboBox* m_combo = nullptr; // only exists on leaf
    std::unordered_map<QWidget*, QString>* m_viewPool = nullptr;
};
