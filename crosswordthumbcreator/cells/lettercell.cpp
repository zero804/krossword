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

#include <qevent.h>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <kglobalsettings.h>
#include <QFontDatabase>

LetterCell::LetterCell(KrossWord* krossWord, const Coord& coord,
                       ClueCell* clueHorizontal, ClueCell* clueVertical/*,
   QGraphicsScene* scene*/)
    : KrossWordCell(krossWord, LetterCellType, coord/*, scene*/)
{
    init(clueHorizontal, clueVertical);
}
LetterCell::LetterCell(KrossWord* krossWord, const Coord& coord,
                       ClueCell* question/*, QGraphicsScene* scene*/)
    : KrossWordCell(krossWord, LetterCellType, coord/*, scene*/)
{
    init(question);
}

LetterCell::LetterCell(KrossWord* krossWord, const Coord& coord,
                       ClueCell* clueHorizontal, ClueCell* clueVertical/*,
   QGraphicsScene* scene*/, CellType cellType)
    : KrossWordCell(krossWord, cellType, coord/*, scene*/)
{
    init(clueHorizontal, clueVertical);
}

LetterCell::LetterCell(KrossWord* krossWord, const Coord& coord,
                       ClueCell* clue/*, QGraphicsScene* scene*/, CellType cellType)
    : KrossWordCell(krossWord, cellType, coord/*, scene*/)
{
    init(clue);
}

void LetterCell::init(ClueCell* clueHorizontal, ClueCell* clueVertical)
{
    m_clueHorizontal = clueHorizontal;
    m_clueVertical = clueVertical;
    m_currentLetter = ' ';
    m_confidence = Confident;
}

void LetterCell::init(ClueCell* clue)
{
    m_clueHorizontal = m_clueVertical = NULL;
    setClue(clue);
    m_currentLetter = ' ';
    m_confidence = Confident;
}

QString LetterCell::confidenceToString(LetterCell::Confidence confidence)
{
//     Solved, /**< The letter is definetly correct, because it was solved. */
//      Confident, /**< Confident that the letter is correct. */
//      NotSure, /**< Unsure if the letter is correct. */
//      Unknown
    switch (confidence) {
    case Solved:
        return "Solved";
    case Confident:
        return "Confident";
    case Unsure:
        return "Unsure";
//  case Unknown:
    default:
        return "Unknown";
    }
}

LetterCell::Confidence LetterCell::stringToConfidence(const QString& string)
{
    QString lower = string.toLower();
    if (lower == "solved")
        return Solved;
    else if (lower == "confident")
        return Confident;
    else if (lower == "unsure")
        return Unsure;
    else // if ( lower == "unknown" )
        return Unknown;
}

void LetterCell::setConfidence(LetterCell::Confidence confidence)
{
    m_confidence = confidence;
    clearCache();
    update();
}

LetterCell* LetterCell::letterCellAtOffset(Offset offset) const
{
    Coord coordAtOffset = coord() + offset;
    KrossWordCell *cell;
    if (krossWord()->inside(coordAtOffset)
            && (cell = krossWord()->at(coordAtOffset)) && cell->isLetterCell())
        return (LetterCell*)cell;
    else
        return NULL;
}

LetterCell *LetterCell::letterCellOnRight() const
{
    return letterCellAtOffset(Offset(1, 0));
}

LetterCell *LetterCell::letterCellOnLeft() const
{
    return letterCellAtOffset(Offset(-1, 0));
}

LetterCell *LetterCell::letterCellOnTop() const
{
    return letterCellAtOffset(Offset(0, -1));
}

LetterCell *LetterCell::letterCellOnBottom() const
{
    return letterCellAtOffset(Offset(0, 1));
}

QChar LetterCell::correctLetter() const
{
    ClueCell *clue;
    int firstLetterOffset, letterOffset;

    if (m_clueHorizontal == NULL) {
        Q_ASSERT(m_clueVertical != NULL);   // At least one question is needed
        clue = m_clueVertical;
        firstLetterOffset = clue->firstLetterOffset().second;
        letterOffset = coord().second - clue->coord().second - firstLetterOffset;
    } else {
        clue = m_clueHorizontal;
        firstLetterOffset = clue->firstLetterOffset().first;
        letterOffset = coord().first - clue->coord().first - firstLetterOffset;
    }

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

    clearCache();
    update();
}

