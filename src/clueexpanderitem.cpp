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

#include "clueexpanderitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QStyleOption>
#include <QPainter>
#include <KColorScheme>


#include <QPropertyAnimation>
ClueExpanderItem::ClueExpanderItem(KrossWord* krossWord, ClueCell* clueCell)
    : QGraphicsObject(krossWord), m_krossWord(krossWord), m_clue(clueCell),
      m_lastLetter(0)
{
    Q_ASSERT(clueCell);

    m_isDragging = false;
    m_origCorrectAnswer = clueCell->correctAnswer();
    setColor(normalColor());

    setZValue(1000);
    setVisible(krossWord->isEditable());
    setCursor(QCursor(clueCell->isHorizontal()
                      ? Qt::SizeHorCursor : Qt::SizeVerCursor));

    clueLastLetterChanged(m_clue->lastLetter());
    setPosAfterCell(m_lastLetter);

    connect(krossWord, SIGNAL(editModeChanged(bool)),
            this, SLOT(krosswordEditModeChanged(bool)));
    connect(clueCell, SIGNAL(answerLengthChanged(ClueCell*, int)),
            this, SLOT(clueAnswerLengthChanged(ClueCell*, int)));
    connect(clueCell, SIGNAL(correctAnswerChanged(ClueCell*, QString)),
            this, SLOT(clueCorrectAnswerChanged(ClueCell*, QString)));
    connect(clueCell, SIGNAL(xChanged()), this, SLOT(clueCellMoved()));
    connect(clueCell, SIGNAL(yChanged()), this, SLOT(clueCellMoved()));
    connect(clueCell, SIGNAL(lastLetterChanged(LetterCell*)),
            this, SLOT(clueLastLetterChanged(LetterCell*)));
}

void ClueExpanderItem::clueLastLetterChanged(LetterCell* lastLetter)
{
    if (m_lastLetter) {
        disconnect(m_lastLetter, SIGNAL(destroyed(QObject*)),
                   this, SLOT(lastLetterDestroyed(QObject*)));
        disconnect(m_lastLetter, SIGNAL(xChanged()), this, SLOT(clueCellMoved()));
        disconnect(m_lastLetter, SIGNAL(yChanged()), this, SLOT(clueCellMoved()));
    }

    m_lastLetter = lastLetter;

    if (m_lastLetter) {
        setPosAfterCell(m_lastLetter);
        connect(m_lastLetter, SIGNAL(destroyed(QObject*)),
                this, SLOT(lastLetterDestroyed(QObject*)));
        connect(m_lastLetter, SIGNAL(xChanged()), this, SLOT(clueCellMoved()));
        connect(m_lastLetter, SIGNAL(yChanged()), this, SLOT(clueCellMoved()));
    }
}

void ClueExpanderItem::lastLetterDestroyed(QObject*)
{
    m_lastLetter = NULL;
}

void ClueExpanderItem::clueCellMoved()
{
    setPosAfterCell(m_lastLetter);
}

void ClueExpanderItem::krosswordEditModeChanged(bool editable)
{
    setVisible(editable);
}

void ClueExpanderItem::clueAnswerLengthChanged(ClueCell *clueCell,
        int newLength)
{
    Q_UNUSED(newLength);
    setPosAfterCell(clueCell->lastLetter());
}

void ClueExpanderItem::clueCorrectAnswerChanged(ClueCell *clueCell,
        const QString &correctAnswer)
{
    Q_UNUSED(clueCell);
    m_origCorrectAnswer.replace(0, correctAnswer.length(), correctAnswer);
}

