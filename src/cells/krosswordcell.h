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


#include "global.h"

#include <QString>
// #include <qstyleoption.h>
#include <qnamespace.h>

#include <KDebug>
#include <kdeversion.h>

#if QT_VERSION >= 0x040600
#include "animator.h"
#include <QGraphicsObject>
#include <QGraphicsEffect>
class QPropertyAnimation;
#else
#include <QGraphicsItem>
#endif

class QPainter;

namespace Crossword
{

class KrossWord;
class KrossWordCell;
class EmptyCell;
class ClueCell;
class DoubleClueCell;
class LetterCell;
class SolutionLetterCell;
class ImageCell;

#if QT_VERSION >= 0x040600
/** An effect that provides a glow effect, based on QGraphcisDropShadowEffect. */
class GlowEffect : public QGraphicsDropShadowEffect
{
public:
    GlowEffect( QObject* parent = 0 ) : QGraphicsDropShadowEffect( parent ) {};

protected:
    virtual void draw( QPainter* painter );
};
#endif

/** Base class for all crossword cells.
  * @see EmptyCell
  * @see LetterCell
  * @see SolutionLetterCell
  * @see ClueCell
  * @see DoubleClueCell
  * @see SpannedCell
  * @see ImageCell */
#if QT_VERSION >= 0x040600
class KrossWordCell : public QGraphicsObject
{
#else
class KrossWordCell : public QObject, public QGraphicsItem
{
#endif
    friend class KrossWord;
    friend class DoubleClueCell; // To be able to set m_coord of the child clue cells in the constructor
    Q_OBJECT
#if QT_VERSION >= 0x040600
    Q_INTERFACES( QGraphicsItem )
    Q_PROPERTY( qreal scaleX READ scaleX WRITE setScaleX )
#endif

public:
    KrossWordCell( KrossWord *krossWord, CellType cellType,
                   const Coord &coord );
    virtual ~KrossWordCell();

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 1 };
    virtual int type() const {
        return Type;
    };

    /** The cell type of this cell. @see CellType. */
    CellType cellType() const {
        return m_cellType;
    };

    bool isType( CellType cellType ) const {
        return m_cellType == cellType;
    };
    virtual bool isLetterCell() const {
        return false;
    };

    /** Returns the KrossWord object, to which this cell belongs. */
    inline KrossWord *krossWord() const {
        return m_krossWord;
    };

    /** The coordinates of this cell in the crossword grid. */
    Coord coord() const {
        return m_coord;
    };

    /** Returns a copy of the current cache pixmap. */
    QPixmap pixmap() const {
        return *m_cache;
    };

    qreal scaleX() const;
    void setScaleX( qreal scaleX );

    /** Returns a list with all synchronization categories. */
    QList< SyncCategory > allSynchronizationCategories() const {
        return QList< SyncCategory >() << OtherSynchronization
               << SolutionLetterSynchronization
               << SameCharacterLetterSynchronization;
    };

    const QHash< SyncCategory, QHash< KrossWordCell*, SyncMethods > >&
    synchronizationByCategory() const {
        return m_synchronizedCells;
    };
    QString syncInfoString() const;

    /** Checks if this cell is synced by one of @p syncMethods with
    * @p cell in one of @p syncCategories.
    *  @returns True, if at least one of @p syncMethods is synced in
    * one of @p syncCategories.
    * @returns False, if none of @p syncMethods are synced in any of
    * @p syncCategories. */
    bool isSynchronizedWith( KrossWordCell *cell,
                             SyncMethods syncMethods = SyncAll,
                             SyncCategories syncCategories = AllSyncCategories );

    /** Checks if this cell is synced by one of @p syncMethods with
    * @p cell in @p syncCategory.
    *  @returns True, if at least one of @p syncMethods is synced in
    * category @p syncCategory.
    * @returns False, if none of @p syncMethods are synced in category
    * @p syncCategory. */
    bool isSynchronizedWith( KrossWordCell *cell, SyncCategory syncCategory,
                             SyncMethods syncMethods = SyncAll );

    void synchronizeWith( KrossWordCell *cell,
                          SyncMethods syncMethods = SyncAll,
                          SyncCategory syncCategory = OtherSynchronization );
    void synchronizeWith( const KrossWordCellList &cellList,
                          SyncMethods syncMethods = SyncAll,
                          SyncCategory syncCategory = OtherSynchronization );
    bool removeSynchronizationWith( KrossWordCell *cell,
                                    SyncMethods syncMethods = SyncAll,
                                    SyncCategories syncCategories = AllSyncCategories );
    void removeSynchronization( SyncMethods syncMethods = SyncAll,
                                SyncCategories syncCategories = AllSyncCategories );

