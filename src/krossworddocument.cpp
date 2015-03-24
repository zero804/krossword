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

#include "krossworddocument.h"
#include "krosswordpuzzleview.h"
#include "cells/cluecell.h"

#include <QString>
#include <QAbstractTextDocumentLayout>
#include <QPrinter>

#include <QDebug>
#include <KGlobalSettings>
#include <QStyleOptionGraphicsItem>
#include <cmath>

DocumentLayout::DocumentLayout(KrossWord &crossword)
    : m_crosswordPage(),
      m_cluesPage(),
      m_crossword(crossword)
{
    makeCrosswordPage();
    makeCluesPage();
}

void DocumentLayout::drawCrosswordPage(QPainter *painter)
{
    m_crosswordPage.drawContents(painter);
}

void DocumentLayout::drawCluesPage(QPainter *painter)
{
    m_cluesPage.drawContents(painter);
}

int DocumentLayout::getCluesPagesCount() const
{
    return m_cluesPage.pageCount();
}

QTextDocument& DocumentLayout::getCrosswordPage() {
    return m_crosswordPage;
}

QTextDocument& DocumentLayout::getCluesPage() {
    return m_cluesPage;
}

void DocumentLayout::makeCrosswordPage()
{
    QString notes;
    if (!m_crossword.notes().isEmpty())
        notes = QString("<h5>%1</h5>").arg(m_crossword.notes());

    m_crosswordPage.setHtml(QString("<html><body>"
                                    "<h1><crosswordTitle /></h1>"
                                    "<crosswordNotes />"
                                    "<table width='100%'><tr>"
                                    "<td><crosswordAuthors /></td>"
                                    "<td align='right'><crosswordCopyright /></td>"
                                    "</tr></table></body>")
                            .replace("<crosswordNotes />",     notes)
                            .replace("<crosswordTitle />",     m_crossword.title())
                            .replace("<crosswordAuthors />",   m_crossword.authors())
                            .replace("<crosswordCopyright />", m_crossword.copyright()));
}

void DocumentLayout::makeCluesPage()
{
    // Create clue list
    ClueCellList horizontalClues, verticalClues;
    m_crossword.clues(&horizontalClues, &verticalClues);

    QString clueTableHorizontal = "<table cellspacing='10'>";
    foreach(ClueCell * clue, horizontalClues) {
        clueTableHorizontal += QString("<tr><td>%1</td></tr>").arg(clue->clueWithNumber());
    }

    QString clueTableVertical = "<table cellspacing='10'>";
    foreach(ClueCell * clue, verticalClues) {
        clueTableVertical += QString("<tr><td>%1</td></tr>").arg(clue->clueWithNumber());
    }
    clueTableHorizontal += "</table>";
    clueTableVertical   += "</table>";

    m_cluesPage.setHtml(QString("<html><body>"
                                   "<center><h1><clueListTitle /></h1></center><br>"
                                   "<table><tr><td>"
                                   "<h2><clueListHorizontalTitle /></h2>"
                                   "<clueTableHorizontal />"
                                   "</td><td>"
                                   "<h2><clueListVerticalTitle /></h2>"
                                   "<clueTableVertical />"
                                   "</td></tr></table>"
                                   "</body></html>")
                           .replace("<clueListTitle />", i18nc("Title for the clue list when printing", "Clue list"))
                           .replace("<clueListVerticalTitle />", i18nc("Title for the list of down/vertical clues when printing", "Down clues"))
                           .replace("<clueListHorizontalTitle />", i18nc("Title for the list of across/horizontal clues when printing", "Across clues"))
                           .replace("<clueTableVertical />", clueTableVertical)
                        .replace("<clueTableHorizontal />", clueTableHorizontal));
}

//==================================================

AbstractKrossWordDocument::AbstractKrossWordDocument(KrossWord *krossWord)
    : m_krossWord(krossWord)
{ }

KrossWord *AbstractKrossWordDocument::getCrossword() const
{
    return m_krossWord;
}

//==================================================

PdfDocument::PdfDocument(KrossWord *krossWord, QPrinter *printer)
    : AbstractKrossWordDocument(krossWord),
      m_docLayout(*krossWord),
      m_printer(nullptr),
      m_cellSize(0.0),
      m_titleHeight(0.0),
      m_translation(QPointF())
{
    setPrinter(printer);

    computeCellSize();
    computeTitleHeight();
    computeTranslationPoint();
}

