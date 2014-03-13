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

#include "cluecell.h"

#include "krossword.h"
#include "krosswordrenderer.h"
#include "krosswordtheme.h"

#include <QGraphicsSceneMouseEvent>
#include <QFocusEvent>
#include <QStyleOption>
#include <QGraphicsScene>
#include <QTextLayout>
#include <QPainter>
#include <qmath.h>

#include <kglobalsettings.h>
#include <kdeversion.h>

#if QT_VERSION >= 0x040600
#include "animator.h"
#include <QPropertyAnimation>
#endif

namespace Crossword
{

DoubleClueCell::DoubleClueCell( KrossWord* krossWord, const Coord& coord,
                                ClueCell* clue1, ClueCell* clue2 )
        : KrossWordCell( krossWord, DoubleClueCellType,
                         coord )
{
    Q_ASSERT( clue1 );
    Q_ASSERT( clue2 );

    m_clue1 = clue1;
    m_clue2 = clue2;

//   if ( clue1->scene() )
//     clue1->scene()->removeItem( clue1 );
//   if ( clue2->scene() )
//     clue2->scene()->removeItem( clue2 );


    clue1->prepareGeometryChange();
    clue1->setParentItem( this );
    clue1->show();
    clue1->setOpacity( 1 );
    clue1->setCoord( coord, false );

    clue2->prepareGeometryChange();
    clue2->setParentItem( this );
    clue2->show();
    clue2->setOpacity( 1 );
    clue2->setCoord( coord, false );

    // Compare the answer offsets to find the clue cell
    // that should be drawn above the other
    bool firstClueAboveSecond;
    Coord coord1 = coord + clue1->firstLetterOffset();
    Coord coord2 = coord + clue2->firstLetterOffset();
    if ( coord1.second < coord2.second )
        firstClueAboveSecond = true;
    else if ( coord1.second > coord2.second )
        firstClueAboveSecond = false;
    else if ( clue2->answerOffset() == OffsetBottom )
        firstClueAboveSecond = true;
    else if ( clue1->answerOffset() == OffsetBottom )
        firstClueAboveSecond = false;
    else
        firstClueAboveSecond = clue1->isHorizontal();

    qreal quartHeight = krossWord->cellSize().height() / 4;
    QPointF ptAbove( 0, -quartHeight );
    QPointF ptBelow( 0,  quartHeight );
    if ( firstClueAboveSecond ) {
        clue1->setPos( ptAbove );
        clue2->setPos( ptBelow );
    } else {
        clue1->setPos( ptBelow );
        clue2->setPos( ptAbove );
    }

    setOpacity( 1 );

//   qDebug() << "    - " << coord << pos();
//   setPositionFromCoordinates( false );
//   qDebug() << "    - " << coord << pos();
//   qDebug() << "    - " << boundingRect();
}

ClueCell* DoubleClueCell::clue( AnswerOffset answerOffset,
                                Qt::Orientation orientation ) const
{
    if ( m_clue1->answerOffset() == answerOffset
            && m_clue1->orientation() == orientation )
        return m_clue1;
    else if ( m_clue2->answerOffset() == answerOffset
              && m_clue2->orientation() == orientation )
        return m_clue2;
    else
        return NULL;
}

void DoubleClueCell::removeClueCell( ClueCell* clueCell )
{
    ClueCell *otherClueCell = m_clue1 == clueCell ? m_clue2 : m_clue1;

    qreal quartHeight = krossWord()->cellSize().height() / 4;
    QPointF newPos = krossWord()->mapFromItem( this, otherClueCell->pos() )
                     + QPointF( 0, quartHeight );
    clueCell->setParentItem( krossWord() );
    otherClueCell->setParentItem( krossWord() );

    if ( clueCell->scene() )
        clueCell->scene()->removeItem( clueCell );
    clueCell->setFlag( ItemIsFocusable, false );
    clueCell->deleteLater();

    m_clue1 = m_clue2 = NULL;

    bool otherIsBelow = otherClueCell->y() > quartHeight * 1.1;
#if QT_VERSION >= 0x040600
    if ( otherIsBelow ) {
        QPointF targetPos = newPos + QPointF( 0, -quartHeight );
        krossWord()->animator()->animate( Crossword::Animator::AnimatePositionChange,
                                          otherClueCell, targetPos, Crossword::Animator::VeryFast );
    } else
        otherClueCell->setPos( newPos );

    QPropertyAnimation *sizeAnim = new QPropertyAnimation(
        otherClueCell, "transitionHeightFactor" );
    sizeAnim->setStartValue( 0.5 );
    sizeAnim->setEndValue( 1.0 );
    sizeAnim->setEasingCurve( QEasingCurve( QEasingCurve::InOutBack ) );
    krossWord()->animator()->startOrEnqueue( sizeAnim, Crossword::Animator::Slow );
#else
    if ( otherIsBelow )
        otherClueCell->setPos( newPos + QPointF( 0, -quartHeight ) );
    else
        otherClueCell->setPos( newPos );
#endif

    otherClueCell->m_transitionHeightFactor = 0.5;
    krossWord()->replaceCell( this, otherClueCell );
    otherClueCell->clearCache();
    otherClueCell->setVisible( true );
}

ClueCell* DoubleClueCell::takeClueCell( ClueCell* clueCell )
{
    ClueCell *otherClueCell = m_clue1 == clueCell ? m_clue2 : m_clue1;
    clueCell->setParentItem( krossWord() );
    otherClueCell->setParentItem( krossWord() );

    if ( clueCell->scene() )
        clueCell->scene()->removeItem( clueCell );

    m_clue1 = m_clue2 = NULL;
    krossWord()->replaceCell( this, otherClueCell );
    otherClueCell->clearCache();
    otherClueCell->setVisible( true );

    return clueCell;
}

QRectF ClueCell::boundingRect() const
{
    DoubleClueCell *doubleClueCell;
    if (( doubleClueCell = qgraphicsitem_cast<DoubleClueCell*>( parentItem() ) ) ) {
//  // If the parent is a DoubleClueCell
//  bool firstClueAboveSeconfd;
//  Coord coord1 = doubleClueCell->clue1()->coord() + doubleClueCell->clue1()->firstLetterOffset();
//  Coord coord2 = doubleClueCell->clue2()->coord() + doubleClueCell->clue2()->firstLetterOffset();
//  if ( coord1.second < coord2.second )
//      firstClueAboveSecond = true;
//  else if ( coord1.second > coord2.second )
//      firstClueAboveSecond = false;
//  else
//      firstClueAboveSecond = doubleClueCell->clue1()->isHorizontal();
//
//  bool isAbove = doubleClueCell->clue1() == this ? firstClueAboveSecond : !firstClueAboveSecond;
        qreal halfWidth = krossWord()->cellSize().width() / 2;
        qreal halfHeight = krossWord()->cellSize().height() / 2;
//  qDebug() << "BOUNDING RECT IN 2CLUE"
//    << QRectF( -halfWidth - 0.5, -halfHeight / 2 - 0.5,
//       krossWord()->cellSize().width() + 1, halfHeight + 1 );
//  QRectF( 0, 0, krossWord()->cellSize().width(), halfHeight )
//    << pos() << isVisible() << coord();
//  qDebug() << "   2Clue-Cell's boundingRect:" << doubleClueCell->boundingRect();
        return QRectF( -halfWidth - 0.5, -halfHeight / 2,
                       krossWord()->cellSize().width() + 1, halfHeight + 1 );
//
// //  qDebug() << "For double clue cell with clue1 =" << doubleClueCell->clue1()->clue();
// //  qDebug() << "For double clue cell with clue2 =" << doubleClueCell->clue2()->clue();
// //  qDebug() << "Clue cell" << clue() << "| isAbove?" << isAbove;
//  if ( isAbove )
//      return QRectF( x(), y(), krossWord()->cellSize().width(), halfHeight );
//  else
//      return QRectF( x(), y() + halfHeight, krossWord()->cellSize().width(), halfHeight );
    } else if ( m_transitionHeightFactor != 1 ) {
        QRectF rect = KrossWordCell::boundingRect();
        rect.setHeight( rect.height() * m_transitionHeightFactor );
        return rect;
    } else
        return KrossWordCell::boundingRect();
}

QRectF ClueCell::boundingRectIncludingAnswerCells() const
{
    QRectF rect = boundingRect();

    LetterCellList letterList = letters();
    foreach( LetterCell *letter, letterList )
    rect = rect.united( QRectF( letter->x(), letter->y(),
                                letter->boundingRect().width(), letter->boundingRect().height() ) );

    return rect;
}

ClueCell::ClueCell( KrossWord* krossWord, Coord coord,
                    Qt::Orientation orientation, AnswerOffset answerOffset,
                    QString clue, QString answer )
        : KrossWordCell( krossWord, ClueCellType, coord )
{
    m_orientation = orientation;
    m_answerOffset = answerOffset;
    m_clue = clue;
    wrapClueText();
    m_clueNumber = -1;
    m_correctAnswer = answer.toUpper();
    m_textLayoutRect.setRect( 0, 0, 0, 0 );
    m_transitionHeightFactor = 1;
    if ( answerOffset == OnClueCell )
        setVisible( false );

    setCursor( Qt::ArrowCursor );
    setToolTip( clueWithoutHyphens() );

    connect( this, SIGNAL( currentAnswerChanged( ClueCell*, const QString& ) ),
             krossWord, SLOT( answerChangedSlot( ClueCell*, const QString& ) ) );
}

void ClueCell::setClue( const QString &clue )
{
    if ( m_clue == clue )
        return;

    m_clue = clue;
    wrapClueText();
    m_textLayoutRect = QRect( 0, 0, 0, 0 ); // Causes recreation of the text layout on paint
    clearCache();
    update();

    emit clueTextChanged( this, clue );
}

void ClueCell::drawBackground( QPainter *p, const QStyleOptionGraphicsItem *option )
{
    if ( isHighlighted() )
        KrosswordRenderer::self()->renderElement( p, "question_cell_highlight", option->rect );
    else
        KrosswordRenderer::self()->renderElement( p, "question_cell", option->rect );
}

void ClueCell::drawBackgroundForPrinting( QPainter *p, const QStyleOptionGraphicsItem *option )
{
    QPen pen( Qt::black );
    p->setPen( pen );
    p->drawRect( option->rect );
}

// void ClueCell::setFocus( Qt::FocusReason focusReason ) {
//     if ( answerOffset() == OnClueCell )
//  firstLetter()->setFocus();
//     else
//  QGraphicsItem::setFocus( focusReason );
// }

void ClueCell::focusInEvent( QFocusEvent* event )
{
    firstLetter()->setFocus();
    setHighlight();
    event->accept();

    KrossWordCell::focusInEvent( event );
}

void ClueCell::focusOutEvent( QFocusEvent* event )
{
    // Only remove highlight if the scene still has focus
//     if ( scene() && scene()->hasFocus() )
//  krossWord()->setHighlightedClue( NULL );
    KrossWordCell::focusOutEvent( event );
}

void ClueCell::mousePressEvent( QGraphicsSceneMouseEvent* event )
{
//     if ( !krossWord()->isInteractive() ) {
//  event->ignore();
//  return;
//     }
    /*
        firstLetter()->setFocus();
        setHighlight();
        event->accept();*/

    KrossWordCell::mousePressEvent( event );
}

Offset ClueCell::answerOffsetToOffset( AnswerOffset answerOffset )
{
    switch ( answerOffset ) {
    case OnClueCell:
        return Offset( 0, 0 );
    case OffsetTop:
        return Offset( 0, -1 );
    case OffsetBottom:
        return Offset( 0, 1 );
    case OffsetLeft:
        return Offset( -1, 0 );
    case OffsetRight:
        return Offset( 1, 0 );
    case OffsetTopLeft:
        return Offset( -1, -1 );
    case OffsetTopRight:
        return Offset( 1, -1 );
    case OffsetBottomLeft:
        return Offset( -1, 1 );
    case OffsetBottomRight:
        return Offset( 1, 1 );
    case OffsetInvalid:
        kDebug() << "Invalid answerOffset value.";
        return Offset( 0, 0 );
    }

    qDebug() << "Unknown value of AnswerOffset:" << answerOffset;
    Q_ASSERT( false ); // Error
    return Offset( 0, 0 );
}

AnswerOffset ClueCell::offsetToAnswerOffset( Offset offset )
{
    if ( offset == Offset( 0, 0 ) )
        return OnClueCell;
    if ( offset == Offset( 0, -1 ) )
        return OffsetTop;
    if ( offset == Offset( 0, 1 ) )
        return OffsetBottom;
    if ( offset == Offset( -1, 0 ) )
        return OffsetLeft;
    if ( offset == Offset( 1, 0 ) )
        return OffsetRight;
    if ( offset == Offset( -1, -1 ) )
        return OffsetTopLeft;
    if ( offset == Offset( 1, -1 ) )
        return OffsetTopRight;
    if ( offset == Offset( -1, 1 ) )
        return OffsetBottomLeft;
    if ( offset == Offset( 1, 1 ) )
        return OffsetBottomRight;
    else {
        kDebug() << "Invalid offset.";
        return OffsetInvalid;
    }
}

void ClueCell::setHidden()
{
    if ( m_answerOffset == OnClueCell )
        return;

//   kDebug() << "Set hidden" << clue() << coord();
    DoubleClueCell *doubleClueCell = qgraphicsitem_cast< DoubleClueCell* >( parentItem() );
    if ( doubleClueCell ) {
        ClueCell *otherClueCell = doubleClueCell->clue1() == this
                                  ? doubleClueCell->clue2() : doubleClueCell->clue1();
        doubleClueCell->clue1()->setParentItem( krossWord() );
        doubleClueCell->clue2()->setParentItem( krossWord() );
        krossWord()->replaceCell( coord(), otherClueCell );
    } else
        krossWord()->removeCell( coord(), false ); // Remove clue cell, but don't delete it

    setCoord( firstLetterCoords(), false );
    //     setCoord( firstLetterCoords() );
    m_answerOffset = OnClueCell;
    setVisible( false );
}

QList< Coord > ClueCell::answerCoordList( const Coord& clueCoord,
        AnswerOffset answerOffset,
        Qt::Orientation orientation,
        int answerLength )
{
    QList< Coord > ret;
    Coord coord = firstLetterCoords( clueCoord, answerOffset );
    Coord letterOffset = orientation == Qt::Horizontal ? Coord( 1, 0 ) : Coord( 0, 1 );
    do {
        ret << coord;
        coord += letterOffset;
    } while ( --answerLength > 0 );

    return ret;
}

ErrorType ClueCell::setProperties( Qt::Orientation newOrientation,
                                   AnswerOffset newAnswerOffset,
                                   const QString &newCorrectAnswer )
{
    return krossWord()->changeClueProperties( this, newOrientation,
            newAnswerOffset, newCorrectAnswer );
}

void ClueCell::setProperties( Qt::Orientation newOrientation,
                              AnswerOffset newAnswerOffset )
{
    bool changed = false;
    if ( newAnswerOffset != m_answerOffset ) {
        if ( newAnswerOffset != OnClueCell
                && m_answerOffset == OnClueCell ) {
            setVisible( true );
#if QT_VERSION >= 0x040600
            if ( krossWord()->isAnimationTypeEnabled( AnimateAppear ) )
                krossWord()->animator()->animate( Animator::AnimateFadeIn, this );
#endif
        }
        m_answerOffset = newAnswerOffset;

        changed = true;
        emit answerOffsetChanged( this, newAnswerOffset );
    }

    if ( newOrientation != m_orientation ) {
        m_orientation = newOrientation;

        changed = true;
        emit orientationChanged( this, newOrientation );
    }

    if ( changed ) {
        LetterCell *first = firstLetter();
        if ( first ) {
            first->clearCache();
            first->update();
        }
    }
}

void ClueCell::setAnswerOffset( AnswerOffset newAnswerOffset )
{
    setProperties( m_orientation, newAnswerOffset );
}

void ClueCell::setOrientation( Qt::Orientation newOrientation )
{
    setProperties( newOrientation, m_answerOffset );
}

// KrossWord::ErrorType ClueCell::setAnswerOffset(
//      ClueCell::AnswerOffset newAnswerOffset ) {
//   Offset offset = answerOffsetToOffset( newAnswerOffset );
//   KrossWord::ErrorType errorType = krossWord()->canInsertClue( coord(),
//       orientation(), offset, correctAnswer(), KrossWord::DontIgnoreErrors,
//       true, this );
//   if ( errorType != KrossWord::NoError )
//     return errorType;
//
//   Coord newFirstLetterCellCoord = coord() + offset;
//   KrossWordCell *cell = krossWord()->at( newFirstLetterCellCoord );
//   ClueCell *clue;
//   if ( (clue = qgraphicsitem_cast<ClueCell*>(cell)) ) {
//     DoubleClueCell *doubleClueCell = new DoubleClueCell( krossWord(),
//       newClueCellCoord, clue, this );
//     krossWord()->replaceCell( newClueCellCoord, doubleClueCell, false );
//   } else if ( cell->isType(KrossWordCell::EmptyCellType) ) {
//     krossWord()->replaceCell( newClueCellCoord, this );
//   } else {
//     qDebug() << "Cannot unhide clue cell to the coordinates, because there "
//       "already is a cell of type " << KrossWordCell::stringFromCellType(cell->cellType());
//     return false;
//   }
//
//   return KrossWord::NoError;
// }

bool ClueCell::setUnhidden( AnswerOffset newAnswerOffset )
{
    if ( m_answerOffset != OnClueCell )
        return true; // Clue cell is already visible

    Offset offset = answerOffsetToOffset( newAnswerOffset );
//   kDebug() << "Set unhidden" << clue() << coord() << "answer offset" << offset;

    Coord newClueCellCoord = coord() - offset;
    KrossWordCell *cell = krossWord()->at( newClueCellCoord );
    ClueCell *clue;
    if (( clue = qgraphicsitem_cast<ClueCell*>( cell ) ) ) {
//     kDebug() << "Creating a new double clue cell";
        DoubleClueCell *doubleClueCell = new DoubleClueCell( krossWord(),
                newClueCellCoord, clue, this );
        krossWord()->replaceCell( newClueCellCoord, doubleClueCell, false );
    } else if ( cell->isType( EmptyCellType ) ) {
        krossWord()->replaceCell( newClueCellCoord, this );
    } else {
        qDebug() << "Cannot unhide clue cell to the coordinates, because there "
        "already is a cell of type " << stringFromCellType( cell->cellType() );
        return false;
    }

    m_answerOffset = newAnswerOffset;
    setVisible( true );
    return true;
}

AnswerOffset ClueCell::tryToMakeVisible( bool simulate,
        QList<Coord> disallowedCoords )
{
    if ( m_answerOffset != OnClueCell )
        return m_answerOffset; // Clue cell is already visible

//   kDebug() << "Clue" << clue() << "at" << coord() << "with answer offset =" <<
//       answerOffsetToOffset(m_answerOffset) << "| First letter pos is" << firstLetter()->coord();

    QList< AnswerOffset > offsets;
    if ( m_orientation == Qt::Horizontal ) {
        offsets << OffsetRight << OffsetBottom << OffsetTop
        << OffsetBottomRight << OffsetTopRight
        << OffsetBottomLeft << OffsetTopLeft;
    } else { // Qt::Vertical
        offsets << OffsetBottom << OffsetRight << OffsetLeft
        << OffsetBottomRight << OffsetBottomLeft
        << OffsetTopRight << OffsetTopLeft;
    }

    AnswerOffset newAnswerOffset = OffsetInvalid;
    Coord newClueCellCoord;
    ClueCell *clueCell = NULL;
    foreach( const AnswerOffset &offset, offsets ) {
//     kDebug() << "Test offset" << ClueCell::answerOffsetToOffset( offset );
        Coord testNewClueCellCoord = coord() - ClueCell::answerOffsetToOffset( offset );
        if ( !krossWord()->inside( testNewClueCellCoord )
                || disallowedCoords.contains( testNewClueCellCoord ) ) {
//       qDebug() << "Outside of grid or" << testNewClueCellCoord << "is disallowed for" << clue();
            continue; // New clue cell coords aren't inside the crossword grid
        }

        KrossWordCell *cell = krossWord()->at( testNewClueCellCoord );
        CellType cellType = cell->cellType();
        if ( cellType == EmptyCellType ) {
            newAnswerOffset = offset;
            newClueCellCoord = testNewClueCellCoord;
            clueCell = NULL;
            break;
        } else if ( cellType == ClueCellType && !clueCell ) {
            newAnswerOffset = offset;
            newClueCellCoord = testNewClueCellCoord;
            clueCell = qgraphicsitem_cast< ClueCell* >( cell );
            // Don't break here to search for a better solution, that doesn't create
            // a new double clue cell. But set newAnswerOffset to use that offset
            // if no better solution can be found.
        }
    }

//   qDebug() << "newAnswerOffset =" << ClueCell::answerOffsetToOffset(newAnswerOffset);

    // No appropriate offset found to make the clue cell visible
    if ( newAnswerOffset == OffsetInvalid ) {
        qDebug() << "No appropriate offset found to make the clue cell visible" << clue();
        return OffsetInvalid;
    } else if ( simulate )
        return newAnswerOffset;

    if ( clueCell ) {
        DoubleClueCell *doubleClueCell = new DoubleClueCell( krossWord(),
                newClueCellCoord, clueCell, this );
        krossWord()->replaceCell( clueCell->coord(), doubleClueCell, false );
    } else {
        setCoord( newClueCellCoord );
    }
    m_answerOffset = newAnswerOffset;
    setVisible( true );

    return newAnswerOffset;
}

Coord ClueCell::firstLetterCoords() const
{
    return coord() + ClueCell::answerOffsetToOffset( m_answerOffset );
}

Coord ClueCell::firstLetterCoords( Coord clueCoords,
                                   AnswerOffset answerOffset )
{
    return clueCoords + ClueCell::answerOffsetToOffset( answerOffset );
}

int ClueCell::minAnswerLength() const
{
    return krossWord()->crosswordTypeInfo().minAnswerLength;
}

int ClueCell::maxAnswerLength() const
{
    return answerLength() + canAddLetters( 10000 );
}

int ClueCell::setAnswerLength( int newLength )
{
    addLetters( newLength - answerLength() );
    return answerLength();
}

int ClueCell::canAddLetters( int count ) const
{
    if ( count == 0 )
        return 0;

    int actualCount = 0;
    int sign = count > 0 ? 1 : -1;
    Offset offset = m_orientation == Qt::Horizontal
                    ? Offset( 1, 0 ) : Offset( 0, 1 );
    Coord coord = firstLetterCoords() + offset * ( m_correctAnswer.length() - 1 );

    offset = offset * sign;
    if ( sign > 0 )
        coord += offset;
    while ( count != 0 && krossWord()->inside( coord ) ) {
        KrossWordCell *cell = krossWord()->at( coord );
        LetterCell *letter;
        if ( sign > 0 ) {
            // Add letter cell to clue
            if (( letter = dynamic_cast<LetterCell*>( cell ) ) ) {
                if ( letter->hasClueInDirection( m_orientation ) ) {
//    qDebug() << "Can't add letters to this clue";
                    break;
                }
                actualCount += sign;
            } else if ( cell->isType( EmptyCellType ) ) {
                actualCount += sign;
            } else {
//  qDebug() << "Wrong cell type";
                break;
            }
        } else { // sign < 0
            // Remove letter cell from clue
            if ( m_correctAnswer.length() + actualCount + sign <
                    m_krossWord->crosswordTypeInfo().minAnswerLength ) {
//  qDebug() << "Can't remove letters because the minimum answer length "
//       "is reached";
                break; // Don't remove all letter cells
            }

            letter = dynamic_cast<LetterCell*>( cell );
            if ( !letter ) {
//  qDebug() << "No letter cell found to remove from" << this
//    << "at" << coord << "| found cell is" << cell;
                break;
            }

            actualCount += sign;
        }

        coord += offset;
        count -= sign;
    }

    return actualCount;
}

int ClueCell::addLetters( int count )
{
    if ( count == 0 )
        return 0;

#if QT_VERSION >= 0x040600
    krossWord()->animator()->beginEnqueueAnimations();
#endif

    LetterCell *oldLastLetter = lastLetter();
    bool oldLastLetterHasEndBar = oldLastLetter->needsEndBar( orientation() );

    int actualCount = 0;
    int sign = count > 0 ? 1 : -1;
    Offset offset = m_orientation == Qt::Horizontal
                    ? Offset( 1, 0 ) : Offset( 0, 1 );
    Coord coord = firstLetterCoords() + offset * ( m_correctAnswer.length() - 1 );

    offset = offset * sign;
    if ( sign > 0 )
        coord += offset;
    else {
        // When the current letter cell will get removed, select the last
        // not removed letter
        LetterCell *letter = dynamic_cast<LetterCell*>( krossWord()->currentCell() );
        if ( letter ) {
            int pos = posOfLetter( letter );
            if ( pos >= answerLength() + count ) { // count < 0
                letter = letterAt( answerLength() + count - 1 );
                krossWord()->setCurrentCell( letter );
                letter->setFocus();
            }
        }
    }

    while ( count != 0 && krossWord()->inside( coord ) ) {
        KrossWordCell *cell = krossWord()->at( coord );
        LetterCell *letter;
        if ( sign > 0 ) {
            // Add letter cell to clue
            if (( letter = dynamic_cast<LetterCell*>( cell ) ) ) {
                if ( letter->hasClueInDirection( m_orientation ) ) {
//    kDebug() << "Can't add letters to this clue";
                    break;
                }

                m_correctAnswer += letter->correctLetter();
                letter->attachClue( this );

                actualCount += sign;
            } else if ( cell->isType( EmptyCellType ) ) {
                m_correctAnswer += ClueCell::EmptyCorrectCharacter;
                letter = new LetterCell( m_krossWord, coord, this );
                krossWord()->replaceCell( coord, letter );

                actualCount += sign;
            } else
                break; // Wrong cell type

            if ( isHighlighted() )
                letter->setHighlight();
        } else { // sign < 0
            // Remove letter cell from clue
            if ( m_correctAnswer.length() < m_krossWord->crosswordTypeInfo().minAnswerLength )
                break; // Don't remove letter cells when the minimum answer length is reached

            letter = dynamic_cast<LetterCell*>( cell );
            if ( !letter )
                break; // No letter cell found to remove

            if ( !letter->isCrossed() ) {
                // Remove letter
                if ( letter == oldLastLetter )
                    oldLastLetter = NULL;
                krossWord()->removeCell( coord );
            }
            letter->detachClue( this );

            m_correctAnswer = m_correctAnswer.left( m_correctAnswer.length() - 1 );
            actualCount += sign;
        }

        coord += offset;
        count -= sign;
    } // while

    if ( actualCount != 0 ) {
        emit answerLengthChanged( this, m_correctAnswer.length() );

        // Update rendering of end bars
        if ( oldLastLetter && oldLastLetterHasEndBar ) {
            oldLastLetter->clearCache();
            oldLastLetter->update();
        }

        LetterCell *newLastLetter = lastLetter();
        if ( newLastLetter->needsEndBar( orientation() ) ) {
            newLastLetter->clearCache();
            newLastLetter->update();
        }

        if ( actualCount < 0 ) {
            // if actualCount > 0 the signal is emitted by letterAdded()
            if ( oldLastLetter != newLastLetter )
                emit lastLetterChanged( newLastLetter );
        }
    }

#if QT_VERSION >= 0x040600
    krossWord()->animator()->endEnqueueAnimations();
#endif
    return actualCount;
}

void ClueCell::beginAddLetters()
{
    m_dontProcessLetterAdded = true;
}

void ClueCell::endAddLetters()
{
    m_dontProcessLetterAdded = false;

    LetterCell *oldLastLetter = lastLetter();
    findLetters();

    LetterCell *newLastLetter = lastLetter();
    if ( oldLastLetter != newLastLetter )
        emit lastLetterChanged( newLastLetter );
}

void ClueCell::endAddLetters( Qt::Orientation newOrientation,
                              AnswerOffset newAnswerOffset )
{
    m_dontProcessLetterAdded = false;

    LetterCell *oldLastLetter = lastLetter();
    findLetters( newOrientation, newAnswerOffset );
    setProperties( newOrientation, newAnswerOffset );

    LetterCell *newLastLetter = lastLetter();
    if ( oldLastLetter != newLastLetter )
        emit lastLetterChanged( newLastLetter );
}

void ClueCell::letterAdded( LetterCell* letter )
{
    Q_UNUSED( letter );
    if ( m_dontProcessLetterAdded )
        return;

    LetterCell *oldLastLetter = lastLetter();
    findLetters( letter );

    LetterCell *newLastLetter = lastLetter();
    if ( oldLastLetter != newLastLetter )
        emit lastLetterChanged( newLastLetter );
}

void ClueCell::letterRemoved( LetterCell* letter )
{
    if ( lastLetter() == letter ) {
        m_letters.takeLast();
        emit lastLetterChanged( lastLetter() );
    }
//   else
//     kDebug() << "Letter removed that isn't the last one, pos =" << posOfLetter( letter );
}

void ClueCell::findLetters( LetterCell *newLetter )
{
    findLetters( orientation(), answerOffset(), newLetter );
}

void ClueCell::findLetters( Qt::Orientation newOrientation,
                            AnswerOffset newAnswerOffset,
                            LetterCell *newLetter )
{
    m_letters.clear();

    const Offset letterOffset = KrosswordGrid::letterOffset( newOrientation );
    Coord letterPos = firstLetterCoords( coord(), newAnswerOffset );

    for ( int i = 0; i < m_correctAnswer.length(); ++i ) {
        KrossWordCell *cell = krossWord()->at( letterPos );
        if ( cell->isLetterCell() )
            m_letters << static_cast< LetterCell* >( cell );
        else if ( newLetter && newLetter->coord() == letterPos )
            m_letters << newLetter;
        else
            kDebug() << "No letter cell at" << letterPos << i;

        letterPos += letterOffset;
    }
}

QVariant ClueCell::itemChange( QGraphicsItem::GraphicsItemChange change,
                               const QVariant& value )
{
//     if ( change == ItemSelectedHasChanged ) {
//  bool selected = value.toBool();
//  LetterCellList letterList = letters();
//  foreach( LetterCell *letterCell, letterList ) {
//      letterCell->setSelected( selected );
//  }
//     }

    return KrossWordCell::itemChange( change, value );
}


QString ClueCell::clueWithoutHyphens() const
{
    if ( m_clue.isEmpty() )
        return i18nc( "Display text for empty clue texts", "<placeholder>empty</placeholder>" );

    QString clueText = m_clue;
    //     QRegExp rx( "[a-z?äöü]-[a-z?äöü]" );
    QRegExp rx( "\\w-\\w" ); // TODO: TEST
    int pos = 0;
    while (( pos = rx.indexIn( clueText, pos ) ) != -1 ) {
        clueText.remove( ++pos, 1 ); // Remove the hyphen
    }

    return clueText;
}

QString ClueCell::clueWithNumber( QString format ) const
{
    if ( krossWord()->crosswordTypeInfo().clueMapping == CluesReferToSetsOfCells
            && clueNumber() != -1 ) {
        return QString( format ).arg( clueNumber() + 1 ).arg( clueWithoutHyphens() );
    } else
        return clueWithoutHyphens();
}

void ClueCell::wrapClueText()
{
    QString clueText = m_clue;
    clueText.replace( QRegExp( "\\b-\\b", Qt::CaseInsensitive ), "-\n" );

    if ( clueText.contains( '\n' ) ) {
        QStringList lines = clueText.split( '\n' );
        while ( lines.count() < 4 ) {
            // Get longest line:
            int longestLength = 0;
            int iLongest = 0;
            for ( int i = 0; i < lines.count(); ++i ) {
                if ( lines[i].length() > longestLength ) {
                    longestLength = lines[ i ].length();
                    iLongest = i;
                }
            }

            // Break longest line at the middle most space character
            QString longestLine = lines[ iLongest ];
            if ( longestLine.contains( ' ' ) ) {
                int iMiddle = longestLine.length() / 2;
                int iMiddleSpace = -1;
                for ( int i = 0; i < iMiddle - 1; ++i ) {
                    if ( longestLine[iMiddle + i] == ' ' ) {
                        iMiddleSpace = iMiddle + i;
                        break;
                    } else if ( longestLine[iMiddle - i] == ' ' ) {
                        iMiddleSpace = iMiddle - i;
                        break;
                    }
                }
                if ( iMiddleSpace == -1 )
                    break; // Would break a single character to a new line

                lines[ iLongest ].replace( iMiddleSpace, 1, '\n' );
            } else
                break; // Longest line has no space character

            clueText = lines.join( "\n" );
            lines = clueText.split( '\n' );
        }
    }

    m_wrappedClue = clueText;
}

void ClueCell::createLayout( const QRect& rect )
{
    QString clueText = m_wrappedClue;
    clueText.replace( "-\n", "-" );
    clueText.replace( '\n', ' ' );

//   qDebug() << "";
//   qDebug() << "Layout for" << clueText;

    int lineWidth = rect.width();
    int maxHeight = rect.height();
    int maxLines;
    int maxFontSizeQuotient;
    int fontSizeQuotient = 3;
    QFont font = KGlobalSettings::smallestReadableFont();
    font.setPixelSize( maxHeight / fontSizeQuotient + 1 );
    QFontMetrics fontMetrics( font );

    if ( isInDoubleClueCell() ) {
        if ( rect.height() < 30 ) {
            maxLines = 2;
            maxFontSizeQuotient = 3;
        } else if ( rect.height() < 60 ) {
            maxLines = 3;
            maxFontSizeQuotient = 4;
        } else {
            maxLines = 4;
            maxFontSizeQuotient = 5;
        }
    } else {
        if ( rect.height() < 80 ) {
            maxLines = 4;
            maxFontSizeQuotient = 5;
        } else if ( rect.height() < 100 ) {
            maxLines = 5;
            maxFontSizeQuotient = 6;
        } else {
            maxLines = 6;
            maxFontSizeQuotient = 7;
        }
    }

    // Get the longest unbreakable "word" (can be only part of a word,
    // eg. for "Aaaaaa-aaa", Aaaaaa is the longest "word")
    int longestWordWidth = 0;
    QString longestWord;
    QStringList words = clueText.split( QRegExp( "(\\s|\\-)" ) );
    // TODO: Only split when a hyphen is between two lowercase characters
    foreach( const QString &word, words ) {
        int wordWidth = fontMetrics.width( word );
        if ( wordWidth > longestWordWidth ) {
            longestWordWidth = wordWidth;
            longestWord = word + '-';
        }
    }

    // Get font size
    while (( fontMetrics.width( clueText ) > lineWidth *
             qFloor( maxHeight / fontMetrics.lineSpacing() )
             || fontMetrics.width( longestWord ) > lineWidth )
            && fontSizeQuotient < maxFontSizeQuotient ) {
        ++fontSizeQuotient;
        font.setPixelSize( maxHeight / fontSizeQuotient + 1 );
        fontMetrics = QFontMetrics( font );
    }

    // Do layout
    int leading = fontMetrics.leading();
    qreal height = 0;
    qreal widthUsed = 0;
    QTextLine line;
    bool textChanged;
    m_textLayout.setText( clueText );
    m_textLayout.setFont( font );
    do {
        textChanged = false;
        m_textLayout.beginLayout();
        int lineNr = 0;
        while (( line = m_textLayout.createLine() ).isValid() ) {
            line.setLineWidth( lineWidth );
            if ( lineNr > 0 )
                height += leading;
            line.setPosition( QPointF( 0, height ) );
            height += line.height();
            widthUsed = qMax( widthUsed, line.naturalTextWidth() );

            // Elide Text that it too wide
            QString lineText = clueText.mid( line.textStart(), line.textLength() );
            if ( fontMetrics.width( lineText ) > lineWidth + 10 ) {
                lineText = fontMetrics.elidedText( lineText, Qt::ElideRight, lineWidth );
                clueText.replace( line.textStart(), line.textLength(), lineText );
                textChanged = true;
                break;
            }

            // Remove line-breaking hyphens in one line
            QRegExp rx( "\\w-\\w" );
            int pos = 0;
            while (( pos = rx.indexIn( lineText, pos ) ) != -1 ) {
                clueText.remove( ++pos + line.textStart(), 1 ); // Remove the hyphen
                textChanged = true;
            }
            if ( textChanged )
                break;

            // Elide last line if the maximal line count is reached and there is
            // more text to layout
            if ( ++lineNr == maxLines
                    && ( line.textStart() + line.textLength() ) < clueText.length() ) {
                // Add horizontal ellipsis (...) and then call elidedText() to make
                // sure that the new line will be no longer than [lineWidth]
                lineText += QChar( 0x2026 );
                lineText = fontMetrics.elidedText( lineText, Qt::ElideRight, lineWidth );

                // Remove remaining text that doesn't fit into the cell
                clueText = clueText.left( line.textStart() ) + lineText;
                textChanged = true;
                break;
            }
        }
        m_textLayout.endLayout();

        // Redo layout if the text has changed
        if ( textChanged ) {
            height = 0;
            widthUsed = 0;
            if ( m_textLayout.text() == clueText )
                break;
            m_textLayout.setText( clueText );
        }
    } while ( textChanged );

    // Lower the line spacing if the current layout is too height
    if ( height > maxHeight && m_textLayout.lineCount() > 1 ) {
        qreal moveUpPerLine = 0.95f * ( height - maxHeight ) /
                              ( m_textLayout.lineCount() - 1 );
        for ( int i = 1; i < m_textLayout.lineCount(); ++i ) {
            QTextLine line = m_textLayout.lineAt( i );
            line.setPosition( QPointF( 0, line.y() - moveUpPerLine * i ) );
        }

        height = maxHeight;
    }

    m_textLayoutRect = rect;
}

void ClueCell::drawForeground( QPainter *p, const QStyleOptionGraphicsItem *option )
{
    QRect drawRect = KrosswordTheme::trimmedRect( option->rect,
                     krossWord()->theme()->marginsClueCell() );

    if ( drawRect != m_textLayoutRect )
        createLayout( drawRect );

    m_textLayout.draw( p, QPointF( drawRect.left(),
                                   drawRect.top() + ( drawRect.height() -
                                                      m_textLayout.boundingRect().height() ) / 2.0f ) );

    drawClueNumber( p , option );
}

void ClueCell::drawForegroundForPrinting( QPainter *p, const QStyleOptionGraphicsItem *option )
{
    ClueCell::drawForeground( p, option );
}

void ClueCell::drawClueNumber( QPainter *p, const QStyleOptionGraphicsItem *option )
{
    // Draw clue number if any
    if ( m_answerOffset == OnClueCell && m_clueNumber != -1 ) {
        QString text = QString( "%1." ).arg( m_clueNumber + 1 );
        QFont font = KGlobalSettings::smallestReadableFont();
#if QT_VERSION >= 0x040600
        qreal levelOfDetail = QStyleOptionGraphicsItem::levelOfDetailFromTransform(
                                  QTransform( option->matrix ) );
#else
        qreal levelOfDetail = option->levelOfDetail;
#endif
        font.setPointSizeF( 7.0 * levelOfDetail );
        p->setFont( font );
        QFontMetrics fontMetrics( font );
        QRect rect = fontMetrics.boundingRect( text );
        rect.setWidth( fontMetrics.width( text ) );
        QRect trimmedRect = KrosswordTheme::trimmedRect( option->rect,
                            krossWord()->theme()->marginsClueCell( levelOfDetail ) );
        p->drawText( KrosswordTheme::rectAtPos( trimmedRect, rect,
                                                krossWord()->theme()->clueNumberPos() ), text );
//  QPoint topLeft = option->rect.bottomRight() - QPoint( rect.width(), rect.height() );
//  p->drawText( QRect( topLeft, rect.size()).adjusted(-3, -3, -3, -3), text );
    }
}

QString ClueCell::currentAnswer( const QChar &pad ) const
{
    QString currentAnswer;
    LetterCellList list = letters();
    foreach( LetterCell *cell, list ) {
        if ( cell->isEmpty() )
            currentAnswer += pad;
        else
            currentAnswer += cell->currentLetter();
    }

    return currentAnswer;
}

void ClueCell::setCurrentAnswer( const QString &answer, Confidence confidence )
{
//     Q_ASSERT( m_answer.length() == answer.length() );
//     QString newCurrentAnswer = answer;
//     if( m_answer.length() >= answer.length() );

    // To not emit currentAnswerChanged for each changed letter
    bool wasBlocked = blockSignals( true );

    int pos = 0, len = answer.length();
    LetterCellList list = letters();
    foreach( LetterCell *cell, list ) {
        if ( pos < len )
            cell->setCurrentLetter( answer[pos++], confidence );
        else
            cell->setCurrentLetter( ' ', confidence );
    }

    blockSignals( wasBlocked );
    emit currentAnswerChanged( this, currentAnswer() );
}

bool ClueCell::isAnswerComplete() const
{
    LetterCellList list = letters();
    foreach( LetterCell *cell, list ) {
        if ( cell->isEmpty() )
            return false;
    }

    return true;
}

bool ClueCell::isAnswerCorrect() const
{
    LetterCellList list = letters();
    foreach( LetterCell *cell, list ) {
        if ( !cell->isCorrect() )
            return false;
    }

    return true;
}

void ClueCell::solve()
{
    setCurrentAnswer( correctAnswer(), Solved );
}

void ClueCell::clear( ClearMode clearMode )
{
    if ( clearMode == ClearCurrentLetter )
        setCurrentAnswer( QString().fill( ' ', correctAnswer().length() ), Confident );
    else // if ( clearMode == LetterCell::ClearCorrectLetter )
        setCorrectAnswer( QString().fill( ' ', correctAnswer().length() ) );
}

bool ClueCell::isEmpty() const
{
    LetterCellList letterList = letters();
    foreach( LetterCell *letter, letterList ) {
        if ( !letter->isEmpty() )
            return false;
    }

    return true;
}

bool ClueCell::isCorrectAnswerEmpty() const
{
    return m_correctAnswer.trimmed().isEmpty();
}

void ClueCell::setHighlight( bool enable )
{
//   kDebug() << enable;
    if ( isHighlighted() == enable )
        return;

#if QT_VERSION >= 0x040600
    krossWord()->animator()->beginEnqueueAnimations();
#endif

    KrossWordCell::setHighlight( enable );
    if ( enable )
        krossWord()->setHighlightedClue( this );
    else
        krossWord()->setHighlightedClue( NULL );

    LetterCellList letterList = letters();
    foreach( LetterCell *letterCell, letterList )
    letterCell->setHighlight( enable );

#if QT_VERSION >= 0x040600
    krossWord()->animator()->endEnqueueAnimations();
#endif
}

void ClueCell::answerLetterChanged( LetterCell* letter, const QChar &newLetter )
{
    Q_UNUSED( letter );
    Q_UNUSED( newLetter );

    emit currentAnswerChanged( this, currentAnswer() );
}

void ClueCell::setCorrectAnswer( const QString& correctAnswer )
{
    Q_ASSERT( m_correctAnswer.length() == correctAnswer.length() );
    if ( m_correctAnswer == correctAnswer )
        return;

//     LetterCellList letterList = letters();
//     foreach ( LetterCell *letter, letterList )
//  letter->clearCache();

    m_correctAnswer = correctAnswer;

    Qt::Orientation otherOrientation = orientation() == Qt::Horizontal
                                       ? Qt::Vertical : Qt::Horizontal;
    int pos = 0;
    LetterCellList list = letters();

#if QT_VERSION >= 0x040600
    krossWord()->animator()->beginEnqueueAnimations();
#endif
    foreach( LetterCell *cell, list ) {
        // Adjust correct letter of clues in the other direction
        if ( cell->hasClueInDirection( otherOrientation ) ) {
            ClueCell *otherClue = cell->clue( otherOrientation );
            int otherPos = otherClue->posOfLetter( cell );
            if ( otherPos != -1 ) {
                QString otherAnswer = otherClue->correctAnswer();
                Q_ASSERT( otherAnswer.length() > otherPos );
                otherAnswer[ otherPos ] = correctAnswer[ pos ];
                otherClue->m_correctAnswer = otherAnswer;
            }
        }

        // Redraw with the new correct letter
        cell->clearCache();
        cell->update();

        ++pos;
    }
#if QT_VERSION >= 0x040600
    krossWord()->animator()->endEnqueueAnimations();
#endif

    emit correctAnswerChanged( this, m_correctAnswer );
}

void ClueCell::setClueNumber( int clueNumber )
{
    m_clueNumber = clueNumber;
    if ( answerOffset() == OnClueCell )
        firstLetter()->clearCache();
}



// Sorting functions:
bool lessThanClueNumber( const ClueCell *clueCell1, const ClueCell *clueCell2 )
{
    return clueCell1->clueNumber() < clueCell2->clueNumber();
}

bool greaterThanClueNumber( const ClueCell* clueCell1, const ClueCell* clueCell2 )
{
    return clueCell1->clueNumber() > clueCell2->clueNumber();
}

bool lessThanAnswerLength( const ClueCell* clueCell1, const ClueCell* clueCell2 )
{
    return clueCell1->correctAnswer().length() < clueCell2->correctAnswer().length();
}

bool greaterThanAnswerLength( const ClueCell* clueCell1, const ClueCell* clueCell2 )
{
    return clueCell1->correctAnswer().length() > clueCell2->correctAnswer().length();
}

bool lessThanOrientation( const ClueCell* clueCell1, const ClueCell* clueCell2 )
{
    return clueCell1->orientation() < clueCell2->orientation();
}

bool greaterThanOrientation( const ClueCell* clueCell1, const ClueCell* clueCell2 )
{
    return clueCell1->orientation() > clueCell2->orientation();
}

}; // namespace Crossword
