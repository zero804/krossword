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
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QPrinter>

#include <QDebug>

#include <KGlobalSettings>

#include <QStyleOptionGraphicsItem>

#include <cmath>


KrossWordDocument::KrossWordDocument(KrossWord *krossWord, QPrinter *printer)
    : m_krossWord(krossWord),
      m_titleDoc(new QTextDocument),
      m_clueListDoc(new QTextDocument),
      m_printer(nullptr),
      m_cellSize(0.0),
      m_titleHeight(0.0),
      m_translation(QPointF())
{
    makeTitlePage();
    makeClueListPage();
    setPrinter(printer);

    computeCellSize();
    computeTitleHeight();
    computeTranslationPoint();
}

KrossWordDocument::~KrossWordDocument()
{
    delete m_titleDoc;
    delete m_clueListDoc;

    m_titleDoc    = nullptr;
    m_clueListDoc = nullptr;
}

void KrossWordDocument::print(int fromPage, int toPage)
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

void KrossWordDocument::renderPage(QPainter *painter, int page)
{
    Q_ASSERT(page >= 1 && page <= pages());

    if (page == 1) {
        m_titleDoc->drawContents(painter);

        foreach(KrossWordCell* cell, m_krossWord->cells()) {
            // Black cells
            if(cell->cellType() == Crossword::CellType::EmptyCellType) {
                painter->setPen(Qt::black);
                painter->setBrush(QBrush(Qt::black, Qt::SolidPattern));
                drawCell(painter, cell->coord());

                /*
                //painter->setPen(Qt::black);
                painter->setBrush(Qt::SolidPattern);
                painter->drawRect((cell->coord().first * cellSize) + cellSize*0.05,
                                  (cell->coord().second * cellSize) + cellSize*0.05,
                                  cellSize - (cellSize*0.1),
                                  cellSize - (cellSize*0.1));
                */
            /*
            // Cell with one clue inside
            } else if(cell->cellType() == Crossword::CellType::ClueCellType) {
                painter->setPen(Qt::green);
                painter->setBrush(Qt::SolidPattern);
                drawCell(painter, cell->coord());

            // Cell with two clues inside
            } else if(cell->cellType() == Crossword::CellType::DoubleClueCellType) {
                painter->setPen(Qt::blue);
                painter->setBrush(Qt::SolidPattern);
                drawCell(painter, cell->coord());

            // Standard letter cell
            */
            } else if(cell->cellType() == Crossword::CellType::LetterCellType) {
                LetterCell* lcell = static_cast<LetterCell*>(cell);
                ClueCell* horiz = lcell->clueHorizontal();
                ClueCell* vert = lcell->clueVertical();

                Coord cellCoord = cell->coord();
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

            /*
            // Cell with two clues inside
            } else if(cell->cellType() == Crossword::CellType::SolutionLetterCellType) {
                painter->setPen(Qt::gray);
                painter->setBrush(Qt::SolidPattern);
                drawCell(painter, cell->coord());

            } else if(cell->cellType() == Crossword::CellType::ImageCellType) {
                painter->setPen(Qt::red);
                painter->setBrush(Qt::SolidPattern);
                drawCell(painter, cell->coord());
            }
            */
        }
        }

    } else {
        painter->setFont(m_clueListDoc->defaultFont());
        QRectF body(QPoint(0, 0), m_clueListDoc->pageSize());

        int clueListPage = page - 2; // clueListPage is zero-based, page isn't
        painter->save();
        painter->translate(body.left(), body.top() - clueListPage * body.height());
        QRectF view(0, clueListPage * body.height(), body.width(), body.height());
        QAbstractTextDocumentLayout *layout = m_clueListDoc->documentLayout();
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

int KrossWordDocument::pages() const
{
    return 1 + m_clueListDoc->pageCount(); // The crossword gets always printed to one page
}

QPrinter *KrossWordDocument::printer() const
{
    return m_printer;
}

void KrossWordDocument::setPrinter(QPrinter *printer)
{
    Q_ASSERT(printer);

    m_titleDoc->setPageSize(printer->pageRect().size());
    m_clueListDoc->setPageSize(printer->pageRect().size());

    m_printer = printer;
}

void KrossWordDocument::makeTitlePage()
{
    QString notes;

    if (!m_krossWord->notes().isEmpty()) {
        notes = QString("<h5>" + m_krossWord->notes() + "</h5>");
    }

    m_titleDoc->setHtml(QString("<html><body>"
                                "<h1><crosswordTitle /></h1>"
                                "<crosswordNotes />"
                                "<table width='100%'><tr>"
                                "<td><crosswordAuthors /></td>"
                                "<td align='right'><crosswordCopyright /></td>"
                                "</tr></table></body>")
                        .replace("<crosswordNotes />", notes)
                        .replace("<crosswordTitle />", m_krossWord->title())
                        .replace("<crosswordAuthors />", m_krossWord->authors())
                        .replace("<crosswordCopyright />", m_krossWord->copyright()));
}

void KrossWordDocument::makeClueListPage()
{
    // Create clue list
    ClueCellList horizontalClues, verticalClues;
    m_krossWord->clues(&horizontalClues, &verticalClues);

    QString clueTableHorizontal = "<table cellspacing='10'>";
    foreach(ClueCell * clue, horizontalClues) {
        clueTableHorizontal += QString("<tr><td>%1</td></tr>").arg(clue->clueWithNumber());
    }

    QString clueTableVertical = "<table cellspacing='10'>";
    foreach(ClueCell * clue, verticalClues) {
        clueTableVertical += QString("<tr><td>%1</td></tr>").arg(clue->clueWithNumber());
    }
    clueTableHorizontal += "</table>";
    clueTableVertical += "</table>";

    m_clueListDoc = new QTextDocument;
    m_clueListDoc->setHtml(QString("<html><body>"
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

void KrossWordDocument::computeTitleHeight() const
{
    const int marginTitle = 15;
    m_titleHeight = m_titleDoc->size().height() + marginTitle;
}

void KrossWordDocument::computeTranslationPoint() const
{
    const int crosswordSize = computeCrosswordSize();
    m_translation = QPointF((m_printer->pageRect().width() - crosswordSize)/2.0,
                            (m_titleHeight + (m_printer->pageRect().height() - 130 - m_titleHeight) - crosswordSize)/2.0);

    qDebug() << m_translation;
}

void KrossWordDocument::computeCellSize() const
{
    const uint w = m_krossWord->width();
    const int crossword_size = computeCrosswordSize();
    m_cellSize = crossword_size / w;
}

int KrossWordDocument::computeCrosswordSize() const
{
    return std::min(m_printer->pageRect().width(),
                    m_printer->pageRect().height() - int(m_titleHeight));
}

void KrossWordDocument::drawCell(QPainter *painter, QPair<int, int> p) const
{

    painter->drawRect(m_translation.x() + p.first * m_cellSize,
                      m_translation.y() + p.second * m_cellSize,
                      m_cellSize,
                      m_cellSize);
}

void KrossWordDocument::drawCellNumbers(QPainter *painter, QPair<int, int> p, int number) const
{
    painter->drawText(QRectF(m_translation.x() + (p.first * m_cellSize) + 1.0,
                             m_translation.y() + (p.second * m_cellSize) + 1.0,
                             m_cellSize,
                             m_cellSize),
                      QString::number(number));
}
