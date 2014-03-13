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

#ifndef KROSSWORDCLUECELL_H
#define KROSSWORDCLUECELL_H

#include "krosswordcell.h"
#include "lettercell.h"

class DoubleClueCell : public KrossWordCell
{
    Q_OBJECT

public:
    DoubleClueCell(KrossWord* krossWord, const Coord& coord,
                   ClueCell *clue1, ClueCell *clue2);

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 4 };
    virtual int type() const {
        return Type;
    };

    ClueCell *clue1() {
        return m_clue1;
    };
    ClueCell *clue2() {
        return m_clue2;
    };

    bool hasClue(ClueCell *clue) {
        return m_clue1 == clue || m_clue2 == clue;
    };

private:
    ClueCell *m_clue1;
    ClueCell *m_clue2;
};

class ClueCell : public KrossWordCell
{
    Q_OBJECT
    friend class LetterCell;

public:
    /** Where the first letter cell of the answer to a clue is, relative to
    * the clue cell position. */
    enum AnswerOffset {
        OffsetInvalid,
        OnClueCell, /**< The clue cell isn't shown and the first letter is
      placed at the coordinates of the clue. */
        OffsetTop,
        OffsetBottom,
        OffsetLeft,
        OffsetRight,
        OffsetBottomLeft,
        OffsetBottomRight,
        OffsetTopLeft,
        OffsetTopRight
    };

    static const char EmptyCorrectCharacter = ' ';
    static QList< ClueCell::AnswerOffset > allAnswerOffsets() {
        return QList< ClueCell::AnswerOffset >()
               << ClueCell::OffsetTopLeft << ClueCell::OffsetTop << ClueCell::OffsetTopRight
               << ClueCell::OffsetLeft << ClueCell::OnClueCell << ClueCell::OffsetRight
               << ClueCell::OffsetBottomLeft << ClueCell::OffsetBottom << ClueCell::OffsetBottomRight;
    };

    ClueCell(KrossWord *krossWord, Coord coord,
             Qt::Orientation orientation, AnswerOffset answerOffset,
             QString clue, QString answer/*, QGraphicsScene *scene*/);

    /** For qgraphicsitem_cast. */
    enum { Type = UserType + 3 };
    virtual int type() const {
        return Type;
    };

    virtual QRectF boundingRect() const;

    QRectF boundingRectIncludingAnswerCells() const;

    Qt::Orientation orientation() const {
        return m_orientation;
    };
    bool isHorizontal() const {
        return m_orientation == Qt::Horizontal;
    }
    bool isVertical() const {
        return m_orientation == Qt::Vertical;
    };

    /** Gets the clue to be displayed in the clue cell. */
    QString clue() const {
        return m_clue;
    };
    void setClue(const QString &clue);
    QString clueWithoutHyphens() const;
    QString clueWithNumber(QString format = "%1. ") const {
        QString nr = clueNumber() == -1 ? "" : QString(format).arg(clueNumber() + 1);
        return nr + clueWithoutHyphens();
    };
    /** Gets the correct answer. */
    QString correctAnswer() const {
        return m_correctAnswer;
    };
    /** Gets the current answer by concatenating the current letter
    * (@ref LetterCell::currentLetter()) of the letter cells of this clue
    * (@ref letters()).
    * @param pad This character is inserted for each empty letter
    * (@ref LetterCell::isEmpty()). */
    QString currentAnswer(const QChar &pad = '-') const;
    void setCurrentAnswer(const QString &answer,
                          LetterCell::Confidence confidence = LetterCell::Confident);
    /** Returns true if no letter cell of this clue is empty.
    * @see LetterCell::isEmpty() */
    bool isAnswerComplete() const;
    /** Returns true if all letter cells of this clue are correct.
    * @see LetterCell::isCorrect() */
    bool isAnswerCorrect() const;

    bool isEmpty() const;
    bool isCorrectAnswerEmpty() const;

    //  Returns true if the cell at (x, y) gets an "across" clue number.
    bool needsAcrossNumber() const;
    //  Returns true if the cell at (x, y) gets an "down" clue number.
    bool needsDownNumber() const;

    AnswerOffset answerOffset() const {
        return m_answerOffset;
    };
    /** The offset of the first letter of the answer in the crossword grid.
    * Can be (0 or +-1, 0 or +-1). It is (0, 0) if the clue is hidden. The
    * clue cell is hidden, if @ref answerOffset() is
    * @ref ClueCell::ClueHidden. */
    Offset firstLetterOffset() const {
        return answerOffsetToOffset(m_answerOffset);
    };
    static Offset answerOffsetToOffset(AnswerOffset answerOffset);
    static Coord firstLetterCoords(Coord clueCoords, AnswerOffset answerOffset);
    bool isHidden() const {
        return m_answerOffset == OnClueCell;
    };

    /** Returns a list of all letter cells that represent the answer to this clue. */
    LetterCellList letters() const;
    /** Gets the first letter cell of the answer to this clue cell.
    * @see lastLetter()
    * @see letterAt() */
    LetterCell *firstLetter() const;
    /** Gets the last letter cell of the answer to this clue cell.
    * @see firstLetter()
    * @see letterAt() */
    LetterCell *lastLetter() const;
    /** The the letter cell at @p letterIndex from the answer letter cells.
    * @see firstLetter()
    * @see lastLetter()
    * @see letters() */
    LetterCell *letterAt(int letterIndex) const;
    int posOfLetter(LetterCell *letterCell) const;
    bool answerContainsLetter(LetterCell *letterCell) const {
        return posOfLetter(letterCell) != -1;
    };

    int clueNumber() const {
        return m_clueNumber;
    };
    void setClueNumber(int clueNumber);

    /** Sets the correct answer to this clue cell. */
    void setCorrectAnswer(const QString &correctAnswer);

protected:
    virtual void drawClueNumber(QPainter *p, const QStyleOptionGraphicsItem *option);

    virtual void drawBackgroundForPrinting(QPainter* , const QStyleOptionGraphicsItem*);
    virtual void drawForegroundForPrinting(QPainter* , const QStyleOptionGraphicsItem*);

    virtual void wrapClueText();

private:
    Qt::Orientation m_orientation;
    AnswerOffset m_answerOffset;
    QString m_clue, m_wrappedClue, m_correctAnswer;
    int m_clueNumber;
};


// Sorting functions
bool lessThanClueNumber(const ClueCell *clueCell1, const ClueCell *clueCell2);
bool greaterThanClueNumber(const ClueCell *clueCell1, const ClueCell *clueCell2);

bool lessThanAnswerLength(const ClueCell *clueCell1, const ClueCell *clueCell2);
bool greaterThanAnswerLength(const ClueCell *clueCell1, const ClueCell *clueCell2);

/** Horizontal clues first. */
bool lessThanOrientation(const ClueCell *clueCell1, const ClueCell *clueCell2);
/** Vertical clues first. */
bool greaterThanOrientation(const ClueCell *clueCell1, const ClueCell *clueCell2);

#endif // KROSSWORDCLUECELL_H