void ClueExpanderItem::setPosAfterCell(LetterCell* letter)
{
    if (m_clue->isHorizontal()) {
        setCursor(QCursor(Qt::SizeHorCursor));
        m_homePos = letter->pos() +
                    QPointF((m_krossWord->getCellSize().width() - EXPANDER_WIDTH) / 2,
                            -m_krossWord->getCellSize().height() / 2);
    } else {
        setCursor(QCursor(Qt::SizeVerCursor));
        m_homePos = letter->pos() +
                    QPointF(-m_krossWord->getCellSize().width() / 2,
                            (m_krossWord->getCellSize().height() - EXPANDER_WIDTH) / 2);
    }

    prepareGeometryChange();
    setPos(m_homePos);
}

QRectF ClueExpanderItem::boundingRect() const
{
    if (m_clue->isHorizontal())
        return QRectF(0, 0, EXPANDER_WIDTH, m_krossWord->getCellSize().height());
    else
        return QRectF(0, 0, m_krossWord->getCellSize().height(), EXPANDER_WIDTH);
}

void ClueExpanderItem::paint(QPainter* painter,
                             const QStyleOptionGraphicsItem* option,
                             QWidget* widget)
{
    Q_UNUSED(widget);
    painter->fillRect(option->rect, m_color);
}

void ClueExpanderItem::setColor(const QColor& color)
{
    m_color = color;
    m_color.setAlpha(170);
    update();
}

const QColor ClueExpanderItem::normalColor() const
{
    // QColor(0, 0, 200, 170);
    return KColorScheme(QPalette::Active).decoration(
               KColorScheme::FocusColor).color();
}

const QColor ClueExpanderItem::dragColor() const
{
    return KColorScheme(QPalette::Active).background(
               KColorScheme::NeutralBackground).color();
}

void ClueExpanderItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
    m_isDragging = true;
    grabMouse();

    QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "color");
    fadeAnim->setDuration(250);
    fadeAnim->setStartValue(color());
    fadeAnim->setEndValue(dragColor());
    fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
//   QGraphicsItem::mousePressEvent( event );
}

void ClueExpanderItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
    m_isDragging = false;
    setPos(m_homePos);
    ungrabMouse();

    QPropertyAnimation *fadeAnim = new QPropertyAnimation(this, "color");
    fadeAnim->setDuration(250);
    fadeAnim->setStartValue(color());
    fadeAnim->setEndValue(normalColor());
    fadeAnim->start(QAbstractAnimation::DeleteWhenStopped);
//   QGraphicsItem::mouseReleaseEvent( event );
}

void ClueExpanderItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_isDragging) {
        LetterCell *letter = m_clue->lastLetter();
        Q_ASSERT(letter);

        qreal length, diff;
        QPointF dragPos;
        if (m_clue->isHorizontal()) {
            qreal x = event->scenePos().x();
            dragPos = QPointF(x, m_homePos.y());

            length = m_krossWord->getCellSize().width();
            diff = (x - letter->x()) / length - 1;
        } else {
            qreal y = event->scenePos().y();
            dragPos = QPointF(m_homePos.x(), y);

            length = m_krossWord->getCellSize().height();
            diff = (y - letter->y()) / length - 1;
        }

        bool snapped = false;
        qreal modulo = qAbs(length);
        while (modulo > length)
            modulo -= length;
        if (modulo > length * 0.8f) {
            snapped = true;
            int cellDiff = qRound(diff);
            cellDiff = m_clue->canAddLetters(cellDiff);

            if (cellDiff != 0) {
                int oldAnswerLength = m_clue->correctAnswer().length();
                emit addLettersToClueRequest(m_clue, cellDiff);

                // Restore correct letters of readded letter cells
                for (int i = oldAnswerLength; i < m_clue->correctAnswer().length(); ++i) {
                    LetterCell *letter = m_clue->letterAt(i);
                    if (letter->isCrossed())
                        continue;

                    if (i < m_origCorrectAnswer.length())
                        letter->setCorrectLetter(m_origCorrectAnswer[i]);
                }
            }
        }

        if (!snapped)
            setPos(dragPos);
    }

    QGraphicsItem::mouseMoveEvent(event);
}


