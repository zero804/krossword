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

#ifndef ANSWERWIDGET_H
#define ANSWERWIDGET_H

#include <QLineEdit>

class AnswerWidget : public QLineEdit
{
public:
    AnswerWidget(QWidget *parent = 0);
    explicit AnswerWidget(const QString& string, QWidget *parent = 0);

protected:
    virtual void focusInEvent(QFocusEvent *ev);
    virtual void mousePressEvent(QMouseEvent *ev);

private:
    bool m_justGotFocusByMouse;
};

#endif // ANSWERWIDGET_H
