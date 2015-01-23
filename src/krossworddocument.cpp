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


AbstractFormatPolicy::AbstractFormatPolicy()
{ }

AbstractFormatPolicy::~AbstractFormatPolicy()
{ }

QString AbstractFormatPolicy::getTitle(const QString &title) {
    return "<h1>" + title + "</h1>";
}

QString AbstractFormatPolicy::getCopyright(const QString &authors, const QString &copyright) {
    return "<table width='100%'><tr><td>" + authors +
            "</td><td align='right'>" + copyright + "</td></tr></table>";
}

QString AbstractFormatPolicy::getNotes(const QString &notes) {
    return notes;
}

QString AbstractFormatPolicy::getCrossword(const KrossWordCellList &cells, uint width, uint height) {
    QString htmlcells;
    foreach(KrossWordCell* cell, cells) {

        switch(cell->cellType()) {
        case Crossword::CellType::EmptyCellType:          //Black cells
            htmlcells += drawBlackCell(*cell);
            break;
        case Crossword::CellType::ClueCellType:           //Cell with one clue inside
            htmlcells += drawClueCell(*cell);
            break;
        case Crossword::CellType::DoubleClueCellType:     //Cell with two clues inside
            htmlcells += drawDoubleClueCell(*cell);
            break;
        case Crossword::CellType::LetterCellType:         //Standard letter cell
            htmlcells += drawLetterCell(*cell);
            break;
        case Crossword::CellType::SolutionLetterCellType: //Solution cell?
            //htmlcells += drawSolutionCell(*cell);
            break;
        case Crossword::CellType::ImageCellType:          //Image cells
            htmlcells += drawImageCell(*cell);
            break;
        default:
            break;
        }
    }

    return htmlcells;
}

//===============

PdfFormatPolicy::PdfFormatPolicy(QPrinter &printer)
    : AbstractFormatPolicy(),
      m_printer(printer),
      m_painter(),
      m_size(m_printer.pageRect().width(), m_printer.pageRect().width()),
      m_image(m_size, QImage::Format_RGB888)
{ }

void PdfFormatPolicy::print(const QString &content)
{
    QTextDocument doc;
    doc.setHtml(content);

    QPainter painter;
    painter.begin(&m_printer);

    painter.drawImage(0, 0, m_image);

    doc.drawContents(&painter);

    painter.end();
}

QString PdfFormatPolicy::getCrossword(const KrossWordCellList &cells, uint width, uint height) {
    m_painter.begin(&m_image);
    m_painter.fillRect(QRectF(0, 0, m_size.width(), m_size.height()), Qt::white);

    float cellSize = m_size.width()/width;

    foreach(KrossWordCell* cell, cells) {
        Coord cellCoord = cell->coord();
        //m_painter.translate(cellCoord.first * cellSize, cellCoord.second * cellSize);

        switch(cell->cellType()) {
        case Crossword::CellType::EmptyCellType:          //Black cells
            drawBlackCell(*cell);
            m_painter.drawRect(cellCoord.first * cellSize,
                               cellCoord.second * cellSize,
                               cellSize,
                               cellSize);

            break;
        case Crossword::CellType::ClueCellType:           //Cell with one clue inside
            drawClueCell(*cell);
            break;
        case Crossword::CellType::DoubleClueCellType:     //Cell with two clues inside
            drawDoubleClueCell(*cell);
            break;
        case Crossword::CellType::LetterCellType:         //Standard letter cell
            drawLetterCell(*cell);
            break;
        case Crossword::CellType::SolutionLetterCellType: //Solution cell?
            //htmlcells += drawSolutionCell(*cell);
            break;
        case Crossword::CellType::ImageCellType:          //Image cells
            drawImageCell(*cell);
            break;
        default:
            break;
        }

        //m_painter.translate(-cellCoord.first * cellSize, -cellCoord.second * cellSize);
    }

    m_painter.end();

    return "";
}

QString PdfFormatPolicy::drawBlackCell(KrossWordCell &cell)
{
    m_painter.setPen(Qt::black);
    m_painter.setBrush(QBrush(Qt::black, Qt::SolidPattern));

    return "";
}

QString PdfFormatPolicy::drawClueCell(KrossWordCell &cell)
{
    return "";
}

QString PdfFormatPolicy::drawDoubleClueCell(KrossWordCell& cell)
{
    return "";
}

QString PdfFormatPolicy::drawLetterCell(KrossWordCell &cell)
{
    return "";
}

QString PdfFormatPolicy::drawCellNumber(KrossWordCell &cell)
{
    return "";
}

QString PdfFormatPolicy::drawImageCell(KrossWordCell& cell)
{
    return "";
}

const QPrinter &PdfFormatPolicy::getPrinter() const
{
    return m_printer;
}

//======================

Document::Document(KrossWord &crossword, AbstractFormatPolicy &policy, const DocumentSettings &settings)
    : m_crossword(crossword),
      m_policy(policy),
      m_settings(settings),
      m_content()
{
    setup();
}

void Document::print() {
    m_policy.print(m_content);
}

void Document::setup() {
    m_content = QString("<html><body>") + m_headerTag + m_crosswordTag + m_cluesTag + QString("</body></html>");


    bool show_crossword = m_settings.showCrosswordOnly;
    bool show_clues     = m_settings.showCluesOnly;

    if (!show_crossword && !show_clues) {
        show_crossword = true;
        show_clues = true;
    }

    makeHeader(show_crossword);
    makeCrossword(show_crossword);
    makeClues(show_clues);
}