    /** Sets the highlight of this cell. */
    virtual void setHighlight( bool enable = true );
    /** Whether or not this cell is currently highlighted. */
    bool isHighlighted() const;

    virtual QRectF boundingRect() const;

    /** Clears the cache pixmap of this cell.
      * @param durationFactor The factor for the duration of a transition
      *  animation. Only used if transition animations are enabled. */
#if QT_VERSION >= 0x040600
    void clearCache( Animator::Duration duration = Animator::DefaultDuration );
#else
    void clearCache();
#endif

signals:
    void gotFocus( KrossWordCell *cell );
    /** The cell has been moved to @p newCoord. */
    void cellMoved( const Coord &newCoord );
    void appearanceAboutToChange();

public slots:
    void setFocusSlot( KrossWordCell *cell );
    void deleteAndRemoveFromSceneLater();

#if QT_VERSION >= 0x040600
public slots:
//  void updateTransformOriginPoint();

protected slots:
    void blurAnimationInFinished();
    void blurAnimationOutFinished();
    void clearCacheAndUpdate() {
        clearCache( Crossword::Animator::Instant );
        update();
    };
#else
protected slots:
    void clearCacheAndUpdate() {
        clearCache();
        update();
    };
#endif

protected:
    virtual bool setPositionFromCoordinates( bool animate = true );
    void setCoord( Coord coord, bool updateInCrosswordGrid = true );

    // Overloaded methods
    virtual void mousePressEvent( QGraphicsSceneMouseEvent* event );
    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent* event );
    virtual void focusInEvent( QFocusEvent* event );
    virtual void focusOutEvent( QFocusEvent* event );
    virtual QVariant itemChange( GraphicsItemChange change, const QVariant& value );
    virtual void paint( QPainter* painter,
                        const QStyleOptionGraphicsItem* option,
                        QWidget* widget = 0 );

    // Virtual methods
    virtual void drawBackground( QPainter*, const QStyleOptionGraphicsItem* ) { };
    virtual void drawForeground( QPainter*, const QStyleOptionGraphicsItem* ) { };
    virtual void drawBackgroundForPrinting( QPainter*, const QStyleOptionGraphicsItem* ) { };
    virtual void drawForegroundForPrinting( QPainter*, const QStyleOptionGraphicsItem* ) { };

    KrossWord *m_krossWord;

#if QT_VERSION >= 0x040600
    bool m_blockCacheClearing;
#endif

private:
    Coord m_coord;
    QHash< SyncCategory, QHash< KrossWordCell*, SyncMethods > > m_synchronizedCells;
    CellType m_cellType;
    bool m_highlight;
    QPixmap *m_cache;
    bool m_redraw;

#if QT_VERSION >= 0x040600
    QPropertyAnimation *m_blurAnim;
#endif
};


class EmptyCell : public KrossWordCell
{
    friend class KrossWord;
    Q_OBJECT

public:
    EmptyCell( KrossWord *krossWord, Coord coord );

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 2 };
    virtual int type() const {
        return Type;
    };

    LetterCell *toLetterCell( const QChar &correctContent = ' ' );

protected:
    virtual void focusInEvent( QFocusEvent* event );
    virtual void focusOutEvent( QFocusEvent* event );
    virtual void mousePressEvent( QGraphicsSceneMouseEvent* event );
    virtual void keyPressEvent( QKeyEvent* event );

    virtual void drawBackground( QPainter *p, const QStyleOptionGraphicsItem* option );
    virtual void drawBackgroundForPrinting( QPainter *p, const QStyleOptionGraphicsItem *option );
};



inline QDebug &operator <<( QDebug debug, CellType cellType ) {
    return debug << stringFromCellType( cellType );
};

inline QDebug &operator <<( QDebug debug, KrossWordCell *cell ) {
    if ( !cell )
        return debug << "NULL ";

    debug << cell->cellType() << "at" << cell->coord();
    return debug.space();
};

// Sorting functions
bool lessThanCellType( const KrossWordCell *cell1, const KrossWordCell *cell2 );
bool greaterThanCellType( const KrossWordCell *cell1, const KrossWordCell *cell2 );


// Serialization
// QDataStream &operator<< ( QDataStream &s, const KrossWordCell *cell );
// QDataStream &operator<< ( QDataStream &s, const LetterCell *cell );
// QDataStream &operator<< ( QDataStream &s, const ClueCell *cell );
//
// QDataStream &operator>> ( QDataStream &s, ClueCell cell );

}; // namespace Crossword

#endif // KROSSWORDCELL_H
