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

#ifndef CLUEEXPANDERITEM_H
#define CLUEEXPANDERITEM_H

#include "krossword.h"
#include "cells/cluecell.h"

#if QT_VERSION >= 0x040600
#include <QGraphicsObject>
#else
#include <QGraphicsItem>
#endif

using namespace Crossword;

class QGraphicsSceneMouseEvent;
#if QT_VERSION >= 0x040600
class ClueExpanderItem : public QGraphicsObject
{
#else
class ClueExpanderItem : public QObject, public QGraphicsItem
{
#endif
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor)

#if QT_VERSION >= 0x040600
    Q_INTERFACES(QGraphicsItem)
#endif

public:
    ClueExpanderItem(KrossWord *krossWord, ClueCell *clueCell);

    static const int EXPANDER_WIDTH = 6;

    KrossWord *krossWord() const {
        return m_krossWord;
    };
    ClueCell *clue() const {
        return m_clue;
    };
    QColor color() const {
        return m_color;
    };
    void setColor(const QColor &color);

    const QColor normalColor() const;
    const QColor dragColor() const;

signals:
    void addLettersToClueRequest(ClueCell *clue, int lettersToAdd);

protected slots:
    void krosswordEditModeChanged(bool editable);
    void clueAnswerLengthChanged(ClueCell *clueCell, int newLength);
    void clueCorrectAnswerChanged(ClueCell *clueCell, const QString &correctAnswer);
    void clueCellMoved();
    void clueLastLetterChanged(LetterCell *lastLetter);

protected:
    virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter,
                       const QStyleOptionGraphicsItem* option,
                       QWidget* widget = 0);

    virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);

private:
    void setPosAfterCell(LetterCell *letter);

    KrossWord *m_krossWord;
    ClueCell *m_clue;
    LetterCell *m_lastLetter;
    QPointF m_homePos;
    bool m_isDragging;
    QString m_origCorrectAnswer;
    QColor m_color;
public slots:
    void lastLetterDestroyed(QObject*);
};

#endif // KROSSWORDLETTERCELL_H
