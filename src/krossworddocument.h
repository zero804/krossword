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

#ifndef KROSSWORDDOCUMENT_H
#define KROSSWORDDOCUMENT_H

class KrossWordPuzzleScene;
class KrossWordPuzzleView;

#include <QPair>
#include <QPointF>

namespace Crossword
{
class KrossWord;
}
using namespace Crossword;

class QTextDocument;
class QPrinter;
class QPainter;

class KrossWordDocument
{
public:
    KrossWordDocument(KrossWord *krossWord, QPrinter *printer);
    ~KrossWordDocument();

    void print(int fromPage = 1, int toPage = -1);
    void renderPage(QPainter *painter, int page);
    int pages() const;

    QPrinter *printer() const;
    void setPrinter(QPrinter *printer);

private:
    void makeTitlePage();
    void makeClueListPage();
    void computeTitleHeight() const;
    void computeTranslationPoint() const;
    void computeCellSize() const;
    int computeCrosswordSize() const;
    void drawCell(QPainter *painter, QPair<int, int> p) const;
    void drawCellNumbers(QPainter *painter, QPair<int, int> p, int number) const;

private:
    KrossWord *m_krossWord;
    KrossWordPuzzleScene *m_krossWordScene;
    KrossWordPuzzleView *m_krossWordView;
    QTextDocument *m_titleDoc;
    QTextDocument *m_clueListDoc;
    QPrinter *m_printer;

    mutable float m_cellSize;
    mutable float m_titleHeight;
    mutable QPointF m_translation;
};

#endif // KROSSWORDDOCUMENT_H
