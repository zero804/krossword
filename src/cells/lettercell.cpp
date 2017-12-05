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

#include "lettercell.h"

#include "krossword.h"
#include "krosswordrenderer.h"
#include "cluecell.h"
#include "krosswordtheme.h"

#include <qevent.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QApplication>
#include <QStyleOption>
#include <QPainter>

#include <QPropertyAnimation>
#include <kglobalsettings.h>
#include <QFontDatabase>

namespace Crossword
{

LetterCell::LetterCell(KrossWord* krossWord, const Coord& coord,
                       ClueCell* clueHorizontal, ClueCell* clueVertical)
    : KrossWordCell(krossWord, LetterCellType, coord),
      m_clueHorizontal(0), m_clueVertical(0), m_changeAnim(0)
{
    init(clueHorizontal, clueVertical);
}
LetterCell::LetterCell(KrossWord* krossWord, const Coord& coord,
                       ClueCell* clue, AnswerOffset answerOffset)
    : KrossWordCell(krossWord, LetterCellType, coord),
      m_clueHorizontal(0), m_clueVertical(0), m_changeAnim(0)
{
    init(clue, answerOffset);
}

const QColor LetterCell::editLetterColor() {
    return Qt::blue;
}

LetterCell::LetterCell(KrossWord* krossWord, const Coord& coord,
                       ClueCell* clueHorizontal, ClueCell* clueVertical,
                       CellType cellType)
    : KrossWordCell(krossWord, cellType, coord),
      m_clueHorizontal(0), m_clueVertical(0), m_changeAnim(0)
{
    init(clueHorizontal, clueVertical);
}

LetterCell::LetterCell(KrossWord* krossWord, const Coord& coord,
                       ClueCell* clue, CellType cellType,
                       AnswerOffset answerOffset)
    : KrossWordCell(krossWord, cellType, coord),
      m_clueHorizontal(0), m_clueVertical(0), m_changeAnim(0)
{
    init(clue, answerOffset);
}

void LetterCell::init(ClueCell* clueHorizontal, ClueCell* clueVertical)
{
    setClueHorizontal(clueHorizontal);
    setClueVertical(clueVertical);
    m_currentLetter = ' ';
    m_correctLetter = correctLetterFromClue();
    m_confidence = Confident;
    m_hadFocusBeforeMousePress = false;
    setCursor(Qt::IBeamCursor);
}

void LetterCell::init(ClueCell* clue, AnswerOffset answerOffset)
{
    setClue(clue);
    m_currentLetter = ' ';
    m_correctLetter = correctLetterFromClue(answerOffset);
    m_confidence = Confident;
    m_hadFocusBeforeMousePress = false;
    setCursor(Qt::IBeamCursor);
}

LetterCell *LetterCell::letterCellAtOffset(Offset offset) const {
    return letterCellAt(coord() + offset);
}

bool LetterCell::isLetterCell() const {
    return true;
}

int LetterCell::type() const {
    return Type;
}

ClueCell *LetterCell::clueHorizontal() const {
    return m_clueHorizontal;
}

ClueCell *LetterCell::clueVertical() const {
    return m_clueVertical;
}

ClueCell *LetterCell::clue() const {
    Q_ASSERT(!isCrossed());
    return m_clueHorizontal ? m_clueHorizontal : m_clueVertical;
}

ClueCell *LetterCell::clue(Qt::Orientation orientation) const {
    return orientation == Qt::Horizontal ? m_clueHorizontal : m_clueVertical;
}

ClueCellList LetterCell::clues() const {
    ClueCellList clueList;
    if (m_clueHorizontal)
        clueList << m_clueHorizontal;
    if (m_clueVertical)
        clueList << m_clueVertical;
    return clueList;
}

bool LetterCell::hasClueInDirection(Qt::Orientation orientation) const {
    return clue(orientation);
}

bool LetterCell::isCrossed() const {
    return m_clueHorizontal && m_clueVertical;
}

bool LetterCell::isAttachedToClue(ClueCell *clue) {
    if (!clue) {
        return false;
    } else {
        return m_clueHorizontal == clue || m_clueVertical == clue;
    }
}

bool LetterCell::isAttachedToClueExclusivly(ClueCell *clue) {
    return isAttachedToClue(clue) && !isCrossed();
}

void LetterCell::setConfidence(Confidence confidence)
{
    m_confidence = confidence;
    clearCache();
    update();
}

void LetterCell::focusInEvent(QFocusEvent* event)
{
    //     qDebug() << "IN" << this->coord();

    if (event->reason() == Qt::MouseFocusReason) {
//       qDebug() << "    IN > MOUSE FOCUS REASON";
        return; // Handled by mousePressEvent(), also if the cell already has focus
    }
    m_hadFocusBeforeMousePress = true;

    // Only if none of the clues of this letter cell is already highlighted
    if (!krossWord()->highlightedClue() ||
            (krossWord()->highlightedClue() != clueHorizontal()
             && krossWord()->highlightedClue() != clueVertical())) {
        Qt::Orientation orientation = Qt::Horizontal;
        if (krossWord()->highlightedClue()
                && hasClueInDirection(krossWord()->highlightedClue()->orientation())) {
            orientation = krossWord()->highlightedClue()->orientation();
        } else {
            bool firstLetterOfVerticalQuestion = clueVertical()
                                                 && clueVertical()->firstLetter() == this;
            orientation = clueHorizontal() && !firstLetterOfVerticalQuestion
                          ? Qt::Horizontal : Qt::Vertical;
        }
//  qDebug() << "Decision for orientation" << orientation;
//  qDebug() << "NEW HIGHLIGHTED CLUE" << (orientation == Qt::Horizontal
//    ? clueHorizontal() : clueVertical());

        krossWord()->setHighlightedClue(orientation == Qt::Horizontal
                                        ? clueHorizontal() : clueVertical());
        event->accept();
    } /*else
      qDebug() << "A clue of this letter cell is already highlighted" << this;*/

    KrossWordCell::focusInEvent(event);
}

void LetterCell::focusOutEvent(QFocusEvent* event)
{
//     qDebug() << "OUT" << this->coord();

    // Only remove highlight if the scene still has focus
//     if ( scene() && scene()->hasFocus() )
//  setHighlight( false );
    m_hadFocusBeforeMousePress = false;
    KrossWordCell::focusOutEvent(event);
}

LetterCell *LetterCell::nextHighlightedLetterCell()
{
    if (krossWord()->highlightedClue() && krossWord()->highlightedClue()->lastLetter() != this) {
        const Offset letterOffset = KrosswordGrid::letterOffset(krossWord()->highlightedClue()->orientation());
        return (LetterCell*)krossWord()->at(coord() + letterOffset);
    }

    return this; // last letter highlighted or no highlighted clue
}

LetterCell *LetterCell::previousHighlightedLetterCell()
{
    if (krossWord()->highlightedClue() &&
            krossWord()->highlightedClue()->firstLetter() != this) {
        const Offset letterOffset = krossWord()->highlightedClue()->orientation()
                                    == Qt::Horizontal ? Offset(-1, 0) : Offset(0, -1);
        return (LetterCell*)krossWord()->at(coord() + letterOffset);
    }

    return this; // first letter highlighted or no highlighted clue
}

LetterCell* LetterCell::letterCellAt(Coord coord) const
{
    KrossWordCell *cell;

    if (krossWord()->inside(coord)
            && (cell = krossWord()->at(coord)) && cell->isLetterCell())
        return dynamic_cast< LetterCell* >(cell);
    else
        return nullptr;
}

LetterCell* LetterCell::letterCellAtOppositeEdge(Grid2D::SquareBase::Neighbour n) const
{
    Coord oppositeCoord = krossWord()->m_krossWordGrid->toEdge(coord(), n);
    KrossWordCell *oppositeCell = krossWord()->at(oppositeCoord);
    return dynamic_cast< LetterCell* >(oppositeCell);
}

LetterCell *LetterCell::letterCellOnRight(KeyboardNavigation keyboardNavigation) const {
    return letterCellOnRight(keyboardNavigation.testFlag(NavigateJump)
                             ? AllJumpFlags : DontJump);
}

LetterCell *LetterCell::letterCellOnLeft(KeyboardNavigation keyboardNavigation) const {
    return letterCellOnLeft(keyboardNavigation.testFlag(NavigateJump)
                            ? AllJumpFlags : DontJump);
}

LetterCell *LetterCell::letterCellOnTop(KeyboardNavigation keyboardNavigation) const {
    return letterCellOnTop(keyboardNavigation.testFlag(NavigateJump)
                           ? AllJumpFlags : DontJump);
}

LetterCell *LetterCell::letterCellOnBottom(KeyboardNavigation keyboardNavigation) const {
    return letterCellOnBottom(keyboardNavigation.testFlag(NavigateJump)
                              ? AllJumpFlags : DontJump);
}

LetterCell *LetterCell::letterCellOnRight(SiblingLetterCellFlags siblingLetterCellFlags) const
{
    if (siblingLetterCellFlags.testFlag(JumpOverNonLetterCells)
            || siblingLetterCellFlags.testFlag(JumpToOppositeEdge)) {
        LetterCell *letter = nullptr;
        Coord otherCoord = coord() + Offset(1, 0);

        if (siblingLetterCellFlags.testFlag(JumpToOppositeEdge)) {
            if (!siblingLetterCellFlags.testFlag(JumpOverNonLetterCells))
                otherCoord.first = 0;
            do {
                if (krossWord()->inside(otherCoord)) {
                    if ((letter = letterCellAt(otherCoord)))
                        break;
                }
                otherCoord += Offset(1, 0);
                if (otherCoord.first >= (int)krossWord()->width())
                    otherCoord.first = 0;
            } while (otherCoord != coord());   // stop after checking the whole row
        } else {
            while (krossWord()->inside(otherCoord) && !(letter = letterCellAt(otherCoord)))
                otherCoord += Offset(1, 0);
        }

        return letter;
    } else
        return letterCellAtOffset(Offset(1, 0));
}

LetterCell *LetterCell::letterCellOnLeft(SiblingLetterCellFlags siblingLetterCellFlags) const
{
    if (siblingLetterCellFlags.testFlag(JumpOverNonLetterCells)
            || siblingLetterCellFlags.testFlag(JumpToOppositeEdge)) {
        LetterCell *letter = nullptr;
        Coord otherCoord = coord() + Offset(-1, 0);

        if (siblingLetterCellFlags.testFlag(JumpToOppositeEdge)) {
            if (!siblingLetterCellFlags.testFlag(JumpOverNonLetterCells))
                otherCoord.first = krossWord()->width() - 1;
            do {
                if (krossWord()->inside(otherCoord)) {
                    if ((letter = letterCellAt(otherCoord)))
                        break;
                }
                otherCoord += Offset(-1, 0);
                if (otherCoord.first < 0)
                    otherCoord.first = krossWord()->width() - 1;
            } while (otherCoord != coord());   // stop after checking the whole row
        } else {
            while (krossWord()->inside(otherCoord) && !(letter = letterCellAt(otherCoord)))
                otherCoord += Offset(-1, 0);
        }

        return letter;
    } else
        return letterCellAtOffset(Offset(-1, 0));
}

LetterCell *LetterCell::letterCellOnTop(SiblingLetterCellFlags siblingLetterCellFlags) const
{
    if (siblingLetterCellFlags.testFlag(JumpOverNonLetterCells)
            || siblingLetterCellFlags.testFlag(JumpToOppositeEdge)) {
        LetterCell *letter = nullptr;
        Coord otherCoord = coord() + Offset(0, -1);

        if (siblingLetterCellFlags.testFlag(JumpToOppositeEdge)) {
            if (!siblingLetterCellFlags.testFlag(JumpOverNonLetterCells))
                otherCoord.second = krossWord()->height() - 1;
            do {
                if (krossWord()->inside(otherCoord)) {
                    if ((letter = letterCellAt(otherCoord)))
                        break;
                }
                otherCoord += Offset(0, -1);
                if (otherCoord.second < 0)
                    otherCoord.second = krossWord()->height() - 1;
            } while (otherCoord != coord());   // stop after checking the whole row
        } else {
            while (krossWord()->inside(otherCoord) && !(letter = letterCellAt(otherCoord)))
                otherCoord += Offset(0, -1);
        }

        return letter;
    } else
        return letterCellAtOffset(Offset(0, -1));
}

LetterCell *LetterCell::letterCellOnBottom(SiblingLetterCellFlags siblingLetterCellFlags) const
{
    if (siblingLetterCellFlags.testFlag(JumpOverNonLetterCells)
            || siblingLetterCellFlags.testFlag(JumpToOppositeEdge)) {
        LetterCell *letter = nullptr;
        Coord otherCoord = coord() + Offset(0, 1);

        if (siblingLetterCellFlags.testFlag(JumpToOppositeEdge)) {
            if (!siblingLetterCellFlags.testFlag(JumpOverNonLetterCells))
                otherCoord.second = 0;
            do {
                if (krossWord()->inside(otherCoord)) {
                    if ((letter = letterCellAt(otherCoord)))
                        break;
                }
                otherCoord += Offset(0, 1);
                if (otherCoord.second >= (int)krossWord()->height())
                    otherCoord.second = 0;
            } while (otherCoord != coord());   // stop after checking the whole row
        } else {
            while (krossWord()->inside(otherCoord) && !(letter = letterCellAt(otherCoord)))
                otherCoord += Offset(0, 1);
        }

        return letter;
    } else
        return letterCellAtOffset(Offset(0, 1));
}

void LetterCell::keyPressEvent(QKeyEvent* event)
{
    if (event->text().length() >= 1 && krossWord()->crosswordTypeInfo().isCharacterLegal(event->text().at(0))) {
        event->accept();
        QChar newChar = event->text().toUpper()[0];
        switch (krossWord()->letterEditMode()) {
        case AutomaticKeyboardEditing:
            if (krossWord()->isEditable())
                setCorrectLetter(newChar);
            else
                setCurrentLetter(newChar);
            break;
        case EmitEditRequestsOnKeyboardEdit:
            if ((!krossWord()->isEditable() && currentLetter() != newChar)
                    || (krossWord()->isEditable() && correctLetter() != newChar))
                krossWord()->emitLetterEditRequest(this, currentLetter(), newChar);
            break;

        case NoEditing:
            break; // Do nothing
        }

        if (krossWord()->keyboardNavigation().testFlag(NavigateForthOnLetterEdit)) {
            // Give some time to animations
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);

            nextHighlightedLetterCell()->setFocus();
        }
    } else {
        switch (event->key()) {
            // Qt::Key_Tab is handled by KrossWordPuzzleScene::keyPressEvent
            LetterCell *cell;
        case Qt::Key_Backspace:
            event->accept();
            switch (krossWord()->letterEditMode()) {
            case AutomaticKeyboardEditing:
                if (krossWord()->isEditable())
                    setCorrectLetter(ClueCell::EmptyCorrectCharacter);
                else
                    setCurrentLetter(' ');
                break;
            case EmitEditRequestsOnKeyboardEdit:
                if ((!krossWord()->isEditable() && !isEmpty())
                        || (krossWord()->isEditable() && correctLetter() != ' '))
                    krossWord()->emitLetterEditRequest(this, currentLetter(), ' ');
                break;

            case NoEditing:
                break; // Do nothing
            }


            if (krossWord()->keyboardNavigation().testFlag(NavigateBackOnBackspace)) {
                // Give some time to animations
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);

                previousHighlightedLetterCell()->setFocus();
            }
            break;

        case Qt::Key_Left:
            event->accept();
            if (krossWord()->keyboardNavigation().testFlag(
                        NavigateSwitchOrientationOnArrowKeyNavigation)
                    && krossWord()->highlightedClue()
                    && krossWord()->highlightedClue()->isVertical()) {
                switchHighlightedClue();
            }
            if (krossWord()->keyboardNavigation().testFlag(NavigateWithArrowKeys)) {
                if ((cell = letterCellOnLeft(krossWord()->keyboardNavigation())))
                    cell->setFocus();
            }
            break;

        case Qt::Key_Up:
            event->accept();
            if (krossWord()->keyboardNavigation().testFlag(
                        NavigateSwitchOrientationOnArrowKeyNavigation)
                    && krossWord()->highlightedClue()
                    && krossWord()->highlightedClue()->isHorizontal()) {
                switchHighlightedClue();
            }

            if (krossWord()->keyboardNavigation().testFlag(NavigateWithArrowKeys)) {
                if ((cell = letterCellOnTop(krossWord()->keyboardNavigation())))
                    cell->setFocus();
            }
            break;

        case Qt::Key_Right:
            event->accept();
            if (krossWord()->keyboardNavigation().testFlag(
                        NavigateSwitchOrientationOnArrowKeyNavigation)
                    && krossWord()->highlightedClue()
                    && krossWord()->highlightedClue()->isVertical()) {
                switchHighlightedClue();
            }
            if (krossWord()->keyboardNavigation().testFlag(NavigateWithArrowKeys)) {
                if ((cell = letterCellOnRight(krossWord()->keyboardNavigation())))
                    cell->setFocus();
            }
            break;

        case Qt::Key_Down:
            event->accept();
            if (krossWord()->keyboardNavigation().testFlag(
                        NavigateSwitchOrientationOnArrowKeyNavigation)
                    && krossWord()->highlightedClue()
                    && krossWord()->highlightedClue()->isHorizontal()) {
                switchHighlightedClue();
            }
            if (krossWord()->keyboardNavigation().testFlag(NavigateWithArrowKeys)) {
                if ((cell = letterCellOnBottom(krossWord()->keyboardNavigation())))
                    cell->setFocus();
            }
            break;
// Is now done through actions
//      case Qt::Key_Home:
//   event->accept();
//   if ( krossWord()->highlightedClue() )
//       krossWord()->highlightedClue()->firstLetter()->setFocus();
//   break;
//      case Qt::Key_End:
//   event->accept();
//   if ( krossWord()->highlightedClue() )
//       krossWord()->highlightedClue()->lastLetter()->setFocus();
//   break;
        case Qt::Key_Delete:
            event->accept();
            switch (krossWord()->letterEditMode()) {
            case AutomaticKeyboardEditing:
                if (krossWord()->isEditable())
                    setCorrectLetter(ClueCell::EmptyCorrectCharacter);
                else
                    setCurrentLetter(' ');
                break;
            case EmitEditRequestsOnKeyboardEdit:
                if ((!krossWord()->isEditable() && !isEmpty())
                        || (krossWord()->isEditable() && correctLetter() != ' '))
                    krossWord()->emitLetterEditRequest(this, currentLetter(), ' ');
                break;

            case NoEditing:
                break; // Do nothing
            }
            break;
        default:
            event->ignore();
        }
    }

    if (!event->isAccepted())
        QGraphicsItem::keyPressEvent(event);
}

