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

#if QT_VERSION >= 0x040600
#include <QPropertyAnimation>
#endif

namespace Crossword
{

SpannedCell::SpannedCell( KrossWord* krossWord, CellType cellType,
                          const Coord& coordTopLeft, int horizontalCellSpan,
                          int verticalCellSpan )
        : KrossWordCell( krossWord, cellType, coordTopLeft )
{
    m_horizontalCellSpan = horizontalCellSpan;
    m_verticalCellSpan = verticalCellSpan;

#if QT_VERSION >= 0x040600
    m_transitionSize = QSizeF();
//   setTransformOriginPoint( -m_transitionSize.width() / 2 - 0.5,
//       -m_transitionSize.height() / 2 - 0.5 );
#endif
}

QRectF SpannedCell::boundingRect() const
{
#if QT_VERSION >= 0x040600
    if ( m_transitionSize.isValid() ) {
        return QRectF( QPointF( -krossWord()->cellSize().width() / 2 - 0.5,
                                -krossWord()->cellSize().height() / 2 - 0.5 ),
                       m_transitionSize );
    } else {
        return QRectF( -krossWord()->cellSize().width() / 2 - 0.5,
                       -krossWord()->cellSize().height() / 2 - 0.5,
                       krossWord()->cellSize().width() * m_horizontalCellSpan + 1,
                       krossWord()->cellSize().height() * m_verticalCellSpan + 1 );
    }
#else
    return QRectF( -krossWord()->cellSize().width() / 2 - 0.5,
                   -krossWord()->cellSize().height() / 2 - 0.5,
                   krossWord()->cellSize().width() * m_horizontalCellSpan + 1,
                   krossWord()->cellSize().height() * m_verticalCellSpan + 1 );
#endif
}

#if QT_VERSION >= 0x040600
void SpannedCell::setTransitionSize( const QSizeF& transitionSize )
{
    prepareGeometryChange();
    m_transitionSize = transitionSize;
    clearCache();
    update();
}

void SpannedCell::endSizeTransizionAnim()
{
    prepareGeometryChange();
    m_transitionSize = QSizeF();
    clearCache();
    update();
}
#endif

void SpannedCell::setCellSpan( int horizontalCellSpan, int verticalCellSpan )
{
    QList< Coord > coordsBefore = spannedCoords();

#if QT_VERSION >= 0x040600
    if ( krossWord()->isAnimationTypeEnabled( AnimateSizeChange ) ) {
        QPropertyAnimation *transitionSizeAnim = new QPropertyAnimation( this, "transitionSize" );
        transitionSizeAnim->setDuration( krossWord()->animator()->defaultDuration() );
        transitionSizeAnim->setStartValue( boundingRect().size() );
        transitionSizeAnim->setEndValue( QSizeF(
                                             krossWord()->cellSize().width() * horizontalCellSpan,
                                             krossWord()->cellSize().width() * verticalCellSpan ) );
        connect( transitionSizeAnim, SIGNAL( finished() ),
                 this, SLOT( endSizeTransizionAnim() ) );
        transitionSizeAnim->start( QAbstractAnimation::DeleteWhenStopped );
    } else
        prepareGeometryChange();
#else
    prepareGeometryChange();
#endif
    m_horizontalCellSpan = horizontalCellSpan;
    m_verticalCellSpan = verticalCellSpan;

    QList< Coord > coordsAfter = spannedCoords();

    // Set image cell into the crossword grid at all new coords.
    foreach( const Coord &coord, coordsAfter ) {
        if ( !coordsBefore.contains( coord ) )
            krossWord()->replaceCell( coord, this );
    }

    // Set new empty cells into the crossword grid at all old coords.
    foreach( const Coord &coord, coordsBefore ) {
        if ( !coordsAfter.contains( coord ) ) {
            EmptyCell *emptyCell = new EmptyCell( krossWord(), coord );
            emptyCell->setZValue( -10 );
            krossWord()->replaceCell( coord, emptyCell, false );
        }
    }
}

QList< Coord > SpannedCell::spannedCoords() const
{
    QList<Coord> coords;
    Coord coord;
    for ( coord.first = coordTopLeft().first;
            coord.first <= coordBottomRight().first; ++coord.first ) {
        for ( coord.second = coordTopLeft().second;
                coord.second <= coordBottomRight().second; ++coord.second ) {
            coords << coord;
        }
    }
    return coords;
}

}; // namespace Crossword
