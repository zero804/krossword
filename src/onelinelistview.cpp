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

#include "onelinelistview.h"

#include <qevent.h>

OneLineListView::OneLineListView(QWidget* parent)
    : QListView(parent)
{

}

QRect OneLineListView::visualRect(const QModelIndex& index) const
{
    QRect rect = QListView::visualRect(index);
    if (height() > width()) {   //flow() == TopToBottom ) {
        rect.setLeft(0);
        rect.setWidth(gridSize().width());
    } else {
        rect.setTop(0);
        rect.setHeight(gridSize().height());
    }
    return rect;
}

void OneLineListView::resizeEvent(QResizeEvent *e)
{
    if (height() > width()) {   //flow() == TopToBottom )
        setGridSize(QSize(e->size().width(), iconSize().height()
                          + fontMetrics().height() * 2 + 10));
    } else {
        setGridSize(QSize(qMax(iconSize().width(),
                               fontMetrics().width("Abcdefghijklmnop")),
                          e->size().height()));
    }
    QListView::resizeEvent(e);
}