bool LetterCell::switchHighlightedClue()
{
    ClueCell *clueCell = krossWord()->highlightedClue();
//     qDebug() << (clueCell ? clueCell->clue() : "<empty>");
//     qDebug() << "SWITCH" << this;

    if (clueCell && isCrossed()) {
        if (clueCell == clueHorizontal()) {
            krossWord()->setHighlightedClue(clueVertical());
            return true;
        } else if (clueCell == clueVertical()) {
            krossWord()->setHighlightedClue(clueHorizontal());
            return true;
        } else
            return false;
    } else
        return false;
}

void LetterCell::solve()
{
    setCurrentLetter(correctLetter(), Solved);
}

void LetterCell::clear(ClearMode clearMode)
{
    if (clearMode == ClearCurrentLetter)
        setCurrentLetter(' ');
    else // if ( clearMode == ClearCorrectLetter )
        setCorrectLetter(' ');
}

bool LetterCell::needsEndBar() const {
    return needsEndBar(Qt::Horizontal) || needsEndBar(Qt::Vertical);
}

QChar LetterCell::correctLetterFromClue(AnswerOffset answerOffset) const
{
    ClueCell *clue;
    int firstLetterOffset, letterOffset;

    if (!m_clueHorizontal) {
        Q_ASSERT(m_clueVertical);   // At least one question is needed
        clue = m_clueVertical;
        firstLetterOffset = clue->firstLetterOffset(answerOffset).second;
        letterOffset = coord().second - clue->coord().second - firstLetterOffset;

//  qDebug() << clue->clue() << "at" << clue->coord() << "| Letter is at" << coord()
//      << "| letterOffset (y) =" << coord().second << "-" << clue->coord().second
//      << "-" << firstLetterOffset << "=" << letterOffset
//      << "| answerOffset =" << clue->firstLetterOffset();
    } else {
        clue = m_clueHorizontal;
        firstLetterOffset = clue->firstLetterOffset(answerOffset).first;
        letterOffset = coord().first - clue->coord().first - firstLetterOffset;

//  qDebug() << clue->clue() << "at" << clue->coord() << "| Letter is at" << coord()
//      << "| letterOffset (x) =" << coord().first << "-" << clue->coord().first
//      << "-" << firstLetterOffset << "=" << letterOffset
//      << "| answerOffset =" << clue->firstLetterOffset();
    }

//     qDebug() << "LetterCell::correctLetter | letterOffset =" << letterOffset
//       << "| firstLetterOffset =" << firstLetterOffset << "| clue =" << clue;
    Q_ASSERT(letterOffset >= 0 && letterOffset < clue->correctAnswer().length());
    return clue->correctAnswer().at(letterOffset);
}

