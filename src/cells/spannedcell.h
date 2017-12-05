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

#ifndef SPANNEDCELL_H
#define SPANNEDCELL_H

#include "krosswordcell.h"

namespace Crossword
{

class SpannedCell : public KrossWordCell
{
    Q_OBJECT
    Q_PROPERTY(QSizeF transitionSize READ transitionSize WRITE setTransitionSize FINAL)
    friend class KrossWord;

public:
    SpannedCell(KrossWord* krossWord, CellType cellType,
                const Coord& coordTopLeft, int horizontalCellSpan,
                int verticalCellSpan);

    virtual QRectF boundingRect() const override;

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 7 };

    virtual int type() const override;

    QSizeF transitionSize() const;
    void setTransitionSize(const QSizeF &transitionSize);

    int horizontalCellSpan() const;
    int verticalCellSpan() const;

    void setCellSpan(int horizontalCellSpan, int verticalCellSpan);
    void setHorizontalCellSpan(int horizontalCellSpan);
    void setVerticalCellSpan(int verticalCellSpan);

    bool inside(const Coord &coord);

    Coord coordTopLeft() const;
    Coord coordTopRight() const;
    Coord coordBottomLeft() const;
    Coord coordBottomRight() const;

    QList<Coord> spannedCoords() const;

protected slots:
    void endSizeTransizionAnim();

private:
    int m_horizontalCellSpan;
    int m_verticalCellSpan;

    QSizeF m_transitionSize;
};

} // namespace Crossword

#endif // SPANNEDCELL_H
