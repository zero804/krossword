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

#ifndef KROSSWORDLETTERCELL_H
#define KROSSWORDLETTERCELL_H

#include "krosswordcell.h"

namespace Crossword
{

class LetterCell : public KrossWordCell
{
    Q_OBJECT
    friend class KrossWord;
    friend class ClueCell;
    friend class SolutionLetterCell;

public:
    enum SiblingLetterCellFlag {
        DontJump = 0x00,
        JumpToOppositeEdge = 0x01,
        JumpOverNonLetterCells = 0x02,
        AllJumpFlags = JumpToOppositeEdge | JumpOverNonLetterCells
    };
    Q_DECLARE_FLAGS(SiblingLetterCellFlags, SiblingLetterCellFlag);

    LetterCell(KrossWord *krossWord, const Coord &coord,
               ClueCell *clueHorizontal, ClueCell *clueVertical);
    LetterCell(KrossWord *krossWord, const Coord &coord,
               ClueCell *clue, AnswerOffset answerOffset = OffsetInvalid);
    ~LetterCell();

    static const qreal BAR_WIDTH = 0.08; // in percent of the total cell width/height
    static const QColor editLetterColor() {
        return Qt::blue;
    };

    virtual bool isLetterCell() const {
        return true;
    };

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 5 };
    virtual int type() const {
        return Type;
    };

    ClueCell *clueHorizontal() const {
        return m_clueHorizontal;
    };
    ClueCell *clueVertical() const {
        return m_clueVertical;
    };
    /** Returns the clue of this letter cell if it only has one clue.
    * @warning Don't use this method, if this letter cell is crossed.
    * @see isCrossed() */
    ClueCell *clue() const {
        Q_ASSERT(!isCrossed());
        return m_clueHorizontal ? m_clueHorizontal : m_clueVertical;
    };
    ClueCell *clue(Qt::Orientation orientation) const {
        return orientation == Qt::Horizontal ? m_clueHorizontal : m_clueVertical;
    };
    ClueCellList clues() const {
        ClueCellList clueList;
        if (m_clueHorizontal)
            clueList << m_clueHorizontal;
        if (m_clueVertical)
            clueList << m_clueVertical;
        return clueList;
    };
    bool hasClueInDirection(Qt::Orientation orientation) const {
        return clue(orientation);
    };
    bool isCrossed() const {
        return m_clueHorizontal && m_clueVertical;
    };
    bool isAttachedToClue(ClueCell *clue) {
        if (!clue) return false;
        else return m_clueHorizontal == clue || m_clueVertical == clue;
    };
    bool isAttachedToClueExclusivly(ClueCell *clue) {
        return isAttachedToClue(clue) && !isCrossed();
    };

    void setPropertiesFrom(LetterCell *other);

    /** Returns the correct letter of this letter cell. */
    QChar correctLetter() const {
        return m_correctLetter;
    };
    /** Returns the current letter of this letter cell. */
    QChar currentLetter() const {
        return m_currentLetter;
    };
    void setCurrentLetter(const QChar &currentLetter,
                          Confidence confidence = Confident);
    void setCorrectLetter(const QChar &correctLetter);
    bool isCorrect() const {
        return m_currentLetter == correctLetter();
    };
    bool isEmpty() const {
        return m_currentLetter == ' ';
    };
    Confidence confidence() const {
        return m_confidence;
    };
    void setConfidence(Confidence confidence);

    bool switchHighlightedClue();
    /** Solves this letter cell by setting it's current letter to the correct letter. */
    void solve();
    /** Clears this letter cell by settings it's current letter to ' '. */
    void clear(ClearMode clearMode = ClearCurrentLetter);

    bool needsEndBar() const {
        return needsEndBar(Qt::Horizontal) || needsEndBar(Qt::Vertical);
    };
    bool needsEndBar(Qt::Orientation orientation) const;

    LetterCell *nextHighlightedLetterCell();
    LetterCell *previousHighlightedLetterCell();
    LetterCell *letterCellOnRight(SiblingLetterCellFlags siblingLetterCellFlag =
                                      DontJump) const;
    LetterCell *letterCellOnLeft(SiblingLetterCellFlags siblingLetterCellFlag =
                                     DontJump) const;
    LetterCell *letterCellOnTop(SiblingLetterCellFlags siblingLetterCellFlag =
                                    DontJump) const;
    LetterCell *letterCellOnBottom(SiblingLetterCellFlags siblingLetterCellFlag =
                                       DontJump) const;

    SolutionLetterCell *toSolutionLetter(int solutionLetterIndex);

    static Confidence stringToConfidence(const QString &string);
    static QString confidenceToString(Confidence confidence);

signals:
    /** Emitted when the current letter of this letter cell has changed. */
    void currentLetterChanged(LetterCell *letter, const QChar &currentLetter);

public slots:
    /** Sets the current letter to @p newLetter. */
    void setCurrentLetterSlot(LetterCell *letter, const QChar &newLetter);

protected slots:
    void orientationChanged(ClueCell *clue, Qt::Orientation orientation);
    void correctAnswerChanged(ClueCell *clue, const QString &correctAnswer);

#if QT_VERSION >= 0x040600
    void changeAnimValueChanged(const QVariant &value);
    void changeAnimFinished();
#endif

protected:
    LetterCell(KrossWord *krossWord, const Coord &coord,
               ClueCell *clueHorizontal, ClueCell *clueVertical,
               CellType cellType);
    LetterCell(KrossWord *krossWord, const Coord &coord, ClueCell *clue,
               CellType cellType,
               AnswerOffset answerOffset = OffsetInvalid);

    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void focusInEvent(QFocusEvent* event);
    virtual void focusOutEvent(QFocusEvent* event);

