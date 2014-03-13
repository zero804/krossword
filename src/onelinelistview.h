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

#ifndef ONELINELISTVIEW_H
#define ONELINELISTVIEW_H

#include <QListView>

class OneLineListView : public QListView
{
public:
    OneLineListView(QWidget* parent = 0);

    virtual QRect visualRect(const QModelIndex& index) const;
    virtual void resizeEvent(QResizeEvent* e);

};

#endif // ONELINELISTVIEW_H