void PdfDocument::print(int fromPage, int toPage)
{
    int pageCount = pages();
    if (toPage < 1)
        toPage = pageCount;

    fromPage = qBound<int>(1, fromPage, pageCount);
    toPage = qBound<int>(1, toPage, pageCount);
    if (toPage < fromPage)
        toPage = fromPage;

    QPainter painter;
    painter.begin(m_printer);

    if (m_printer->pageOrder() == QPrinter::FirstPageFirst) {
        for (int page = fromPage; page <= toPage; ++page) {
            renderPage(&painter, page);
            if (page < toPage)
                m_printer->newPage();
        }
    } else {
        for (int page = toPage; page >= fromPage; --page) {
            renderPage(&painter, page);
            if (page > fromPage)
                m_printer->newPage();
        }
    }

    painter.end();
}

void PdfDocument::renderPage(QPainter *painter, int page)
{
    Q_ASSERT(page >= 1 && page <= pages());

    if (page == 1) {
        m_docLayout.drawCrosswordPage(painter);

        foreach(KrossWordCell* cell, getCrossword()->cells()) {
            switch (cell->cellType()) {
            case Crossword::CellType::EmptyCellType:            // Black cell
                drawEmptyCell(painter, cell->coord());
                break;
            case Crossword::CellType::ClueCellType:             // Cell with one clue inside
                //drawClueCell(painter, cell->coord());
                break;
            case Crossword::CellType::DoubleClueCellType:       // Cell with two clues inside
                //drawDoubleClueCell(painter, cell->coord());
                break;
            case Crossword::CellType::LetterCellType:
                drawLetterCell(painter, *cell);
                break;
            case Crossword::CellType::SolutionLetterCellType:
                //drawSolutionLetterCell(painter, cell->coord());
                break;
            case Crossword::CellType::ImageCellType:
                drawImageCellCell(painter, cell->coord());
                break;
            default:
                break;
            }
        }

    } else {
        painter->setFont(m_docLayout.getCluesPage().defaultFont());
        QRectF body(QPoint(0, 0), m_docLayout.getCluesPage().pageSize());

        int clueListPage = page - 2; // clueListPage is zero-based, page isn't
        painter->save();
        painter->translate(body.left(), body.top() - clueListPage * body.height());
        QRectF view(0, clueListPage * body.height(), body.width(), body.height());
        QAbstractTextDocumentLayout *layout = m_docLayout.getCluesPage().documentLayout();
        QAbstractTextDocumentLayout::PaintContext ctx;

        ctx.clip = view;

        // don't use the system palette text as default text color, on HP/UX
        // for example that's white, and white text on white paper doesn't
        // look that nice
        ctx.palette.setColor(QPalette::Text, Qt::black);
        layout->draw(painter, ctx);
        painter->restore();
    }
}

int PdfDocument::pages() const
{
    return 1 + m_docLayout.getCluesPagesCount(); // The crossword gets always printed to one page
}

QPrinter *PdfDocument::getPrinter() const
{
    return m_printer;
}

void PdfDocument::setPrinter(QPrinter *printer)
{
    Q_ASSERT(printer);

    m_docLayout.getCrosswordPage().setPageSize(printer->pageRect().size());
    m_docLayout.getCluesPage().setPageSize(printer->pageRect().size());

    m_printer = printer;
}

void PdfDocument::computeTitleHeight()
{
    const int marginTitle = 15;
    m_titleHeight = m_docLayout.getCrosswordPage().size().height() + marginTitle;
}

void PdfDocument::computeTranslationPoint()
{
    const QPoint canvas = QPoint(m_printer->pageRect().width(), m_printer->pageRect().height());
    const float slack_w = canvas.x() - m_cellSize * getCrossword()->width();
    const float slack_h = canvas.y() - m_cellSize * getCrossword()->height();

    /*NOTE: titleHeight is useful in some way? */
    m_translation = QPointF(canvas.x() - (canvas.x() - (slack_w/2.0)),
                            //m_titleHeight + (m_printer->pageRect().height() - 130 - m_titleHeight) - cvSize/2.0);
                            canvas.y() - (canvas.y() - (slack_h/2.0)));

    qDebug() << m_translation;
}