    /** Gets the clue of this letter cell that is orthogonal to the given
    * @p clue, or NULL if this letter cell has no other clue.
    * @note It is assumed that @p clue is one @ref horizontalClue()
    * and @ref verticalClue(). */
    ClueCell *getOrthogonalClueTo(ClueCell *clue) const;

    void setClueHorizontal(ClueCell *clue);
    void setClueVertical(ClueCell *clue);
    void setClue(ClueCell *clue);
    void setClue(ClueCell *clue, Qt::Orientation orientation);

    void detachClues();
    bool detachClue(ClueCell *cell);
    bool attachClue(ClueCell *cell);

    virtual void drawBackground(QPainter *p, const QStyleOptionGraphicsItem *option);
    virtual void drawForeground(QPainter *p, const QStyleOptionGraphicsItem *option);
    virtual void drawClueArrows(QPainter *p, const QStyleOptionGraphicsItem *option);
    virtual void drawClueForCell(QPainter *p, const QStyleOptionGraphicsItem *option);
    virtual void drawEndBarIfNeeded(QPainter *p, const QStyleOptionGraphicsItem *option);

    virtual void drawBackgroundForPrinting(QPainter *p, const QStyleOptionGraphicsItem *option);
    virtual void drawForegroundForPrinting(QPainter *p, const QStyleOptionGraphicsItem *option);

private:
    void init(ClueCell *clueHorizontal, ClueCell *clueVertical);
    void init(ClueCell *clue, AnswerOffset answerOffset = OffsetInvalid);

    inline LetterCell *letterCellAtOffset(Offset offset) const {
        return letterCellAt(coord() + offset);
    };
    LetterCell *letterCellAt(Coord coord) const;
    LetterCell *letterCellAtOppositeEdge(KGrid2D::SquareBase::Neighbour n) const;
    inline LetterCell *letterCellOnRight(KeyboardNavigation keyboardNavigation) const {
        return letterCellOnRight(keyboardNavigation.testFlag(NavigateJump)
                                 ? AllJumpFlags : DontJump);
    };
    inline LetterCell *letterCellOnLeft(KeyboardNavigation keyboardNavigation) const {
        return letterCellOnLeft(keyboardNavigation.testFlag(NavigateJump)
                                ? AllJumpFlags : DontJump);
    };
    inline LetterCell *letterCellOnTop(KeyboardNavigation keyboardNavigation) const {
        return letterCellOnTop(keyboardNavigation.testFlag(NavigateJump)
                               ? AllJumpFlags : DontJump);
    };
    inline LetterCell *letterCellOnBottom(KeyboardNavigation keyboardNavigation) const {
        return letterCellOnBottom(keyboardNavigation.testFlag(NavigateJump)
                                  ? AllJumpFlags : DontJump);
    };
    QChar correctLetterFromClue(AnswerOffset answerOffset = OffsetInvalid) const;

    ClueCell *m_clueHorizontal, *m_clueVertical;
    QChar m_currentLetter, m_correctLetter;
    Confidence m_confidence;
    bool m_hadFocusBeforeMousePress;

#if QT_VERSION >= 0x040600
    QPropertyAnimation *m_changeAnim;
#endif
};


class SolutionLetterCell : public LetterCell
{
    friend class KrossWord;
    friend class LetterCell;
    friend class ClueCell;
    Q_OBJECT

public:
    SolutionLetterCell(KrossWord *krossWord, const Coord &coord,
                       ClueCell *clueHorizontal, ClueCell *clueVertical,
                       int solutionWordIndex);
    SolutionLetterCell(KrossWord *krossWord, const Coord &coord,
                       ClueCell *clue, int solutionWordIndex);

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 6 };
    virtual int type() const {
        return Type;
    };

    int solutionWordIndex() const {
        return m_solutionWordIndex;
    };
    void setSolutionWordIndex(int solutionWordIndex);

    LetterCell *toLetter();
    static SolutionLetterCell *fromLetterCell(LetterCell *&letter,
            int solutionWordIndex, bool deleteLetter = true);

protected:
    virtual void drawForeground(QPainter* p, const QStyleOptionGraphicsItem* option);

private:
    // Use fromLetterCell()
    SolutionLetterCell(const LetterCell *letter, int solutionWordIndex);
    void init(int solutionWordIndex);
    static SolutionLetterCell *fromLetterCell(const LetterCell *letter,
            int solutionWordIndex);

    int m_solutionWordIndex;
};

QDebug &operator <<(QDebug debug, LetterCell *cell);

// Sorting functions
bool lessThanSolutionWordIndex(const SolutionLetterCell *cell1, const SolutionLetterCell *cell2);
bool greaterThanSolutionWordIndex(const SolutionLetterCell *cell1, const SolutionLetterCell *cell2);

}; // namespace Crossword
Q_DECLARE_OPERATORS_FOR_FLAGS(Crossword::LetterCell::SiblingLetterCellFlags)

#endif // KROSSWORDLETTERCELL_H
