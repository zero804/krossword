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

#include "answerwidget.h"
#include <KLocalizedString>
#include <QFocusEvent>
#include <QDebug>

AnswerWidget::AnswerWidget( QWidget* parent )
        : KLineEdit( parent )
{
    setToolTip( i18n( "The correct answer to the clue. For each letter a letter "
                      "cell is created" ) );
    setClickMessage( i18n( "The answer to the clue" ) );
    setClearButtonShown( true );
    m_justGotFocusByMouse = false;
}

AnswerWidget::AnswerWidget( const QString &text, QWidget *parent )
        : KLineEdit( text, parent )
{
    setToolTip( i18n( "The correct answer to the clue. For each letter a letter "
                      "cell is created" ) );
    setClickMessage( i18n( "The answer to the clue" ) );
    setClearButtonShown( true );
    m_justGotFocusByMouse = false;
}

void AnswerWidget::focusInEvent( QFocusEvent* ev )
{
    KLineEdit::focusInEvent( ev );

    if ( ev->reason() == Qt::MouseFocusReason )
        m_justGotFocusByMouse = true;
}

void AnswerWidget::mousePressEvent( QMouseEvent* ev )
{
    if ( m_justGotFocusByMouse ) {
        m_justGotFocusByMouse = false;
        if ( cursorPositionAt( ev->pos() ) > text().length() )
            home( false );
        else
            KLineEdit::mousePressEvent( ev );
    } else
        KLineEdit::mousePressEvent( ev );
}