void LetterCell::setCurrentLetter(const QChar& currentLetter, Confidence confidence)
{
    if (!allowedLetters().contains(currentLetter, Qt::CaseInsensitive)) {
        m_currentLetter = ' ';
//  m_confidence = Unknown;
    } else {
        m_currentLetter = currentLetter;
        m_confidence = confidence;
    }
    clearCache();
    update();
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
    Q_ASSERT(cell != NULL);

    if (m_clueHorizontal == cell)
        m_clueHorizontal = NULL;
    else if (m_clueVertical == cell)
        m_clueVertical = NULL;
    else
        return false;

    return true;
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
            QPointF topLeft = option->rect.topLeft() + // pos()+
                              QPointF((1.0 - BAR_WIDTH) * option->rect.width(), 0);
            p->fillRect(QRectF(topLeft,
                               QSize(BAR_WIDTH * option->rect.width(), option->rect.height())),
                        krossWord()->emptyCellColorForPrinting());
        } else {
            QPointF topLeft = option->rect.topLeft() + //pos() +
                              QPointF(0, (1.0 - BAR_WIDTH) * option->rect.height());
            p->fillRect(QRectF(topLeft,
                               QSize(option->rect.width(), BAR_WIDTH * option->rect.height())),
                        krossWord()->emptyCellColorForPrinting());
        }
    }
}

void LetterCell::drawBackgroundForPrinting(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    qDebug() << "Draw Letter Cell Background";

    QPen pen(Qt::black);
    p->setPen(pen);
    p->drawRect(option->rect);
}

void LetterCell::drawForegroundForPrinting(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    qDebug() << "Draw Letter Cell Foreground";

    drawClueArrows(p, option);
    drawEndBarIfNeeded(p, option);

    if (isEmpty())
        return;

    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPixelSize((float)option->rect.height() * 0.8);

    p->setFont(font);
//     int margin = (option->rect.width() * 0.2f;
//     QRect rect = option->rect.adjusted( margin, margin, -margin, -margin );
    QTextOption to;
    to.setAlignment(Qt::AlignCenter);
    p->drawText(option->rect, currentLetter().toUpper(), to);
}

void LetterCell::drawClueArrows(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    int arrowLength = option->rect.height() / 5;
    int arrowWidth = option->rect.height() * 0.3f;
    int x = option->rect.width() * 0.35f;
    int y = option->rect.height() * 0.35f;

    if (clueHorizontal() && clueHorizontal()->firstLetter() == this) {
        p->save();
        switch (clueHorizontal()->answerOffset()) {
        case ClueCell::OnClueCell:
        case ClueCell::OffsetRight:
            p->fillRect(QRect(option->rect.left() + 1, option->rect.top() + y, arrowLength, arrowWidth), Qt::gray);
//   KrosswordRenderer::self()->renderElement( p, "arrow_right",
//       QRect(option->rect.left() + 1, option->rect.top() + y, arrowLength, arrowWidth) );
            break;
        case ClueCell::OffsetTop:
            p->fillRect(QRect(option->rect.left() + x, option->rect.top() + krossWord()->cellSize().height() - 1 - arrowWidth, arrowLength, arrowWidth), Qt::gray);
//   KrosswordRenderer::self()->renderElement( p, "arrow_bottom_right",
// //       QRect(option->rect.left() + x, option->rect.top() + krossWord()->cellSize().height() - 1- arrowWidth, arrowLength, arrowWidth) );
            break;
        case ClueCell::OffsetBottom:
            p->fillRect(QRect(option->rect.left() + x, option->rect.top() + 1, arrowLength, arrowWidth), Qt::gray);
//   KrosswordRenderer::self()->renderElement( p, "arrow_top_right",
//       QRect(option->rect.left() + x, option->rect.top() + 1, arrowLength, arrowWidth) );
            break;
        case ClueCell::OffsetTopRight:
            p->fillRect(QRect(option->rect.left() + 1, option->rect.top() + krossWord()->cellSize().height() - 1 - arrowWidth, arrowLength, arrowWidth), Qt::gray);
//   KrosswordRenderer::self()->renderElement( p, "arrow_bottomleft_right",
//       QRect(option->rect.left() + 1, option->rect.top() + krossWord()->cellSize().height() - 1 - arrowWidth, arrowLength, arrowWidth) );
            break;
        case ClueCell::OffsetBottomRight:
            p->fillRect(QRect(option->rect.left() + 1, option->rect.top() + 1, arrowLength, arrowWidth), Qt::gray);
//   KrosswordRenderer::self()->renderElement( p, "arrow_topleft_right",
//       QRect(option->rect.left() + 1, option->rect.top() + 1, arrowLength, arrowWidth) );
            break;
        default:
            qDebug() << "Can't draw clue arrow for letterPosition" << clueHorizontal()->answerOffset();
        }
        p->restore();

        if (clueHorizontal()->answerOffset() == ClueCell::OnClueCell)
            clueHorizontal()->drawClueNumber(p , option);
    }

    if (clueVertical() && clueVertical()->firstLetter() == this) {
        p->save();
        switch (clueVertical()->answerOffset()) {
        case ClueCell::OnClueCell:
        case ClueCell::OffsetBottom:
            p->fillRect(QRect(option->rect.left() + x, option->rect.top() + 1, arrowLength, arrowWidth), Qt::gray);
//   KrosswordRenderer::self()->renderElement( p, "arrow_down",
//       QRect(option->rect.left() + x, option->rect.top() + 1, arrowLength, arrowWidth) );
            break;
        case ClueCell::OffsetRight:
            p->fillRect(QRect(option->rect.left() + 1, option->rect.top() + y, arrowLength, arrowWidth), Qt::gray);
//   KrosswordRenderer::self()->renderElement( p, "arrow_left_down",
//       QRect(option->rect.left() + 1, option->rect.top() + y, arrowLength, arrowWidth) );
            break;
        case ClueCell::OffsetLeft:
            p->fillRect(QRect(option->rect.left() + krossWord()->cellSize().width() - 1 - arrowLength, option->rect.top() + y, arrowLength, arrowWidth), Qt::gray);
//   KrosswordRenderer::self()->renderElement( p, "arrow_right_down",
//       QRect(option->rect.left() + krossWord()->cellSize().width() - 1 - arrowLength, option->rect.top() + y, arrowLength, arrowWidth) );
            break;
        case ClueCell::OffsetBottomRight:
            p->fillRect(QRect(option->rect.left() + 1, option->rect.top() + 1, arrowLength, arrowWidth), Qt::gray);
//   KrosswordRenderer::self()->renderElement( p, "arrow_topleft_down",
//       QRect(option->rect.left() + 1, option->rect.top() + 1, arrowLength, arrowWidth) );
            break;
        case ClueCell::OffsetBottomLeft:
            p->fillRect(QRect(option->rect.left() + krossWord()->cellSize().width() - 1 - arrowLength, option->rect.top() + 1, arrowLength, arrowWidth), Qt::gray);
//   KrosswordRenderer::self()->renderElement( p, "arrow_topright_down",
//       QRect(option->rect.left() + krossWord()->cellSize().width() - 1 - arrowLength, option->rect.top() + 1, arrowLength, arrowWidth) );
            break;
        default:
            qDebug() << "Can't draw clue arrow for letterPosition" << clueVertical()->answerOffset();
        }
        p->restore();

        if (clueVertical()->answerOffset() == ClueCell::OnClueCell)
            clueVertical()->drawClueNumber(p , option);
    }
}

