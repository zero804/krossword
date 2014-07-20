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

#include <QGraphicsSceneMouseEvent>
#include <QFocusEvent>
#include <kdeversion.h>
#include <kglobalsettings.h>

DoubleClueCell::DoubleClueCell(KrossWord* krossWord, const Coord& coord,
                               ClueCell* clue1, ClueCell* clue2)
    : KrossWordCell(krossWord, DoubleClueCellType, coord)
{
    Q_ASSERT(clue1);
    Q_ASSERT(clue2);

    m_clue1 = clue1;
    m_clue2 = clue2;

    clue1->setParentItem(this);
    clue1->setPos(0, 0);
    clue2->setParentItem(this);
    clue2->setPos(0, 0);

    setPos(x() * 2, y() * 2);
}

QRectF ClueCell::boundingRect() const
{
    DoubleClueCell *doubleClueCell;
    if ((doubleClueCell = qgraphicsitem_cast<DoubleClueCell*>(parentItem()))) {
        // If the parent is a DoubleClueCell
        bool firstClueAboveSecond;
        Coord coord1 = doubleClueCell->clue1()->coord() + doubleClueCell->clue1()->firstLetterOffset();
        Coord coord2 = doubleClueCell->clue2()->coord() + doubleClueCell->clue2()->firstLetterOffset();
        if (coord1.second < coord2.second)
            firstClueAboveSecond = true;
        else if (coord1.second > coord2.second)
            firstClueAboveSecond = false;
        else
            firstClueAboveSecond = doubleClueCell->clue1()->isHorizontal();

        bool isAbove = doubleClueCell->clue1() == this ? firstClueAboveSecond : !firstClueAboveSecond;
        qreal halfHeight = krossWord()->cellSize().height() / 2.0;

        if (isAbove)
            return QRectF(x(), y(), krossWord()->cellSize().width(), halfHeight);
        else
            return QRectF(x(), y() + halfHeight, krossWord()->cellSize().width(), halfHeight);
    } else
        return KrossWordCell::boundingRect();
}

QRectF ClueCell::boundingRectIncludingAnswerCells() const
{
    QRectF rect = boundingRect();

    LetterCellList letterList = letters();
    foreach(LetterCell * letter, letterList)
    rect = rect.united(QRectF(letter->x() * 2, letter->y() * 2,
                              letter->boundingRect().width(), letter->boundingRect().height()));

    return rect;
}

ClueCell::ClueCell(KrossWord* krossWord, Coord coord,
                   Qt::Orientation orientation, AnswerOffset answerOffset,
                   QString clue, QString answer/*, QGraphicsScene* scene*/)
    : KrossWordCell(krossWord, ClueCellType, coord/*, scene*/)
{
    m_orientation = orientation;
    m_answerOffset = answerOffset;
    m_clue = clue;
    wrapClueText();
    m_clueNumber = -1;
    m_correctAnswer = answer.toUpper();

    if (answerOffset == OnClueCell)
        setVisible(false);
}

void ClueCell::setClue(const QString &clue)
{
    if (m_clue == clue)
        return;

    m_clue = clue;
    wrapClueText();
    clearCache();
    update();
}

void ClueCell::drawBackgroundForPrinting(QPainter *p, const QStyleOptionGraphicsItem *option)
{
    if (!isVisible())
        return;

//     p->save();
    QPen pen(Qt::black);
    p->setPen(pen);
    p->fillRect(option->rect, Qt::lightGray);
    p->drawRect(option->rect);

//     p->setPen( Qt::transparent );
//     p->setBrush( Qt::gray );
//     p->drawEllipse( option->rect.center(), (int)option->rect.width() / 4, (int)option->rect.height() / 4 );
//     p->restore();
}

