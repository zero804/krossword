/*
*   Copyright 2010 Friedrich Pülz <fpuelz@gmx.de>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Library General Public License as
*   published by the Free Software Foundation; either version 2 or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details
*
*   You should have received a copy of the GNU Library General Public
*   License along with this program; if not, write to the
*   Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "cluemodel.h"
#include "cells/cluecell.h"

#include <KLocalizedString>

ClueItem::ClueItem(ClueCell *clueCell) : QStandardItem(clueCell->clueWithNumber())
{
    m_clueCell = clueCell;

    setEditable(true);
    if (clueCell->clueNumber() != -1) {
        setData(clueCell->clueNumber(), Qt::UserRole + 1);
    } else {
        setData(clueCell->clue(), Qt::UserRole + 1);
    }
}

QVariant ClueItem::data(int role) const
{
    if (role == Qt::EditRole) {
        return m_clueCell->clue();
    } else if (role == Qt::DisplayRole) {
        return m_clueCell->clueWithNumber("<table><tr><td><b>%1. </b></td><td>%2</td></tr></table>"); //("<b>%1. </b>%2")
    } else {
        return QStandardItem::data(role);
    }
}

void ClueItem::setData(const QVariant& value, int role)
{
    if (role == Qt::EditRole)
        emit changeClueTextRequest(m_clueCell, value.toString());
    else if (role == Qt::DisplayRole)
        ; // Nothing to do
    else
        QStandardItem::setData(value, role);
}


ClueModel::ClueModel(QObject* parent) : QStandardItemModel(0, 1, parent)
{
    setSortRole(Qt::UserRole + 1);

    m_itemHorizontal = new QStandardItem("<b>" + i18n("Across clues") + "</b>");
    m_itemVertical = new QStandardItem("<b>" + i18n("Down clues") + "</b>");

    m_itemHorizontal->setEditable(false);
    m_itemVertical->setEditable(false);

    // Put across clues before down clues
    m_itemHorizontal->setData(0, Qt::UserRole + 1);
    m_itemVertical->setData(1, Qt::UserRole + 1);

    appendRow(m_itemHorizontal);
    appendRow(m_itemVertical);
}

void ClueModel::clear()
{
    m_itemHorizontal->removeRows(0, m_itemHorizontal->rowCount());
    m_itemVertical->removeRows(0, m_itemVertical->rowCount());
}

void ClueModel::addClue(ClueCell* clueCell)
{
    QStandardItem *item = clueCell->isHorizontal() ? m_itemHorizontal : m_itemVertical;

    ClueItem *clueItem = new ClueItem(clueCell);
    connect(clueItem, SIGNAL(changeClueTextRequest(ClueCell*, QString)),
            this, SLOT(changeClueTextRequested(ClueCell*, QString)));

    item->appendRow(QList<QStandardItem*>() << clueItem);

    connect(clueCell, SIGNAL(orientationChanged(ClueCell*, Qt::Orientation)),
            this, SLOT(updateClueOrientation(ClueCell*, Qt::Orientation)));
    connect(clueCell, SIGNAL(clueTextChanged(ClueCell*, QString)),
            this, SLOT(updateClueText(ClueCell*, QString)));
}

void ClueModel::updateClueOrientation(ClueCell* clue, Qt::Orientation newOrientation)
{
    Q_UNUSED(newOrientation);

    ClueItem *item = clueItem(clue);
    if (!item
            || (item->parent() == m_itemHorizontal && clue->isHorizontal())
            || (item->parent() == m_itemVertical && clue->isVertical()))
        return;

    removeRow(item->row(), item->parent()->index());
    addClue(clue);
}

void ClueModel::updateClueText(ClueCell* clue, const QString& newText)
{
    Q_UNUSED(newText);
    ClueItem *item = clueItem(clue);
    if (item) {
        emit itemChanged(item);
    }
}

void ClueModel::removeClue(ClueCell* clueCell)
{
    ClueItem *clue = clueItem(clueCell);
    if (!clue) {
        return;
    }

    removeRow(clue->row(), clue->parent()->index());
}

ClueItem* ClueModel::clueItem(ClueCell* clueCell) const
{
    QStandardItem *item = clueCell->isHorizontal() ? m_itemHorizontal : m_itemVertical;

    for (int row = 0; row < item->rowCount(); ++row) {
        ClueItem *curClueItem = static_cast< ClueItem* >(item->child(row));
        if (curClueItem->clueCell() == clueCell) {
            return curClueItem;
        }
    }

    // Not found in list of the clues current orientation, now search the list
    // of clues in the other direction
    QStandardItem *otherItem = clueCell->isHorizontal()
                               ? m_itemVertical : m_itemHorizontal;
    for (int row = 0; row < otherItem->rowCount(); ++row) {
        ClueItem *curClueItem = static_cast< ClueItem* >(otherItem->child(row));
        if (curClueItem->clueCell() == clueCell) {
            return curClueItem;
        }
    }

    qDebug() << "Clue not found in model" << clueCell;
    return NULL; // Item for clueCell not found
}

ClueItem* ClueModel::clueItemFromIndex(const QModelIndex& index) const
{
    if (index.parent() != m_itemHorizontal->index()
            && index.parent() != m_itemVertical->index()) {
        return NULL; // No clue item
    }

    QModelIndex clueItemIndex = index.parent().child(index.row(), 0);
    return dynamic_cast<ClueItem*>(itemFromIndex(clueItemIndex));
}