void LetterCell::setClue(ClueCell* clue)
{
    if (clue->isHorizontal())
        m_clueHorizontal = clue;
    else
        m_clueVertical = clue;
}

void LetterCell::detachClues()
{
    if (m_clueHorizontal != NULL)
        m_clueHorizontal = NULL;
    if (m_clueVertical != NULL)
        m_clueVertical = NULL;
}


SolutionLetterCell::SolutionLetterCell(KrossWord* krossWord, const Coord& coord,
                                       ClueCell* questionHorizontal,
                                       ClueCell* questionVertical, int solutionWordIndex)
    : LetterCell(krossWord, coord, questionHorizontal,
                 questionVertical, SolutionLetterCellType)
{
    init(solutionWordIndex);
}

SolutionLetterCell::SolutionLetterCell(KrossWord* krossWord, const Coord& coord,
                                       ClueCell* question, int solutionWordIndex)
    : LetterCell(krossWord, coord, question, SolutionLetterCellType)
{
    init(solutionWordIndex);
}

SolutionLetterCell::SolutionLetterCell(LetterCell* letter, int solutionWordIndex)
    : LetterCell(letter->krossWord(), letter->coord(),
                 letter->clueHorizontal(), letter->clueVertical(),
                 SolutionLetterCellType)
{
    init(solutionWordIndex);
}

void SolutionLetterCell::init(int solutionWordIndex)
{
    m_solutionWordIndex = solutionWordIndex;
    krossWord()->solutionWordLettersNonConst().insert(solutionWordIndex, this);
}

SolutionLetterCell* SolutionLetterCell::fromLetterCell(LetterCell *&letter,
        int solutionWordIndex, bool deleteLetter)
{
    SolutionLetterCell *solutionLetter = new SolutionLetterCell(letter, solutionWordIndex);
    solutionLetter->setConfidence(letter->confidence());

    if (deleteLetter) {
        letter->detachClues();
        if (letter->scene())
            letter->scene()->removeItem(letter);
        delete letter;
        letter = NULL;
    }
    return solutionLetter;
}

void SolutionLetterCell::drawForeground(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    LetterCell::drawForegroundForPrinting(p, option);
}