void LetterCell::setCorrectLetter(const QChar& correctLetter)
{
    int firstLetterOffset, letterOffset;

    if (m_clueVertical) {
        firstLetterOffset = m_clueVertical->firstLetterOffset().second;
        letterOffset = coord().second - m_clueVertical->coord().second - firstLetterOffset;
        m_clueVertical->setCorrectAnswer(
            m_clueVertical->correctAnswer().replace(letterOffset, 1, correctLetter));
    }

    if (m_clueHorizontal) {
        firstLetterOffset = m_clueHorizontal->firstLetterOffset().first;
        letterOffset = coord().first - m_clueHorizontal->coord().first - firstLetterOffset;
        m_clueHorizontal->setCorrectAnswer(
            m_clueHorizontal->correctAnswer().replace(letterOffset, 1, correctLetter));
    }

    if (krossWord()->isAnimationEnabled()) {
        if (m_changeAnim) {
            if (!m_blockCacheClearing) {   // changeAnimValueChanged() already disconnected
                connect(m_changeAnim, SIGNAL(valueChanged(QVariant)),
                        this, SLOT(changeAnimValueChanged(QVariant)));
            }
            m_blockCacheClearing = true; // wait for scaleX == 0 ("90° rotated")
            m_changeAnim->setStartValue(m_changeAnim->currentValue());
            m_changeAnim->setCurrentTime(0);
        } else {
            m_blockCacheClearing = true; // wait for scaleX == 0 ("90° rotated")
            m_changeAnim = new QPropertyAnimation(this, "scaleX");
            connect(m_changeAnim, SIGNAL(valueChanged(QVariant)),
                    this, SLOT(changeAnimValueChanged(QVariant)));
            connect(m_changeAnim, SIGNAL(finished()), this, SLOT(changeAnimFinished()));
            m_changeAnim->setDuration(krossWord()->animator()->defaultDuration() * 2);
            m_changeAnim->setStartValue(1.0);
            m_changeAnim->setKeyValueAt(0.5, 0.0);
            m_changeAnim->setEndValue(1.0);
            m_changeAnim->setEasingCurve(QEasingCurve(QEasingCurve::InOutSine));
            m_changeAnim->start();
        }
    } else {
        clearCache(Crossword::Animator::Slow);
        update();
    }
}

