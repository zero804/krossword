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
#include <QTextDocument>

#include <QPainter>
#include <type_traits>

#include "krossword.h"

namespace Crossword
{
    class KrossWord;
}

using namespace Crossword;

class QPrinter;


class DocumentLayout
{
public:
    DocumentLayout(KrossWord& crossword);

    void drawCrosswordPage(QPainter* painter);
    void drawCluesPage(QPainter* painter);

    int getCluesPagesCount() const;

    float getRelativeCellSize() const;

    QTextDocument& getCrosswordPage();
    QTextDocument& getCluesPage();

private:
    void makeCrosswordPage();
    void makeCluesPage();

    void computeRelativeCellSize();

private:
    QTextDocument m_crosswordPage;
    QTextDocument m_cluesPage;
    KrossWord&    m_crossword;

    float m_titleHeight;
    QPointF m_translation;
};

class AbstractKrossWordDocument
{
public:
    AbstractKrossWordDocument(KrossWord *krossWord);
    virtual ~AbstractKrossWordDocument() = default;

    KrossWord *getCrossword() const;
    virtual void print(int fromPage = 1, int toPage = -1) = 0;
    virtual int pages() const = 0;

private:
     KrossWord *m_krossWord;
};

class PdfDocument : public AbstractKrossWordDocument
{
public:
    PdfDocument(KrossWord *krossWord, QPrinter *printer);

    void print(int fromPage = 1, int toPage = -1) override;
    int pages() const override;
    void renderPage(QPainter *painter, int page);

    QPrinter *getPrinter() const;
    void setPrinter(QPrinter *printer);

private:
    void computeTitleHeight();
    void computeTranslationPoint();
    void computeCellSize();
    int computeCanvasSize() const;
    void drawCell(QPainter *painter, QPair<int, int> p) const;
    void drawCellNumbers(QPainter *painter, QPair<int, int> p, int number) const;

    void drawEmptyCell(QPainter *painter, Coord cellCoord);
    void drawClueCell(QPainter *painter, Coord cellCoord);
    void drawDoubleClueCell(QPainter *painter, Coord cellCoord);
    void drawLetterCell(QPainter *painter, KrossWordCell& cell);
    void drawSolutionLetterCell(QPainter *painter, Coord cellCoord);
    void drawImageCellCell(QPainter *painter, Coord cellCoord);


private:
    DocumentLayout m_docLayout;
    QPrinter *m_printer;

    float m_cellSize;
    float m_titleHeight;
    QPointF m_translation;
};

#endif // KROSSWORDDOCUMENT_H
