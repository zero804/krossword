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
#if QT_VERSION >= 0x040600
    Q_PROPERTY( QSizeF transitionSize READ transitionSize WRITE setTransitionSize FINAL )
#endif
    friend class KrossWord;

public:
    SpannedCell( KrossWord* krossWord, CellType cellType,
                 const Coord& coordTopLeft, int horizontalCellSpan,
                 int verticalCellSpan );

    virtual QRectF boundingRect() const;

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 7 };
    virtual int type() const {
        return Type;
    };

#if QT_VERSION >= 0x040600
    QSizeF transitionSize() const {
        return m_transitionSize;
    };
    void setTransitionSize( const QSizeF &transitionSize );
#endif

    int horizontalCellSpan() const {
        return m_horizontalCellSpan;
    };
    int verticalCellSpan() const {
        return m_verticalCellSpan;
    };
    void setCellSpan( int horizontalCellSpan, int verticalCellSpan );
    inline void setHorizontalCellSpan( int horizontalCellSpan );
    inline void setVerticalCellSpan( int verticalCellSpan );

    bool inside( const Coord &coord ) {
        return coord >= coordTopLeft() && coord <= coordBottomRight();
    };

    inline Coord coordTopLeft() const;
    inline Coord coordTopRight() const;
    inline Coord coordBottomLeft() const;
    inline Coord coordBottomRight() const;

    QList<Coord> spannedCoords() const;

protected slots:
#if QT_VERSION >= 0x040600
    void endSizeTransizionAnim();
#endif

private:
    int m_horizontalCellSpan;
    int m_verticalCellSpan;

#if QT_VERSION >= 0x040600
    QSizeF m_transitionSize;
#endif
};

inline void SpannedCell::setHorizontalCellSpan( int horizontalCellSpan )
{
    setCellSpan( horizontalCellSpan, m_verticalCellSpan );
};

inline void SpannedCell::setVerticalCellSpan( int verticalCellSpan )
{
    setCellSpan( m_horizontalCellSpan, verticalCellSpan );
};

inline Coord SpannedCell::coordTopLeft() const
{
    return coord();
};
inline Coord SpannedCell::coordTopRight() const
{
    return Coord( coord().first + m_horizontalCellSpan - 1,
                  coord().second );
};
inline Coord SpannedCell::coordBottomLeft() const
{
    return Coord( coord().first,
                  coord().second + m_verticalCellSpan - 1 );
};
inline Coord SpannedCell::coordBottomRight() const
{
    return Coord( coord().first + m_horizontalCellSpan - 1,
                  coord().second + m_verticalCellSpan - 1 );
};

}; // namespace Crossword

#endif // SPANNEDCELL_H