bool LetterCell::isCorrect() const {
    return m_currentLetter == correctLetter();
}

bool LetterCell::isEmpty() const {
    return m_currentLetter == ' ';
}

Confidence LetterCell::confidence() const {
    return m_confidence;
}

void LetterCell::changeAnimValueChanged(const QVariant& value)
{
    if (m_blockCacheClearing && (value.toReal() < 0.01
                                 || m_changeAnim->currentTime() >= 0.5 * m_changeAnim->totalDuration())) {
//     qDebug() << value.toReal() << m_changeAnim->currentTime() << "/" << m_changeAnim->totalDuration();
        disconnect(m_changeAnim, SIGNAL(valueChanged(QVariant)),
                   this, SLOT(changeAnimValueChanged(QVariant)));
        m_blockCacheClearing = false;
        clearCache(Animator::Instant);
        update();
    }
}

void LetterCell::changeAnimFinished()
{
    delete m_changeAnim;
    m_changeAnim = nullptr;
}

void LetterCell::setCurrentLetter(const QChar& currentLetter,
                                  Confidence confidence)
{
    QChar newCurrentLetter;
    if (krossWord()->crosswordTypeInfo().isCharacterLegal(currentLetter))
        newCurrentLetter = currentLetter;
    else
        newCurrentLetter = ' ';

    if (m_currentLetter == newCurrentLetter)
        return;

    m_currentLetter = newCurrentLetter;
    m_confidence = confidence;

    if (krossWord()->isAnimationEnabled()) {
        if (m_changeAnim) {
            if (!m_blockCacheClearing) {   // changeAnimValueChanged() already disconnected
                connect(m_changeAnim, SIGNAL(valueChanged(QVariant)),
                        this, SLOT(changeAnimValueChanged(QVariant)));
            }
            m_blockCacheClearing = true; // wait for scaleX == 0 ("90° rotated")
            m_changeAnim->setStartValue(m_changeAnim->currentValue());
            m_changeAnim->setCurrentTime(0);
        } else {
            m_blockCacheClearing = true; // wait for scaleX == 0 ("90° rotated")
            m_changeAnim = new QPropertyAnimation(this, "scaleX");
            connect(m_changeAnim, SIGNAL(valueChanged(QVariant)),
                    this, SLOT(changeAnimValueChanged(QVariant)));
            connect(m_changeAnim, SIGNAL(finished()), this, SLOT(changeAnimFinished()));
            m_changeAnim->setDuration(krossWord()->animator()->defaultDuration() * 2);
            m_changeAnim->setStartValue(1.0);
            m_changeAnim->setKeyValueAt(0.5, 0.0);
            m_changeAnim->setEndValue(1.0);
            m_changeAnim->setEasingCurve(QEasingCurve(QEasingCurve::InOutSine));
            m_changeAnim->start();
        }
    } else {
        clearCache(Crossword::Animator::Slow);
        update();
    }

    emit currentLetterChanged(this, m_currentLetter);
}

