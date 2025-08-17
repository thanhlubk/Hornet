#pragma once
#include "FancyWidgetsExport.h"
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QTreeWidget>

namespace FUtilTree
{
    FANCYWIDGETS_EXPORT bool isExpandedInView(const QTreeView *view, const QModelIndex &index);

    FANCYWIDGETS_EXPORT bool isLastColumn(const QModelIndex &index);

    FANCYWIDGETS_EXPORT bool isLastChildOfParent(const QModelIndex &index);

    FANCYWIDGETS_EXPORT bool isPreviousIndexHasChildren(const QModelIndex &index);

    FANCYWIDGETS_EXPORT bool isNextIndexHasChildren(const QModelIndex &index);

    FANCYWIDGETS_EXPORT bool isPreviousIndexExpandedInView(const QTreeView *view, const QModelIndex &index);

    FANCYWIDGETS_EXPORT bool isParentLastChildOfItsParent(const QModelIndex &index);

    FANCYWIDGETS_EXPORT inline bool isTopLevelFirst(const QModelIndex &index)
    {
        return (index.row() == 0 && !index.parent().isValid());
    }
    
    FANCYWIDGETS_EXPORT inline bool isChild(const QModelIndex &index)
    {
        return index.parent().isValid();
    }

    FANCYWIDGETS_EXPORT inline bool isParent(const QModelIndex &index)
    {
        const QAbstractItemModel *m = index.model();
        if (!m || !index.isValid())
            return false;

        return m->hasChildren(index);
    }

    FANCYWIDGETS_EXPORT inline bool isSiblingParent(const QModelIndex &index)
    {
        if (index.column() != 0) // check only first column
            return false;

        auto nextIndex = index.sibling(index.row(), 0);
        return isParent(nextIndex);
    }

    FANCYWIDGETS_EXPORT inline int getIndentationFromStyleOption(const QStyleOptionViewItem &option)
    {
        const QTreeView *pTreeView = qobject_cast<const QTreeView *>(option.widget);
        if (pTreeView)
            return pTreeView->indentation();

        return 0; // default indentation
    }

}