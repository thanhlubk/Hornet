#pragma once
#include "FancyWidgetsExport.h"
#include "FThemeableWidget.h"
#include <QWidget>
#include <Qt>
#include "FViewHost.h"
#include <unordered_map>
#include <QWidget>
#include <QMetaType>

Q_DECLARE_METATYPE(QWidget*)

class QComboBox;
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
    FViewHost *viewWidget() const;
    QList<FSplitWidget*> leafNodes() const;
    FSplitWidget* topMostParent();

    // Current active leaf (for THIS subtree; if you call on a child, you get the active
    // leaf of that child's subtree)
    FSplitWidget* activeNode();

    void setViewPool(std::unordered_map<QWidget*, QString>* pool);
    void updateAllViewCombos(); // call on any node; it runs from topMostParent()

signals:
    // Emitted right *before* the internal split logic runs
    void beforeSplit(FSplitWidget* node, Qt::Orientation orientation);
    void activeNodeChanged(FSplitWidget* node);

public slots:
    // Create the top-most node if there is none.
    // If a node already exists (leaf or splitter), this does nothing.
    void createRootNode();

private slots:
    void splitHorizontally();
    void splitVertically();
    void closeNode();
    void onComboIndexChanged(int index);

private:
    // Set up the initial blank state (called by ctor)
    void createBlank();

    // Actually builds a single leaf node (controls + FViewHost) in this widget.
    void createNodeInternal();

    // Return this widget to the blank initial state (no node).
    void clearToBlank();
    bool isBlank() const;

    void makeSplit(Qt::Orientation orientation, QWidget *newViewWidget);
    QWidget *createViewWidget();

    // Helper used by a child node to collapse its parent splitter
    void collapseSplitter(QSplitter *splitter, FSplitWidget *keep, FSplitWidget *remove);

    void setActiveLeaf(FSplitWidget* leaf);

    void updateComboForLeaf(const QList<QWidget*>& unusedWidgets);
    QWidget* currentLeafView() const; // returns current view widget for this leaf, else nullptr

    QVBoxLayout *m_pMainLayout = nullptr;
    QWidget *m_pCentralWidget = nullptr;  // leaf or QSplitter
    QWidget *m_pControlWidget = nullptr; // the button row
    QComboBox* m_combo = nullptr; // only exists on leaf nodes
    std::unordered_map<QWidget*, QString>* m_viewPool = nullptr;
};