void LetterCell::setCurrentLetterSlot(LetterCell* letter, const QChar& newLetter)
{
    Q_UNUSED(letter);
    setCurrentLetter(newLetter);
}

ClueCell* LetterCell::getOrthogonalClueTo(ClueCell* question) const
{
    Q_ASSERT(question == clueHorizontal() || question == clueVertical());

    if (question == clueHorizontal())
        return clueVertical();
    else
        return clueHorizontal();
}

bool LetterCell::detachClue(ClueCell* cell)
{
    Q_ASSERT(cell);

    if (m_clueHorizontal == cell)
        setClueHorizontal(nullptr);
    else if (m_clueVertical == cell)
        setClueVertical(nullptr);
    else
        return false;

    return true;
}

bool LetterCell::attachClue(ClueCell* cell)
{
    Q_ASSERT(cell);

    if (cell->orientation() == Qt::Horizontal) {
        if (m_clueHorizontal)
            return false; // First detach the old clue
        setClueHorizontal(cell);
    } else { // Vertical
        if (m_clueVertical)
            return false; // First detach the old clue
        setClueVertical(cell);
    }

    return true;
}

void LetterCell::setPropertiesFrom(LetterCell* other)
{
    m_confidence = other->confidence();
    setCurrentLetter(other->currentLetter());
}

QChar LetterCell::correctLetter() const {
    return m_correctLetter;
}

QChar LetterCell::currentLetter() const {
    return m_currentLetter;
}

void LetterCell::drawBackground(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    if (option->state.testFlag(QStyle::State_HasFocus) && !krossWord()->isDrawingForPrinting()) {
        KrosswordRenderer::self()->renderElement(p, "letter_cell_focus", option->rect);
    } else if (isHighlighted()) {
        KrosswordRenderer::self()->renderElement(p, "letter_cell_highlight", option->rect);
    } else {
        KrosswordRenderer::self()->renderElement(p, "letter_cell", option->rect);
    }
}

void LetterCell::drawBackgroundForPrinting(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    QPen pen(Qt::black);
    p->setPen(pen);
    p->drawRect(option->rect);
}

void LetterCell::drawForeground(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    if (krossWord()->crosswordTypeInfo().clueMapping != CluesReferToCells) {
        drawClueArrows(p, option);
    } else {
        drawClueForCell(p, option);
    }

    drawEndBarIfNeeded(p, option);

    // Draw current/correct letter
    QString id;
    QChar letter;
    if (krossWord()->isEditable()) {
        if (correctLetter() == ClueCell::EmptyCorrectCharacter) {
            return;
        }
        letter = correctLetter().toUpper();
        id = QString("correct_letter_%1").arg(correctLetter().toLower());
    } else {
        if (currentLetter().isSpace()) {
            return;
        }
        letter = currentLetter().toUpper();
        id = QString("letter_%1").arg(currentLetter().toLower());
    }

    qreal levelOfDetail = QStyleOptionGraphicsItem::levelOfDetailFromTransform(QTransform(option->matrix));
    QRect rect = KrosswordTheme::trimmedRect(option->rect, krossWord()->theme()->marginsLetterCell(levelOfDetail));

    QFont letterFont = p->font();
    letterFont.setPixelSize(rect.height());
    letterFont.setBold(true);

    p->setPen(QPen(krossWord()->theme()->fontColor()));
    p->setFont(letterFont);

    QFontMetrics fontmetrics(letterFont); //to adjust the rect in case of font with descender (portion under baseline)
    p->drawText(fontmetrics.boundingRect(rect, Qt::AlignCenter, letter), Qt::AlignCenter, letter);
}