Offset ClueCell::answerOffsetToOffset(AnswerOffset answerOffset)
{
    switch (answerOffset) {
    case OnClueCell:
        return Offset(0, 0);
    case OffsetTop:
        return Offset(0, -1);
    case OffsetBottom:
        return Offset(0, 1);
    case OffsetLeft:
        return Offset(-1, 0);
    case OffsetRight:
        return Offset(1, 0);
    case OffsetTopLeft:
        return Offset(-1, -1);
    case OffsetTopRight:
        return Offset(1, -1);
    case OffsetBottomLeft:
        return Offset(-1, 1);
    case OffsetBottomRight:
        return Offset(1, 1);
    case OffsetInvalid:
        kDebug() << "Invalid answerOffset value.";
        return Offset(0, 0);
    }

    kDebug() << "Unknown value of AnswerOffset:" << answerOffset;
    Q_ASSERT(false);   // Error
    return Offset(0, 0);
}

Coord ClueCell::firstLetterCoords(Coord clueCoords, ClueCell::AnswerOffset answerOffset)
{
    return clueCoords + ClueCell::answerOffsetToOffset(answerOffset);
}

LetterCellList ClueCell::letters() const
{
    LetterCellList letterList;
    Coord letterPos = coord() + firstLetterOffset();
    const Offset letterOffset = isHorizontal() ? Offset(1, 0) : Offset(0, 1);

    for (int i = 0; i < correctAnswer().length(); ++i) {
        letterList << (LetterCell*)(krossWord()->at(letterPos));
        letterPos = letterPos + letterOffset;
    }

    return letterList;
}

LetterCell* ClueCell::firstLetter() const
{
    Coord letterPos = coord() + firstLetterOffset();
    return (LetterCell*)krossWord()->at(letterPos);
}

LetterCell* ClueCell::lastLetter() const
{
    return letterAt(correctAnswer().length() - 1);
}

LetterCell* ClueCell::letterAt(int letterIndex) const
{
    Q_ASSERT(letterIndex >= 0 && letterIndex < m_correctAnswer.length());

    const Offset letterOffset = isHorizontal() ? Offset(1, 0) : Offset(0, 1);
    Coord letterPos = coord() + firstLetterOffset() + letterOffset * letterIndex;
    return (LetterCell*)krossWord()->at(letterPos);
}

int ClueCell::posOfLetter(LetterCell* letterCell) const
{
    LetterCellList letterList = letters();
    int i = 0;
    foreach(LetterCell * letter, letterList) {
        if (letter == letterCell)
            return i;
        ++i;
    }

    return -1; // Given letter cell doesn't belong to this clue
}

QString ClueCell::clueWithoutHyphens() const
{
    if (m_clue.isEmpty())
        return i18nc("Display text for empty clue texts", "<empty>");

    QString clueText = m_clue;
    QRegExp rx("[a-zßäöü]-[a-zßäöü]");
    int pos = 0;
    while ((pos = rx.indexIn(clueText, pos)) != -1) {
        clueText.remove(++pos, 1);   // Remove the hyphen
    }

    return clueText;
}

void ClueCell::wrapClueText()
{
    QString clueText = m_clue;
    clueText.replace(QRegExp("\\b-\\b", Qt::CaseInsensitive), "-\n");

    if (clueText.contains('\n')) {
        QStringList lines = clueText.split('\n');
        while (lines.count() < 4) {
            // Get longest line:
            int longestLength = 0;
            int iLongest = 0;
            for (int i = 0; i < lines.count(); ++i) {
                if (lines[i].length() > longestLength) {
                    longestLength = lines[ i ].length();
                    iLongest = i;
                }
            }

            // Break longest line at the middle most space character
            QString longestLine = lines[ iLongest ];
            if (longestLine.contains(' ')) {
                int iMiddle = longestLine.length() / 2;
                int iMiddleSpace = -1;
                for (int i = 0; i < iMiddle - 1; ++i) {
                    if (longestLine[iMiddle + i] == ' ') {
                        iMiddleSpace = iMiddle + i;
                        break;
                    } else if (longestLine[iMiddle - i] == ' ') {
                        iMiddleSpace = iMiddle - i;
                        break;
                    }
                }
                if (iMiddleSpace == -1)
                    break; // Would break a single character to a new line

                lines[ iLongest ].replace(iMiddleSpace, 1, '\n');
            } else
                break; // Longest line has no space character

            clueText = lines.join("\n");
            lines = clueText.split('\n');
        }
    }

    m_wrappedClue = clueText;
}

