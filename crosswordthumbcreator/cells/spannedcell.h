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

class SpannedCell : public KrossWordCell
{
public:
    SpannedCell( KrossWord* krossWord, KrossWordCell::CellType cellType,
                 const Coord& coordTopLeft, int horizontalCellSpan,
                 int verticalCellSpan );

    virtual QRectF boundingRect() const;

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 7 };
    virtual int type() const {
        return Type;
    };

    int horizontalCellSpan() const {
        return m_horizontalCellSpan;
    };
    int verticalCellSpan() const {
        return m_verticalCellSpan;
    };

    inline Coord coordTopLeft() const {
        return coord();
    };
    inline Coord coordTopRight() const {
        return Coord( coord().first + m_horizontalCellSpan - 1, coord().second );
    };
    inline Coord coordBottomLeft() const {
        return Coord( coord().first, coord().second + m_verticalCellSpan - 1 );
    };
    inline Coord coordBottomRight() const {
        return Coord( coord().first + m_horizontalCellSpan - 1,
                      coord().second + m_verticalCellSpan - 1 );
    };

private:
    int m_horizontalCellSpan;
    int m_verticalCellSpan;
};

#endif // SPANNEDCELL_H
