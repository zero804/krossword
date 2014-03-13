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

#include "spannedcell.h"
#include "krossword.h"

SpannedCell::SpannedCell( KrossWord* krossWord, KrossWordCell::CellType cellType,
                          const Coord& coordTopLeft, int horizontalCellSpan,
                          int verticalCellSpan )
        : KrossWordCell( krossWord, cellType, coordTopLeft )
{
    m_horizontalCellSpan = horizontalCellSpan;
    m_verticalCellSpan = verticalCellSpan;
}

QRectF SpannedCell::boundingRect() const
{
    return QRectF( x(), y(),
                   krossWord()->cellSize().width() * m_horizontalCellSpan,
                   krossWord()->cellSize().width() * m_verticalCellSpan );
}
