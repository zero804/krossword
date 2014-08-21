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

#ifndef KROSSWORDCELL_H
#define KROSSWORDCELL_H

#include <qnamespace.h>
#include <QString>
#include <QPair>
#include <QPainter>
#include "kgrid2d.h"
#include <qstyleoption.h>

#include <QDebug>
#include <kdeversion.h>

#include <QGraphicsObject>

class KrossWord;
typedef KGrid2D::Coord Coord;
typedef KGrid2D::Coord Offset;

class KrossWordCell;
class EmptyCell;
class ClueCell;
class LetterCell;
class SolutionLetterCell;
class ImageCell;
typedef QList<KrossWordCell*> KrossWordCellList;
typedef QList<EmptyCell*> EmptyCellList;
typedef QList<ClueCell*> ClueCellList;
typedef QList<LetterCell*> LetterCellList;
typedef QList<SolutionLetterCell*> SolutionLetterCellList;
typedef QList<ImageCell*> ImageCellList;

Offset operator *(const Offset &offset, int factor);
Coord operator +=(Coord & coord1, const Coord & coord2);

const QString ALLOWED_CHARACTERS = QLatin1String("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

class KrossWordCell : public QGraphicsObject
{
    friend class KrossWord;
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    /** Different cell types for crosswords. */
    enum CellType {
        EmptyCellType = 0x001,
        ClueCellType = 0x002,
        DoubleClueCellType = 0x004,
        LetterCellType = 0x008,
        SolutionLetterCellType = 0x010,
        ImageCellType = 0x020,

        AllCellTypes = EmptyCellType | ClueCellType | DoubleClueCellType
                       | LetterCellType | SolutionLetterCellType | ImageCellType,
        /**< All cell types, but not user defined cell types. */

        InteractiveCellTypes = ClueCellType | DoubleClueCellType
                               | LetterCellType | SolutionLetterCellType,
        /**< All cell types except empty cells, but not user defined cell types. */

        UserType = 0x100 /**< For user defined cell classes derived from KrossWordCell.
  To make use of this in the flags class CellTypes, only use values computed
  like this: value=2^x with x>=4. */
    };
    Q_DECLARE_FLAGS(CellTypes, CellType);


    KrossWordCell(KrossWord *krossWord, CellType cellType, const Coord &coord);
    virtual ~KrossWordCell();

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 1 };
    virtual int type() const {
        return Type;
    };

    CellType cellType() const {
        return m_cellType;
    };
    virtual bool isLetterCell() const {
        return false;
    };
    Coord coord() const {
        return m_coord;
    };
    KrossWord *krossWord() const {
        return m_krossWord;
    };

    void clearCache() {
        m_cache = NULL;
    };

    static QString displayStringFromCellType(CellType cellType);

protected:
    void setPositionFromCoordinates();

    // Overloaded methods
    virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter,
                       const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

    // Virtual methods
    virtual void drawBackgroundForPrinting(QPainter*, const QStyleOptionGraphicsItem*) { };
    virtual void drawForegroundForPrinting(QPainter*, const QStyleOptionGraphicsItem*) { };

private:
    CellType m_cellType;
    Coord m_coord;
    KrossWord *m_krossWord;
    QPixmap *m_cache;
};


class EmptyCell : public KrossWordCell
{
    Q_OBJECT

public:
    EmptyCell(KrossWord *krossWord, Coord coord/*, QGraphicsScene *scene*/);

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 2 };
    virtual int type() const {
        return Type;
    };

protected:
    virtual void drawBackgroundForPrinting(QPainter *p, const QStyleOptionGraphicsItem *option);
};



// Sorting functions
bool lessThanCellType(const KrossWordCell *cell1, const KrossWordCell *cell2);
bool greaterThanCellType(const KrossWordCell *cell1, const KrossWordCell *cell2);


// Serialization
// QDataStream &operator<< ( QDataStream &s, const KrossWordCell *cell );
// QDataStream &operator<< ( QDataStream &s, const LetterCell *cell );
// QDataStream &operator<< ( QDataStream &s, const ClueCell *cell );
//
// QDataStream &operator>> ( QDataStream &s, ClueCell cell );


#endif // KROSSWORDCELL_H
