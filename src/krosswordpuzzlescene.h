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

#ifndef KROSSWORDPUZZLESCENE_H
#define KROSSWORDPUZZLESCENE_H

#include <QGraphicsScene>

#include "krossword.h"
#include <qevent.h>

class KrossWordPuzzleScene : public QGraphicsScene {
    public:
	explicit KrossWordPuzzleScene( Crossword::KrossWord *krossWord, QObject* parent = 0 )
		    : QGraphicsScene(parent),
		    m_krossWord(krossWord) {
	  addItem( m_krossWord );
	  setItemIndexMethod( NoIndex );
	};

        KrossWordPuzzleScene( QObject* parent = 0 )
		    : QGraphicsScene(parent),
		    m_krossWord(new Crossword::KrossWord()) {
	  addItem( m_krossWord );
	  setItemIndexMethod( NoIndex );
	};

	Crossword::KrossWord *krossWord() { return m_krossWord; };

    private:
	Crossword::KrossWord *m_krossWord;
};

#endif // KROSSWORDPUZZLESCENE_H