bool LetterCell::needsEndBar(Qt::Orientation orientation) const
{
    ClueCell *clue;
    if (orientation == Qt::Horizontal)
        clue = clueHorizontal();
    else
        clue = clueVertical();

    if (clue) {
        LetterCell *letter = clue->lastLetter();
        if (letter == this) {
            Coord nextCoord = coord() + (clue->orientation() == Qt::Horizontal
                                         ? Offset(1, 0) : Offset(0, 1));
            if (krossWord()->inside(nextCoord)
                    && krossWord()->at(nextCoord)->isLetterCell())
                return true;
        }
    }

    return false;
}

void LetterCell::drawEndBarIfNeeded(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    foreach(ClueCell * clue, clues()) {
        if (!needsEndBar(clue->orientation()))
            continue;

        // TODO: Draw SVG-themed end bar
        if (clue->orientation() == Qt::Horizontal) {
            QPointF topLeft = pos() +
                              QPointF((1.0 - BAR_WIDTH) * option->rect.width(), 0);
            p->fillRect(QRectF(topLeft,
                               QSize(BAR_WIDTH * option->rect.width(), option->rect.height())),
                        krossWord()->isDrawingForPrinting()
                        ? krossWord()->emptyCellColorForPrinting() : Qt::black);
        } else {
            QPointF topLeft = pos() +
                              QPointF(0, (1.0 - BAR_WIDTH) * option->rect.height());
            p->fillRect(QRectF(topLeft,
                               QSize(option->rect.width(), BAR_WIDTH * option->rect.height())),
                        krossWord()->isDrawingForPrinting()
                        ? krossWord()->emptyCellColorForPrinting() : Qt::black);
        }
    }
}

void LetterCell::drawForegroundForPrinting(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    LetterCell::drawForeground(p, option);
}

void LetterCell::drawClueForCell(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    p->save();

    QString text;
    if (krossWord()->crosswordTypeInfo().clueType == NumberClues1To26 && krossWord()->crosswordTypeInfo().letterCellContent == Characters) {
        int clueNumber = krossWord()->letterContentToClueNumberMapping().indexOf(correctLetter()) + 1;
        if (clueNumber > 0)
            text = QString::number(clueNumber);
    } else
        text = clue()->clue();

    QFont font = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);

    qreal levelOfDetail = QStyleOptionGraphicsItem::levelOfDetailFromTransform(QTransform(option->matrix));

    font.setPixelSize(10 * levelOfDetail);
    font.setBold(true);
    p->setFont(font);
    p->setPen(isEnabled() ? Qt::black : Qt::darkGray);
    QFontMetrics fontMetrics(font);
    QRect rect = fontMetrics.boundingRect(text);
    rect.setWidth(fontMetrics.width(text));
    QRect trimmedRect = KrosswordTheme::trimmedRect(option->rect, krossWord()->theme()->marginsLetterCell(levelOfDetail));
    p->drawText(KrosswordTheme::rectAtPos(trimmedRect, rect, TopRight /*krossWord()->theme()->codedPuzzleCluePos()*/), text);
    p->restore();
}

void LetterCell::drawClueArrows(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    bool isFirstLetterOfVerticalClue = clueVertical() && clueVertical()->firstLetter() == this;
    bool isFirstLetterOfHorizontalClue = clueHorizontal() && clueHorizontal()->firstLetter() == this;
    if (!isFirstLetterOfVerticalClue && !isFirstLetterOfHorizontalClue)
        return;

    int arrowLength = option->rect.height()/5;
    int arrowWidth = arrowLength; //option->rect.height() * 0.3f;
    int x = option->rect.width() * 0.35f;
    int y;

    if (isFirstLetterOfHorizontalClue) {
        if (qgraphicsitem_cast<DoubleClueCell*>(clueHorizontal()->parentItem())) {
            y = (clueHorizontal()->boundingRect().top() > 0 ? 1.32 : 0.32) *
                clueHorizontal()->boundingRect().height();
        } else {
            y = option->rect.height() * 0.35f;
        }

        if(krossWord()->crosswordTypeInfo().crosswordType != CrosswordType::American) {
            switch (clueHorizontal()->answerOffset()) {
            case OnClueCell:
            case OffsetRight:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_right",
                        QRect(option->rect.left() + 1, option->rect.top() + y, arrowLength, arrowWidth));
                break;
            case OffsetTop:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_bottom_right",
                        QRect(option->rect.left() + x, option->rect.top() + krossWord()->getCellSize().height() - 1 - arrowWidth,
                              arrowLength, arrowWidth));
                break;
            case OffsetBottom:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_top_right",
                        QRect(option->rect.left() + x, option->rect.top() + 1,
                              arrowLength, arrowWidth));
                break;
            case OffsetTopRight:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_bottomleft_right",
                        QRect(option->rect.left() + 1, option->rect.top() +
                              krossWord()->getCellSize().height() - 1 - arrowWidth,
                              arrowLength, arrowWidth));
                break;
            case OffsetBottomRight:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_topleft_right",
                        QRect(option->rect.left() + 1, option->rect.top() + 1,
                              arrowLength, arrowWidth));
                break;
            case OffsetTopLeft:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_bottomright_right",
                        QRect(option->rect.left() + option->rect.width() - 1 - arrowLength,
                              option->rect.top() + krossWord()->getCellSize().height() - 1 - arrowWidth,
                              arrowLength, arrowWidth));
                break;
            case OffsetBottomLeft:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_topright_right",
                        QRect(option->rect.left() + option->rect.width() - 1 - arrowLength,
                              option->rect.top() + 1, arrowLength, arrowWidth));
                break;
            default:
                qDebug() << "Can't draw clue arrow for letterPosition"
                         << clueHorizontal()->answerOffset()
                         << "and orientation horizontal";
            }
        }

        if (clueHorizontal()->answerOffset() == OnClueCell)
            clueHorizontal()->drawClueNumber(p , option);
    }

    if (isFirstLetterOfVerticalClue) {
        if (qgraphicsitem_cast<DoubleClueCell*>(clueVertical()->parentItem())) {
            y = (clueVertical()->boundingRect().top() > 0 ? 1.32 : 0.32) *
                clueVertical()->boundingRect().height();
        } else {
            y = option->rect.height() * 0.35f;
        }

        if(krossWord()->crosswordTypeInfo().crosswordType != CrosswordType::American) {
            switch (clueVertical()->answerOffset()) {
            case OnClueCell:
            case OffsetBottom:
                KrosswordRenderer::self()->renderElement(p, "arrow_down",
                        QRect(option->rect.left() + x, option->rect.top() + 1,
                              arrowLength, arrowWidth));
                break;
            case OffsetRight:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_left_down",
                        QRect(option->rect.left() + 1, option->rect.top() + y,
                              arrowLength, arrowWidth));
                break;
            case OffsetLeft:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_right_down",
                        QRect(option->rect.left() + krossWord()->getCellSize().width() -
                              1 - arrowLength, option->rect.top() + y, arrowLength, arrowWidth));
                break;
            case OffsetBottomRight:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_topleft_down",
                        QRect(option->rect.left() + 1, option->rect.top() + 1,
                              arrowLength, arrowWidth));
                break;
            case OffsetBottomLeft:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_topright_down",
                        QRect(option->rect.left() + krossWord()->getCellSize().width() -
                              1 - arrowLength, option->rect.top() + 1, arrowLength, arrowWidth));
                break;
            case OffsetTopLeft:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_bottomright_down",
                        QRect(option->rect.left() + option->rect.width() - 1 - arrowLength,
                              option->rect.top() + krossWord()->getCellSize().height() - 1 - arrowWidth,
                              arrowLength, arrowWidth));
                break;
            case OffsetTopRight:
                KrosswordRenderer::self()->renderElement(p,
                        "arrow_bottomleft_down",
                        QRect(option->rect.left() + 1, option->rect.top() +
                              krossWord()->getCellSize().height() - 1 - arrowWidth,
                              arrowLength, arrowWidth));
                break;
            default:
                qDebug() << "Can't draw clue arrow for letterPosition"
                         << clueVertical()->answerOffset()
                         << "and orientation Vertical";
            }
        }

        if (clueVertical()->answerOffset() == OnClueCell)
            clueVertical()->drawClueNumber(p , option);
    }
}

