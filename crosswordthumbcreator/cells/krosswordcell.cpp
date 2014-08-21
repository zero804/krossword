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

#include "krosswordcell.h"
#include "krossword.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <qevent.h>
#include <QDebug>
#include <kdeversion.h>

Offset operator *(const Offset &offset, int factor)
{
    return Offset(offset.first * factor, offset.second * factor);
}

Coord operator+=(Coord & coord1, const Coord & coord2)
{
    return coord1 = coord1 + coord2;
}

KrossWordCell::KrossWordCell(KrossWord* krossWord, CellType cellType,
                             const Coord& coord)
    : QGraphicsObject(krossWord),
      m_cache(0)
{
    m_krossWord = krossWord;
    m_cellType = cellType;
    m_coord = coord;

    setPositionFromCoordinates();
}

KrossWordCell::~KrossWordCell()
{
    delete m_cache;
}

QString KrossWordCell::displayStringFromCellType(KrossWordCell::CellType cellType)
{
    switch (cellType) {
    case EmptyCellType:
        return "Empty cell";
    case ClueCellType:
        return "Clue cell";
    case DoubleClueCellType:
        return "Double clue cell";
    case LetterCellType:
        return "Letter cell";
    case SolutionLetterCellType:
        return "Solution letter cell";
    case ImageCellType:
        return "Image cell";
    case AllCellTypes:
        return "All cells";
    case InteractiveCellTypes:
        return "Interactive cells";
    case UserType:
        return "User cell type";

    default:
        return QString("Unknown cell type (%1)").arg(static_cast<int>(cellType));
    }
}

void KrossWordCell::setPositionFromCoordinates()
{
    // "1 +" for spacing
    setPos(coord().first * (/* 1 +*/ krossWord()->cellSize().width() / 2),
           coord().second * (/* 1 +*/ krossWord()->cellSize().height() / 2));
}

QRectF KrossWordCell::boundingRect() const
{
//     qDebug() << "bounding size =" << krossWord()->cellSize();
//     qreal penWidth = 1;
//     return QRectF( pos().x() - penWidth / 2, pos().y() - penWidth / 2,
//      krossWord()->cellSize().width() + penWidth,
//      krossWord()->cellSize().height() + penWidth );
    return QRectF(pos(), krossWord()->cellSize());
}


EmptyCell::EmptyCell(KrossWord* krossWord, Coord coord)
    : KrossWordCell(krossWord, EmptyCellType, coord)
{
}

void EmptyCell::drawBackgroundForPrinting(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    p->fillRect(option->rect, krossWord()->emptyCellColorForPrinting());
}

void KrossWordCell::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    drawBackgroundForPrinting(painter, option);
    drawForegroundForPrinting(painter, option);
}


// Sorting functions:
bool lessThanCellType(const KrossWordCell* cell1, const KrossWordCell* cell2)
{
    return cell1->cellType() < cell2->cellType();
}

bool greaterThanCellType(const KrossWordCell* cell1, const KrossWordCell* cell2)
{
    return cell1->cellType() > cell2->cellType();
}