void Document::makeHeader(bool show) {
    if (show) {
        m_content.replace(m_headerTag, m_titleTag + m_notesTag + m_copyrightTag);

        makeTitle(m_settings.showTitle);
        makeCopyright(m_settings.showCopyright);
        makeNotes(m_settings.showNotes);
    } else {
        m_content.replace(m_headerTag, "");
    }
}

void Document::makeTitle(bool add) {
    if (add)
        m_content.replace(m_titleTag, m_policy.getTitle(m_crossword.title()));
    else
        m_content.replace(m_titleTag, "");
}

void Document::makeCopyright(bool add) {
    if (add)
        m_content.replace(m_copyrightTag, m_policy.getCopyright(m_crossword.authors(), m_crossword.copyright()));
    else
        m_content.replace(m_copyrightTag, "");
}

void Document::makeNotes(bool add) {
    if (add)
        m_content.replace(m_notesTag, m_policy.getNotes(m_crossword.notes()));
    else
        m_content.replace(m_notesTag, "");
}

void Document::makeCrossword(bool show)
{
    if (show) {
        m_content.replace(m_crosswordTag, m_policy.getCrossword(m_crossword.cells(), m_crossword.width(), m_crossword.height()));
    } else {
        m_content.replace(m_crosswordTag, "");
    }
}

void Document::makeClues(bool show) {
    if (show) {
        ClueCellList horizontalClues, verticalClues;
        m_crossword.clues(&horizontalClues, &verticalClues);

        m_content.replace(m_cluesTag, m_cluesTitleTag + m_horizontalCluesTag + m_verticalCluesTag);

        makeCluesTitle();
        makeCluesList(horizontalClues, verticalClues);

    } else {
        m_content.replace(m_cluesTag, "");
    }
}

void Document::makeCluesTitle()
{
    m_content.replace(m_cluesTitleTag, "<center><h1>" + i18nc("Title for the clue list when printing", "Clue list") + "</h1></center><br>");
}

void Document::makeCluesList(const ClueCellList& horizontal, const ClueCellList& vertical)
{
    QString clueTableHorizontal = "<table cellspacing='10'>";
    foreach(ClueCell * clue, horizontal) {
        clueTableHorizontal += QString("<tr><td>%1</td></tr>").arg(clue->clueWithNumber());
    }

    QString clueTableVertical = "<table cellspacing='10'>";
    foreach(ClueCell * clue, vertical) {
        clueTableVertical += QString("<tr><td>%1</td></tr>").arg(clue->clueWithNumber());
    }

    m_content.replace(m_horizontalCluesTag, QString("<table><tr><td>"
                                                    "<h2>%1</h2>"
                                                    "%2</td>"
                                                    "<td><h2>%3</h2>"
                                                    "%4</div>"
                                                    "</td></tr></table>").arg(i18nc("Title for the list of across/horizontal clues when printing", "Across clues"),
                                                                              clueTableHorizontal,
                                                                              i18nc("Title for the list of down/vertical clues when printing", "Down clues"),
                                                                              clueTableVertical));
}

//================================================================


DocumentLayout::DocumentLayout(KrossWord &crossword)
    : m_crosswordPage(),
      m_cluesPage(),
      m_crossword(crossword),
      m_relativeCellSize(0.0)
{
    makeCrosswordPage();
    makeCluesPage();

    computeRelativeCellSize();
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

float DocumentLayout::getRelativeCellSize() const
{
    return m_relativeCellSize;
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

void DocumentLayout::computeRelativeCellSize()
{
    const uint w = m_crossword.width();
    m_relativeCellSize = 100.0 / w;
}

//==================================================

KrossWordDocument::KrossWordDocument(KrossWord *krossWord, QPrinter *printer)
    : m_krossWord(krossWord),
      m_docLayout(*m_krossWord),
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

KrossWordDocument::~KrossWordDocument()
{ }

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
        m_docLayout.drawCrosswordPage(painter);

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

int KrossWordDocument::pages() const
{
    return 1 + m_docLayout.getCluesPagesCount(); // The crossword gets always printed to one page
}

QPrinter *KrossWordDocument::printer() const
{
    return m_printer;
}

void KrossWordDocument::setPrinter(QPrinter *printer)
{
    Q_ASSERT(printer);

    m_docLayout.getCrosswordPage().setPageSize(printer->pageRect().size());
    m_docLayout.getCluesPage().setPageSize(printer->pageRect().size());

    m_printer = printer;
}

void KrossWordDocument::computeTitleHeight()
{
    const int marginTitle = 15;
    m_titleHeight = m_docLayout.getCrosswordPage().size().height() + marginTitle;
}

void KrossWordDocument::computeTranslationPoint()
{
    const int crosswordSize = computeCrosswordSize();
    m_translation = QPointF((m_printer->pageRect().width() - crosswordSize)/2.0,
                            (m_titleHeight + (m_printer->pageRect().height() - 130 - m_titleHeight) - crosswordSize)/2.0);

    qDebug() << m_translation;
}

void KrossWordDocument::computeCellSize()
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