void LetterCell::orientationChanged(ClueCell* clueCell,
                                    Qt::Orientation orientation)
{
    if (hasClueInDirection(orientation)) {
        // The clue cells orientation has been changed wrongly
        qDebug() << "Can't set clue as this letters clue with orientation"
                 << orientation << "because there already is a clue with that "
                 "orientation:" << clue(orientation);
        return;
    }

    if (clueCell == m_clueHorizontal) {
        m_clueVertical = clueCell;
        m_clueHorizontal = nullptr;
    } else {
        m_clueVertical = nullptr;
        m_clueHorizontal = clueCell;
    }
}

void LetterCell::correctAnswerChanged(ClueCell* clue,
                                      const QString& correctAnswer)
{
    Q_UNUSED(clue);
    Q_UNUSED(correctAnswer);

    m_correctLetter = correctLetterFromClue();
    clearCache();
    update();
}

void LetterCell::setClueHorizontal(ClueCell* clue)
{
    if (m_clueHorizontal) {
        disconnect(this, SIGNAL(currentLetterChanged(LetterCell*, const QChar&)),
                   m_clueHorizontal, SLOT(answerLetterChanged(LetterCell*, const QChar&)));
        disconnect(m_clueHorizontal, SIGNAL(orientationChanged(ClueCell*, Qt::Orientation)),
                   this, SLOT(orientationChanged(ClueCell*, Qt::Orientation)));
        disconnect(m_clueHorizontal, SIGNAL(correctAnswerChanged(ClueCell*, QString)),
                   this, SLOT(correctAnswerChanged(ClueCell*, QString)));
        m_clueHorizontal->letterRemoved(this);
    }

    m_clueHorizontal = clue;

    if (clue) {
        connect(this, SIGNAL(currentLetterChanged(LetterCell*, const QChar&)),
                clue, SLOT(answerLetterChanged(LetterCell*, const QChar&)));
        connect(clue, SIGNAL(orientationChanged(ClueCell*, Qt::Orientation)),
                this, SLOT(orientationChanged(ClueCell*, Qt::Orientation)));
        connect(clue, SIGNAL(correctAnswerChanged(ClueCell*, QString)),
                this, SLOT(correctAnswerChanged(ClueCell*, QString)));
        clue->letterAdded(this);
    }
}

void LetterCell::setClueVertical(ClueCell* clue)
{
    if (m_clueVertical) {
        disconnect(this, SIGNAL(currentLetterChanged(LetterCell*, const QChar&)),
                   m_clueVertical, SLOT(answerLetterChanged(LetterCell*, const QChar&)));
        disconnect(m_clueVertical, SIGNAL(orientationChanged(ClueCell*, Qt::Orientation)),
                   this, SLOT(orientationChanged(ClueCell*, Qt::Orientation)));
        disconnect(m_clueVertical, SIGNAL(correctAnswerChanged(ClueCell*, QString)),
                   this, SLOT(correctAnswerChanged(ClueCell*, QString)));
        m_clueVertical->letterRemoved(this);
    }

    m_clueVertical = clue;

    if (clue) {
        connect(this, SIGNAL(currentLetterChanged(LetterCell*, const QChar&)),
                clue, SLOT(answerLetterChanged(LetterCell*, const QChar&)));
        connect(clue, SIGNAL(orientationChanged(ClueCell*, Qt::Orientation)),
                this, SLOT(orientationChanged(ClueCell*, Qt::Orientation)));
        connect(clue, SIGNAL(correctAnswerChanged(ClueCell*, QString)),
                this, SLOT(correctAnswerChanged(ClueCell*, QString)));
        clue->letterAdded(this);
    }
}

void LetterCell::setClue(ClueCell* clue, Qt::Orientation orientation)
{
    if (orientation == Qt::Horizontal)
        setClueHorizontal(clue);
    else if (orientation == Qt::Vertical)
        setClueVertical(clue);
}

void LetterCell::setClue(ClueCell* clue)
{
    if (clue->isHorizontal())
        setClueHorizontal(clue);
    else
        setClueVertical(clue);
}

void LetterCell::detachClues()
{
    if (m_clueHorizontal)
        setClueHorizontal(nullptr);

    if (m_clueVertical)
        setClueVertical(nullptr);
}

