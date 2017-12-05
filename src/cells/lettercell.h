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
    Q_DECLARE_FLAGS(SiblingLetterCellFlags, SiblingLetterCellFlag)

    LetterCell(KrossWord *krossWord, const Coord &coord,
               ClueCell *clueHorizontal, ClueCell *clueVertical);
    LetterCell(KrossWord *krossWord, const Coord &coord,
               ClueCell *clue, AnswerOffset answerOffset = OffsetInvalid);
    virtual ~LetterCell() = default;
    static const QColor editLetterColor();

    static constexpr qreal BAR_WIDTH = 0.08; // in percent of the total cell width/height
    virtual bool isLetterCell() const override;

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 5 };
    virtual int type() const override;

    ClueCell *clueHorizontal() const;
    ClueCell *clueVertical() const;
    /** Returns the clue of this letter cell if it only has one clue.
    * @warning Don't use this method, if this letter cell is crossed.
    * @see isCrossed() */
    ClueCell *clue() const;
    ClueCell *clue(Qt::Orientation orientation) const;

    ClueCellList clues() const;
    bool hasClueInDirection(Qt::Orientation orientation) const;
    bool isCrossed() const;
    bool isAttachedToClue(ClueCell *clue);
    bool isAttachedToClueExclusivly(ClueCell *clue);

    void setPropertiesFrom(LetterCell *other);

    /** Returns the correct letter of this letter cell. */
    QChar correctLetter() const;
    /** Returns the current letter of this letter cell. */
    QChar currentLetter() const;
    void setCurrentLetter(const QChar &currentLetter, Confidence confidence = Confident);
    void setCorrectLetter(const QChar &correctLetter);

    bool isCorrect() const;
    bool isEmpty() const;
    Confidence confidence() const;
    void setConfidence(Confidence confidence);

    bool switchHighlightedClue();
    /** Solves this letter cell by setting it's current letter to the correct letter. */
    void solve();
    /** Clears this letter cell by settings it's current letter to ' '. */
    void clear(ClearMode clearMode = ClearCurrentLetter);

    bool needsEndBar() const;
    bool needsEndBar(Qt::Orientation orientation) const;

    LetterCell *nextHighlightedLetterCell();
    LetterCell *previousHighlightedLetterCell();
    LetterCell *letterCellOnRight(SiblingLetterCellFlags siblingLetterCellFlag = DontJump) const;
    LetterCell *letterCellOnLeft(SiblingLetterCellFlags siblingLetterCellFlag = DontJump) const;
    LetterCell *letterCellOnTop(SiblingLetterCellFlags siblingLetterCellFlag = DontJump) const;
    LetterCell *letterCellOnBottom(SiblingLetterCellFlags siblingLetterCellFlag = DontJump) const;

    SolutionLetterCell *toSolutionLetter(int solutionLetterIndex);

signals:
    /** Emitted when the current letter of this letter cell has changed. */
    void currentLetterChanged(LetterCell *letter, const QChar &currentLetter);

public slots:
    /** Sets the current letter to @p newLetter. */
    void setCurrentLetterSlot(LetterCell *letter, const QChar &newLetter);

protected slots:
    void orientationChanged(ClueCell *clue, Qt::Orientation orientation);
    void correctAnswerChanged(ClueCell *clue, const QString &correctAnswer);

    void changeAnimValueChanged(const QVariant &value);
    void changeAnimFinished();

protected:
    LetterCell(KrossWord *krossWord, const Coord &coord, ClueCell *clueHorizontal,
               ClueCell *clueVertical, CellType cellType);
    LetterCell(KrossWord *krossWord, const Coord &coord, ClueCell *clue,
               CellType cellType, AnswerOffset answerOffset = OffsetInvalid);

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

    LetterCell *letterCellAtOffset(Offset offset) const;
    LetterCell *letterCellAt(Coord coord) const;
    LetterCell *letterCellAtOppositeEdge(Grid2D::SquareBase::Neighbour n) const;
    LetterCell *letterCellOnRight(KeyboardNavigation keyboardNavigation) const;
    inline LetterCell *letterCellOnLeft(KeyboardNavigation keyboardNavigation) const;
    inline LetterCell *letterCellOnTop(KeyboardNavigation keyboardNavigation) const;
    inline LetterCell *letterCellOnBottom(KeyboardNavigation keyboardNavigation) const;
    QChar correctLetterFromClue(AnswerOffset answerOffset = OffsetInvalid) const;

    ClueCell *m_clueHorizontal, *m_clueVertical;
    QChar m_currentLetter, m_correctLetter;
    Confidence m_confidence;
    bool m_hadFocusBeforeMousePress;

    QPropertyAnimation *m_changeAnim;
};


class SolutionLetterCell : public LetterCell
{
    friend class KrossWord;
    friend class LetterCell;
    friend class ClueCell;
    Q_OBJECT

public:
    SolutionLetterCell(KrossWord *krossWord, const Coord &coord, ClueCell *clueHorizontal,
                       ClueCell *clueVertical, int solutionWordIndex);
    SolutionLetterCell(KrossWord *krossWord, const Coord &coord, ClueCell *clue, int solutionWordIndex);

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 6 };
    virtual int type() const override;

    int solutionWordIndex() const;
    void setSolutionWordIndex(int solutionWordIndex);

    LetterCell *toLetter();
    static SolutionLetterCell *fromLetterCell(LetterCell *&letter, int solutionWordIndex, bool deleteLetter = true);

protected:
    virtual void drawForeground(QPainter* p, const QStyleOptionGraphicsItem* option);

private:
    // Use fromLetterCell()
    SolutionLetterCell(const LetterCell *letter, int solutionWordIndex);
    void init(int solutionWordIndex);
    static SolutionLetterCell *fromLetterCell(const LetterCell *letter, int solutionWordIndex);

    int m_solutionWordIndex;
};

QDebug &operator <<(QDebug debug, LetterCell *cell);

// Sorting functions
bool lessThanSolutionWordIndex(const SolutionLetterCell *cell1, const SolutionLetterCell *cell2);
bool greaterThanSolutionWordIndex(const SolutionLetterCell *cell1, const SolutionLetterCell *cell2);

} // namespace Crossword
Q_DECLARE_OPERATORS_FOR_FLAGS(Crossword::LetterCell::SiblingLetterCellFlags)

#endif // KROSSWORDLETTERCELL_H