void ClueCell::drawForegroundForPrinting(QPainter *p, const QStyleOptionGraphicsItem *option)
{
    if (!isVisible())
        return;

    QRect boundingRect;
    QRect drawRect = option->rect.adjusted(1, 1, -1, -1);
    QRect testRect = drawRect;
    testRect.setHeight(testRect.height() * 10);

    QString clueText = m_wrappedClue;
    QFont font = KGlobalSettings::generalFont();
    qreal pointSize = option->rect.height() / 4;
    font.setPointSize(pointSize);
//     if ( (option->rect.height() / 3) > 6 )
//  font.setPixelSize( option->rect.height() / 3 );
//     QFont font = p->font();
//     qreal levelOfDetail = QStyleOptionGraphicsItem::levelOfDetailFromTransform(sceneTransform());
//     font.setPointSizeF( pointSize * levelOfDetail );
    p->setFont(font);

// TODO Less line spacing when using smaller fonts? (Howto?)
    boundingRect = p->fontMetrics().boundingRect(testRect, Qt::TextWordWrap, clueText);
    while ((boundingRect.height() > drawRect.height() ||
            boundingRect.width() > drawRect.width()) && font.pointSize() > 6) {
        pointSize -= 2;
        font.setPointSize(pointSize);
        p->setFont(font);
        boundingRect = p->fontMetrics().boundingRect(testRect, Qt::TextWordWrap, clueText);
    }

    QTextOption textOption(Qt::AlignLeft | Qt::AlignVCenter);
    if (clueText.contains('\n'))
        textOption.setWrapMode(QTextOption::ManualWrap);
    else
        textOption.setWrapMode(QTextOption::WordWrap);

    p->drawText(drawRect, clueText, textOption);
//     p->fillRect( option->rect, Qt::gray );

    drawClueNumber(p, option);
}

void ClueCell::drawClueNumber(QPainter *p, const QStyleOptionGraphicsItem *option)
{
    // Draw clue number if any
    if (m_answerOffset == OnClueCell && m_clueNumber != -1) {
        QString text = QString("%1.").arg(m_clueNumber + 1);
//  QFont font = p->font();
        QFont font = KGlobalSettings::generalFont();
        if ((option->rect.height() / 3) > 6)
            font.setPixelSize(option->rect.height() / 3);

//  qreal levelOfDetail = QStyleOptionGraphicsItem::levelOfDetailFromTransform(transform());
//  font.setPointSizeF( 7.0 * levelOfDetail );
        p->setFont(font);
//  QFontMetrics fontMetrics( font );
//  QRect rect = fontMetrics.boundingRect( text );
//  QPoint topLeft = option->rect.topLeft();// - QPoint( rect.width(), rect.height() );
//  qDebug() << "Draw text";
        int margin = option->rect.height() / 5;
        p->drawText(option->rect.adjusted(margin, margin, -margin, -margin), text);
//  p->drawText( QRect( topLeft, option->rect.size()).adjusted(3, 3, -3, -3), text );

//  p->save();
//  QRect rect = option->rect;
//  rect.setWidth( rect.width() / 5 );
//  rect.setHeight( rect.height() / 5 );
//  p->fillRect( rect, Qt::gray );
//  p->restore();
    }
}

QString ClueCell::currentAnswer(const QChar &pad) const
{
    QString currentAnswer;
    LetterCellList list = letters();
    foreach(LetterCell * cell, list) {
        if (cell->isEmpty())
            currentAnswer += pad;
        else
            currentAnswer += cell->currentLetter();
    }

    return currentAnswer;
}

void ClueCell::setCurrentAnswer(const QString &answer,
                                LetterCell::Confidence confidence)
{
    Q_ASSERT(m_correctAnswer.length() == answer.length());

    int pos = 0;
    LetterCellList list = letters();
    foreach(LetterCell * cell, list) {
        cell->setCurrentLetter(answer[pos++], confidence);
    }
}