void LetterCell::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
//   qDebug() << "MOUSE PRESS" << this->coord();

    if (m_hadFocusBeforeMousePress)
        switchHighlightedClue();

    KrossWordCell::mousePressEvent(event);
    if (m_clueHorizontal || m_clueVertical) {
//     qDebug() << "CALL FOCUS IN" << this->coord();
        // Call focus in event with reason != MouseReason
        // (needed to handle focus in events AFTER mouse press )
        focusInEvent(new QFocusEvent(QEvent::FocusIn));
    }
}

SolutionLetterCell* LetterCell::toSolutionLetter(int solutionLetterIndex)
{
    SolutionLetterCell *solutionLetter = SolutionLetterCell::fromLetterCell(this, solutionLetterIndex);

    // Calls deleteLater() on this cell
    krossWord()->replaceCell(coord(), solutionLetter);

    foreach(ClueCell * clue, clues()) {
        clue->findLetters();
    }

    return solutionLetter;
}

LetterCell* SolutionLetterCell::toLetter()
{
    LetterCell *letter = new LetterCell(krossWord(), coord(),
                                        clueHorizontal(), clueVertical());
    letter->setCurrentLetter(currentLetter());
    letter->setConfidence(confidence());
    letter->setHighlight(isHighlighted());

    // Calls deleteLater() on this cell
    krossWord()->replaceCell(coord(), letter);

    foreach(ClueCell * clue, clues())
    clue->findLetters();

    return letter;
}

SolutionLetterCell::SolutionLetterCell(KrossWord* krossWord, const Coord& coord,
                                       ClueCell* questionHorizontal,
                                       ClueCell* questionVertical, int solutionWordIndex)
    : LetterCell(krossWord, coord, questionHorizontal,
                 questionVertical,
                 SolutionLetterCellType)
{
    init(solutionWordIndex);
}

SolutionLetterCell::SolutionLetterCell(KrossWord* krossWord, const Coord& coord,
                                       ClueCell* question, int solutionWordIndex)
    : LetterCell(krossWord, coord, question,
                 SolutionLetterCellType)
{
    init(solutionWordIndex);
}

int SolutionLetterCell::type() const {
    return Type;
}

int SolutionLetterCell::solutionWordIndex() const {
    return m_solutionWordIndex;
}

SolutionLetterCell::SolutionLetterCell(const LetterCell* letter, int solutionWordIndex)
    : LetterCell(letter->krossWord(), letter->coord(),
                 letter->clueHorizontal(), letter->clueVertical(),
                 SolutionLetterCellType)
{
    setCurrentLetter(letter->currentLetter());
    setConfidence(letter->confidence());
    setHighlight(letter->isHighlighted());
    init(solutionWordIndex);
}

void SolutionLetterCell::init(int solutionWordIndex)
{
    m_solutionWordIndex = solutionWordIndex;
//     krossWord()->m_solutionLetters.insert( solutionWordIndex, this ); // is now done in KrossWord::replaceCell()
    connect(this, SIGNAL(currentLetterChanged(LetterCell*, QChar)),
            krossWord(), SLOT(solutionLetterChanged(LetterCell*, QChar)));
}

void SolutionLetterCell::setSolutionWordIndex(int solutionWordIndex)
{
    m_solutionWordIndex = solutionWordIndex;
    qSort(krossWord()->m_solutionLetters.begin(),
          krossWord()->m_solutionLetters.end(), lessThanSolutionWordIndex);

    clearCache();
    update();
}

SolutionLetterCell* SolutionLetterCell::fromLetterCell(const LetterCell *letter,
        int solutionWordIndex)
{
    SolutionLetterCell *solutionLetter = new SolutionLetterCell(letter,
            solutionWordIndex);

    return solutionLetter;
}

SolutionLetterCell* SolutionLetterCell::fromLetterCell(LetterCell *&letter,
        int solutionWordIndex,
        bool deleteLetter)
{
    SolutionLetterCell *solutionLetter = new SolutionLetterCell(letter,
            solutionWordIndex);

    if (deleteLetter) {
        letter->detachClues();
        if (letter->scene())
            letter->scene()->removeItem(letter);
        delete letter;
        letter = nullptr;
    }

    return solutionLetter;
}

void SolutionLetterCell::drawForeground(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    LetterCell::drawForeground(p, option);

    // Draw solution letter index
    p->save();
    QString text = QString("(%1)").arg(solutionWordIndex() + 1);
    QFont font = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);

    qreal levelOfDetail = QStyleOptionGraphicsItem::levelOfDetailFromTransform(QTransform(option->matrix));

    font.setPointSizeF(10 * levelOfDetail);

    p->setFont(font);
    p->setPen(isEnabled() ? Qt::black : Qt::darkGray);
    QFontMetrics fontMetrics(font);
    QRect rect = fontMetrics.boundingRect(text);
    rect.setWidth(fontMetrics.width(text));
    QRect trimmedRect = KrosswordTheme::trimmedRect(option->rect, krossWord()->theme()->marginsLetterCell(levelOfDetail));
    p->drawText(KrosswordTheme::rectAtPos(trimmedRect, rect, BottomLeft /*krossWord()->theme()->solutionLetterIndexPos()*/), text);
    p->restore();
}

QDebug& operator<<(QDebug debug, LetterCell* cell)
{
    if (!cell)
        return debug << "nullptr (LetterCell)";

    debug << (KrossWordCell*)cell
          << QString("Correct: %1, Current: %2")
          .arg(cell->correctLetter())
          .arg(cell->currentLetter()).toLatin1()
          << "  || ";

    int i = 1;
    foreach(ClueCell * clue, cell->clues()) {
//     debug << "CLUE:" << clue << " || ";
        debug << "CLUE" << i++ << ":";
        debug << clue;
    }

    return debug << "";
}

// Sorting functions:
bool lessThanSolutionWordIndex(const SolutionLetterCell* cell1, const SolutionLetterCell* cell2)
{
    return cell1->solutionWordIndex() < cell2->solutionWordIndex();
}

bool greaterThanSolutionWordIndex(const SolutionLetterCell* cell1, const SolutionLetterCell* cell2)
{
    return cell1->solutionWordIndex() > cell2->solutionWordIndex();
}

}; // namespace Crossword
