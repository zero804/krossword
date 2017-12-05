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

#include <QPropertyAnimation>

namespace Crossword
{

SpannedCell::SpannedCell(KrossWord* krossWord, CellType cellType,
                         const Coord& coordTopLeft, int horizontalCellSpan,
                         int verticalCellSpan)
    : KrossWordCell(krossWord, cellType, coordTopLeft)
{
    m_horizontalCellSpan = horizontalCellSpan;
    m_verticalCellSpan = verticalCellSpan;

    m_transitionSize = QSizeF();
}

QRectF SpannedCell::boundingRect() const
{
    if (m_transitionSize.isValid()) {
        return QRectF(QPointF(-krossWord()->getCellSize().width() / 2 - 0.5,
                              -krossWord()->getCellSize().height() / 2 - 0.5),
                      m_transitionSize);
    } else {
        return QRectF(-krossWord()->getCellSize().width() / 2 - 0.5,
                      -krossWord()->getCellSize().height() / 2 - 0.5,
                      krossWord()->getCellSize().width() * m_horizontalCellSpan + 1,
                      krossWord()->getCellSize().height() * m_verticalCellSpan + 1);
    }
}

int SpannedCell::type() const {
    return Type;
}

QSizeF SpannedCell::transitionSize() const {
    return m_transitionSize;
}

void SpannedCell::setTransitionSize(const QSizeF& transitionSize)
{
    prepareGeometryChange();
    m_transitionSize = transitionSize;
    clearCache();
    update();
}

int SpannedCell::horizontalCellSpan() const {
    return m_horizontalCellSpan;
}

int SpannedCell::verticalCellSpan() const {
    return m_verticalCellSpan;
}

void SpannedCell::endSizeTransizionAnim()
{
    prepareGeometryChange();
    m_transitionSize = QSizeF();
    clearCache();
    update();
}

void SpannedCell::setCellSpan(int horizontalCellSpan, int verticalCellSpan)
{
    QList<Coord> coordsBefore = spannedCoords();

    if (krossWord()->isAnimationEnabled()) {
        QPropertyAnimation *transitionSizeAnim = new QPropertyAnimation(this, "transitionSize");
        transitionSizeAnim->setDuration(krossWord()->animator()->defaultDuration());
        transitionSizeAnim->setStartValue(boundingRect().size());
        transitionSizeAnim->setEndValue(QSizeF(
                                            krossWord()->getCellSize().width() * horizontalCellSpan,
                                            krossWord()->getCellSize().width() * verticalCellSpan));
        connect(transitionSizeAnim, SIGNAL(finished()),
                this, SLOT(endSizeTransizionAnim()));
        transitionSizeAnim->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        prepareGeometryChange();
    }

    m_horizontalCellSpan = horizontalCellSpan;
    m_verticalCellSpan = verticalCellSpan;

    QList<Coord> coordsAfter = spannedCoords();

    // Set image cell into the crossword grid at all new coords.
    foreach(const Coord & coord, coordsAfter) {
        if (!coordsBefore.contains(coord)) {
            krossWord()->replaceCell(coord, this);
        }
    }

    // Set new empty cells into the crossword grid at all old coords.
    foreach(const Coord & coord, coordsBefore) {
        if (!coordsAfter.contains(coord)) {
            EmptyCell *emptyCell = new EmptyCell(krossWord(), coord);
            emptyCell->setZValue(-10);
            krossWord()->replaceCell(coord, emptyCell, false);
        }
    }
}

QList<Coord> SpannedCell::spannedCoords() const
{
    QList<Coord> coords;
    Coord coord;
    for (coord.first = coordTopLeft().first; coord.first <= coordBottomRight().first; ++coord.first) {
        for (coord.second = coordTopLeft().second; coord.second <= coordBottomRight().second; ++coord.second) {
            coords << coord;
        }
    }
    return coords;
}

void SpannedCell::setHorizontalCellSpan(int horizontalCellSpan)
{
    setCellSpan(horizontalCellSpan, m_verticalCellSpan);
}

void SpannedCell::setVerticalCellSpan(int verticalCellSpan)
{
    setCellSpan(m_horizontalCellSpan, verticalCellSpan);
}

bool SpannedCell::inside(const Grid2D::Coord &coord) {
    return coord >= coordTopLeft() && coord <= coordBottomRight();
}

Grid2D::Coord SpannedCell::coordTopLeft() const
{
    return coord();
}

Grid2D::Coord SpannedCell::coordTopRight() const
{
    return Coord(coord().first + m_horizontalCellSpan - 1,
                 coord().second);
}

Grid2D::Coord SpannedCell::coordBottomLeft() const
{
    return Coord(coord().first,
                 coord().second + m_verticalCellSpan - 1);
}

Grid2D::Coord SpannedCell::coordBottomRight() const
{
    return Coord(coord().first + m_horizontalCellSpan - 1,
                 coord().second + m_verticalCellSpan - 1);
}

} // namespace Crossword
