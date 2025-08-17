#include <FancyWidgets/FUtilTree.h>

namespace FUtilTree
{
    bool isExpandedInView(const QTreeView *view, const QModelIndex &index)
    {
        if (!view || !index.isValid())
            return false;

        const QAbstractItemModel *m = index.model();
        if (!m)
            return false;

        auto firstColumnIndex = index.sibling(index.row(), 0); // check column 0 by convention
        if (m->hasChildren(firstColumnIndex))
            return view->isExpanded(firstColumnIndex);
        return false; // check column 0 by convention
    }
    
    bool isLastColumn(const QModelIndex &index)
    {
        const QAbstractItemModel *m = index.model();
        if (!m || !index.isValid())
            return false;

        const int cols = m->columnCount(index.parent()); // columns are defined per parent
        return cols > 0 && index.column() == (cols - 1);
    }
    
    bool isLastChildOfParent(const QModelIndex &index)
    {
        const QAbstractItemModel *m = index.model();
        if (!m || !index.isValid())
            return false;

        const QModelIndex parent = index.parent(); // may be invalid for top-level rows
        const int rowCount = m->rowCount(parent);  // number of siblings
        if (rowCount <= 0)
            return false;

        return index.row() == (rowCount - 1);
    }
    
    bool isPreviousIndexHasChildren(const QModelIndex &index)
    {
        const QAbstractItemModel *m = index.model();
        if (!m || !index.isValid())
            return false;

        const QModelIndex parent = index.parent(); // may be invalid for top-level rows
        const int prevRow = index.row() - 1;
        if (prevRow < 0)
            return false; // no previous sibling

        const QModelIndex prev = m->index(prevRow, 0, parent); // check column 0 by convention
        return prev.isValid() && m->hasChildren(prev);
    }
    
    bool isNextIndexHasChildren(const QModelIndex &index)
    {
        if (!index.parent().isValid())
            return false; // current is top-level, no parent
        const QAbstractItemModel *m = index.model();
        const QModelIndex parent = index.parent();

        const int nextRow = index.row() + 1;
        const int rowCount = m->rowCount(parent);
        if (nextRow >= rowCount)
            return false; // no next sibling

        // check column 0 (conventionally used to test child presence)
        const QModelIndex next = m->index(nextRow, 0, parent);
        return next.isValid() && m->hasChildren(next);
    }
    
    bool isPreviousIndexExpandedInView(const QTreeView *view, const QModelIndex &index)
    {
        if (!view || !index.isValid())
            return false;

        const QAbstractItemModel *m = index.model();
        if (!m)
            return false;

        const QModelIndex parent = index.parent(); // may be invalid for top-level rows
        const int prevRow = index.row() - 1;
        if (prevRow < 0)
            return false; // no previous sibling

        const QModelIndex prev = m->index(prevRow, 0, parent); // check column 0 by convention
        return isExpandedInView(view, prev);
        // if (prev.isValid() && m->hasChildren(prev))
        //     return view->isExpanded(prev.sibling(prev.row(), 0));
        // return false;
    }
    
    bool isParentLastChildOfItsParent(const QModelIndex &index)
    {
        const QAbstractItemModel *m = index.model();
        if (!m || !index.isValid())
            return false;

        const QModelIndex parent = index.parent(); // may be invalid for top-level rows
        if (!parent.isValid())
            return false; // no parent, so can't be last child of parent

        return isLastChildOfParent(parent);
    }
}