void PdfDocument::computeCellSize()
{
    const uint w = getCrossword()->width();
    const uint h = getCrossword()->height();
    const int cvSize = computeCanvasSize();
    m_cellSize = std::min(cvSize / w, cvSize / h);
}

int PdfDocument::computeCanvasSize() const
{
    return std::min(m_printer->pageRect().width(),
                    m_printer->pageRect().height() - int(m_titleHeight));
}

void PdfDocument::drawCell(QPainter *painter, QPair<int, int> p) const
{

    painter->drawRect(m_translation.x() + p.first * m_cellSize,
                      m_translation.y() + p.second * m_cellSize,
                      m_cellSize,
                      m_cellSize);
}

void PdfDocument::drawCellNumbers(QPainter *painter, QPair<int, int> p, int number) const
{
    painter->drawText(QRectF(m_translation.x() + (p.first * m_cellSize) + 1.0,
                             m_translation.y() + (p.second * m_cellSize) + 1.0,
                             m_cellSize,
                             m_cellSize),
                      QString::number(number));
}

void PdfDocument::drawEmptyCell(QPainter *painter, Coord cellCoord)
{
    painter->setPen(Qt::black);
    painter->setBrush(QBrush(Qt::black, Qt::NoBrush));
    drawCell(painter, cellCoord);
    const float border = 2.0;

    painter->setBrush(QBrush(Qt::black, Qt::SolidPattern));
    painter->drawRect(m_translation.x() + cellCoord.first  * m_cellSize + border,
                      m_translation.y() + cellCoord.second * m_cellSize + border,
                      m_cellSize - 2*border,
                      m_cellSize - 2*border);

}

void PdfDocument::drawClueCell(QPainter *painter, Coord cellCoord)
{
    //TODO: implement this
    painter->setPen(Qt::green);
    painter->setBrush(QBrush(Qt::black, Qt::SolidPattern));
    drawCell(painter, cellCoord);
}

void PdfDocument::drawDoubleClueCell(QPainter *painter, Coord cellCoord)
{
    //TODO: implement this
    painter->setPen(Qt::red);
    painter->setBrush(QBrush(Qt::black, Qt::SolidPattern));
    drawCell(painter, cellCoord);
}

void PdfDocument::drawLetterCell(QPainter *painter, KrossWordCell& cell)
{
    LetterCell* lcell = static_cast<LetterCell*>(&cell);
    ClueCell* horiz = lcell->clueHorizontal();
    ClueCell* vert = lcell->clueVertical();

    Coord cellCoord = cell.coord();
    Coord horClueCoord;
    Coord verClueCoord;

    int clueNumber = -1;
    int hclueNumber = -1;
    int vclueNumber = -1;

    if (horiz != nullptr) {
        horClueCoord = horiz->coord();
        hclueNumber = horiz->clueNumber();
    }

    if (vert != nullptr) {
        verClueCoord = vert->coord();
        vclueNumber = vert->clueNumber();
    }


    if (horClueCoord == cellCoord) {
        clueNumber = hclueNumber;
    } else if (verClueCoord == cellCoord) {
        clueNumber = vclueNumber;
    }

    painter->setPen(Qt::black);
    painter->setBrush(QBrush(Qt::white, Qt::SolidPattern));
    drawCell(painter, cellCoord);

    if (clueNumber != -1) {
        clueNumber += 1;

        QFont font = KGlobalSettings::smallestReadableFont();
        qreal levelOfDetail = QStyleOptionGraphicsItem::levelOfDetailFromTransform(QTransform(painter->matrix()));
        font.setPointSizeF(7.0 * levelOfDetail);

        painter->setFont(font);
        drawCellNumbers(painter, cellCoord, clueNumber);
    }
}

void PdfDocument::drawSolutionLetterCell(QPainter *painter, Coord cellCoord)
{
    //TODO: implement this
    painter->setPen(Qt::gray);
    painter->setBrush(QBrush(Qt::black, Qt::SolidPattern));
    drawCell(painter, cellCoord);
}

void PdfDocument::drawImageCellCell(QPainter *painter, Coord cellCoord)
{
    //TODO: implement this
    painter->setPen(Qt::blue);
    painter->setBrush(QBrush(Qt::black, Qt::SolidPattern));
    drawCell(painter, cellCoord);
}
