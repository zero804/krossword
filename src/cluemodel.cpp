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
#include <KEmoticons>
#include <KIcon>


ClueItem::ClueItem(const QIcon& icon, ClueCell *clueCell)
    : QStandardItem(icon, clueCell->clueWithNumber())
{
    m_clueCell = clueCell;

    setEditable(true);
    if (clueCell->clueNumber() != -1)
        setData(clueCell->clueNumber(), Qt::UserRole + 1);
    else
        setData(clueCell->clue(), Qt::UserRole + 1);
}

QVariant ClueItem::data(int role) const
{
    if (role == Qt::EditRole)
        return m_clueCell->clue();
    else if (role == Qt::DisplayRole)
        return m_clueCell->clueWithNumber(
                   "<table><tr><td><b>%1. </b></td><td>%2</td></tr></table>");
    else
        return QStandardItem::data(role);
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


ClueModel::ClueModel(QObject* parent)
    : QStandardItemModel(0, 2, parent)
{
    setHorizontalHeaderLabels(
        QStringList() << i18n("Clue") << i18n("Answer"));
    setSortRole(Qt::UserRole + 1);

    m_itemHorizontal = new QStandardItem("<b><h3>" + i18n("Across clues") + "</h3></b>");
    m_itemVertical = new QStandardItem("<b><h3>" + i18n("Down clues") + "</h3></b>");

    QFont font = m_itemHorizontal->font();
    font.setBold(true);
    m_itemHorizontal->setFont(font);
    m_itemVertical->setFont(font);
    m_itemHorizontal->setEditable(false);
    m_itemVertical->setEditable(false);

    // Sort across clue list before down clue list
    m_itemHorizontal->setData(0, Qt::UserRole + 1);
    m_itemVertical->setData(1, Qt::UserRole + 1);

    appendRow(m_itemHorizontal);
    appendRow(m_itemVertical);

    // Get emoticon icon names
    KEmoticonsTheme emoTheme = KEmoticons().theme();
    QHash<QString, QStringList> emoticonsMap = emoTheme.emoticonsMap();
    for (QHash<QString, QStringList>::const_iterator it = emoticonsMap.constBegin();
            it != emoticonsMap.constEnd(); ++it) {
        if ((*it).contains(":(") || (*it).contains(":-(")) {
            m_iconSad = it.key();
            break;
        }
    }
}

void ClueModel::clear()
{
    m_itemHorizontal->removeRows(0, m_itemHorizontal->rowCount());
    m_itemVertical->removeRows(0, m_itemVertical->rowCount());
}

void ClueModel::addClue(ClueCell* clueCell)
{
    QStandardItem *item = clueCell->isHorizontal() ? m_itemHorizontal : m_itemVertical;

    QStandardItem *currentAnswerItem = new QStandardItem(clueCell->currentAnswer());
    currentAnswerItem->setEditable(false);

    ClueItem *clueItem = new ClueItem(KIcon(m_iconSad), clueCell);
    connect(clueItem, SIGNAL(changeClueTextRequest(ClueCell*, QString)),
            this, SLOT(changeClueTextRequested(ClueCell*, QString)));

    item->appendRow(QList<QStandardItem*>() << clueItem << currentAnswerItem);

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
    if (item)
        emit itemChanged(item);
}

void ClueModel::removeClue(ClueCell* clueCell)
{
    ClueItem *clue = clueItem(clueCell);
    if (!clue)
        return;

    removeRow(clue->row(), clue->parent()->index());
}

ClueItem* ClueModel::clueItem(ClueCell* clueCell) const
{
    QStandardItem *item = clueCell->isHorizontal()
                          ? m_itemHorizontal : m_itemVertical;

    for (int row = 0; row < item->rowCount(); ++row) {
        ClueItem *curClueItem = static_cast< ClueItem* >(item->child(row));
        if (curClueItem->clueCell() == clueCell)
            return curClueItem;
    }

    // Not found in list of the clues current orientation, now search the list
    // of clues in the other direction
    QStandardItem *otherItem = clueCell->isHorizontal()
                               ? m_itemVertical : m_itemHorizontal;
    for (int row = 0; row < otherItem->rowCount(); ++row) {
        ClueItem *curClueItem = static_cast< ClueItem* >(otherItem->child(row));
        if (curClueItem->clueCell() == clueCell)
            return curClueItem;
    }

    kDebug() << "Clue not found in model" << clueCell;
    return NULL; // Item for clueCell not found
}

ClueItem* ClueModel::clueItemFromIndex(const QModelIndex& index) const
{
    if (index.parent() != m_itemHorizontal->index()
            && index.parent() != m_itemVertical->index())
        return NULL; // No clue item

    QModelIndex clueItemIndex = index.parent().child(index.row(), 0);
    return dynamic_cast<ClueItem*>(itemFromIndex(clueItemIndex));
}

QStandardItem* ClueModel::answerItem(ClueCell* clueCell) const
{
    ClueItem *_item = clueItem(clueCell);
    if (!_item)
        return NULL;

//     QStandardItem *parentItem = clueCell->isHorizontal()
//      ? m_itemHorizontal : m_itemVertical;
    return _item->parent()->child(_item->row(), 1);
}