bool ClueCell::isAnswerComplete() const
{
    LetterCellList list = letters();
    foreach(LetterCell * cell, list) {
        if (cell->isEmpty())
            return false;
    }

    return true;
}

bool ClueCell::isAnswerCorrect() const
{
    LetterCellList list = letters();
    foreach(LetterCell * cell, list) {
        if (!cell->isCorrect())
            return false;
    }

    return true;
}

bool ClueCell::isEmpty() const
{
    LetterCellList letterList = letters();
    foreach(LetterCell * letter, letterList) {
        if (!letter->isEmpty())
            return false;
    }

    return true;
}

bool ClueCell::isCorrectAnswerEmpty() const
{
    return m_correctAnswer.trimmed().isEmpty();
}

bool ClueCell::needsAcrossNumber() const
{
    // Check that there is a blank to the left of us
    if (isHorizontal() && (coord().first == 0
                           || krossWord()->at(coord() - Offset(1, 0))->cellType() == KrossWordCell::EmptyCellType)) {
        // Check that there is space (at least two cells) for a word here
//  if ( coord().first + 1 < krossWord()->width() && puzzleSolution[coordsToIndex(coord().first + 1, y, width)] != '.' )
        return true;
    }
    return false;
}

bool ClueCell::needsDownNumber() const
{
    // Check that there is a blank to the top of us
    if (isVertical() && (coord().second == 0
                         || krossWord()->at(coord() - Offset(0, 1))->cellType() == KrossWordCell::EmptyCellType)) {
        // Check that there is space (at least two cells) for a word here
//  if ( coord().second + 1 < width && puzzleSolution[coordsToIndex(x, y + 1, width)] != '.' )
        return true;
    }
    return false;
}

void ClueCell::setCorrectAnswer(const QString& correctAnswer)
{
    Q_ASSERT(m_correctAnswer.length() == correctAnswer.length());

    m_correctAnswer = correctAnswer;

    Qt::Orientation otherOrientation = orientation() == Qt::Horizontal
                                       ? Qt::Vertical : Qt::Horizontal;
    int pos = 0;
    LetterCellList list = letters();
    foreach(LetterCell * cell, list) {
        // Adjust correct letter of clues in the other direction
        if (cell->hasClueInDirection(otherOrientation)) {
            ClueCell *otherClue = cell->clue(otherOrientation);
            int otherPos = otherClue->posOfLetter(cell);
            if (otherPos != -1) {
                QString otherAnswer = otherClue->correctAnswer();
                Q_ASSERT(otherAnswer.length() > otherPos);
                otherAnswer[ otherPos ] = correctAnswer[ pos ];
                otherClue->m_correctAnswer = otherAnswer;
            }
        }

        // Redraw with the new correct letter
        cell->clearCache();
        cell->update();

        ++pos;
    }
}

void ClueCell::setClueNumber(int clueNumber)
{
    m_clueNumber = clueNumber;
    if (answerOffset() == OnClueCell)
        firstLetter()->clearCache();
}



// Sorting functions:
bool lessThanClueNumber(const ClueCell *clueCell1, const ClueCell *clueCell2)
{
    return clueCell1->clueNumber() < clueCell2->clueNumber();
}

bool greaterThanClueNumber(const ClueCell* clueCell1, const ClueCell* clueCell2)
{
    return clueCell1->clueNumber() > clueCell2->clueNumber();
}

bool lessThanAnswerLength(const ClueCell* clueCell1, const ClueCell* clueCell2)
{
    return clueCell1->correctAnswer().length() < clueCell2->correctAnswer().length();
}

bool greaterThanAnswerLength(const ClueCell* clueCell1, const ClueCell* clueCell2)
{
    return clueCell1->correctAnswer().length() > clueCell2->correctAnswer().length();
}

bool lessThanOrientation(const ClueCell* clueCell1, const ClueCell* clueCell2)
{
    return clueCell1->orientation() < clueCell2->orientation();
}

bool greaterThanOrientation(const ClueCell* clueCell1, const ClueCell* clueCell2)
{
    return clueCell1->orientation() > clueCell2->orientation();
}

