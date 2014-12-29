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

#include <type_traits>

namespace Crossword
{
    class KrossWord;
}
using namespace Crossword;

class QPrinter;
class QPainter;



struct DocumentSettings
{
    bool showNotes     = true;
    bool showTitle     = true;
    bool showCopyright = true;
};

class AbstractFormatPolicy
{
public:
    AbstractFormatPolicy()
    { }

    virtual void drawLetterCell() = 0;
    virtual void drawCellNumber() = 0;
    virtual void drawImageCell()  = 0;
};

class PdfFormatPolicy : public AbstractFormatPolicy
{
public:
    PdfFormatPolicy(QPrinter& printer, const PolicySettings& settings)
        : m_printer(printer),
          AbstractFormatPolicy(settings)
    { }

    void drawLetterCell() override;
    void drawCellNumber() override;
    void drawImageCell()  override;

private:
    QPrinter& m_printer;
};


class HtmlFormatPolicy : public AbstractFormatPolicy
{
};

class Document
{
public:
    Document(KrossWord& crossword, AbstractFormatPolicy& policy, const DocumentSettings& settings)
        : m_crossword(crossword),
          m_policy(policy),
          m_settings(settings),
          m_content()
    {
        setup();
    }

    void print() {
        printHeader();
    }

private:
    void setup() {
        m_content = QString("<html><body>") + m_headerTag + m_cluesTag + QString("</body></html>");

        makeHeader();
        makeCrossword();
        makeClues(horizontalClues, verticalClues);
    }

    void makeHeader() {
        m_content.replace(m_headerTag, m_titleTag + m_notesTag + m_copyrightTag);

        makeTitle(m_settings.showTitle);
        makeCopyright(m_settings.showCopyright);
        makeNotes(m_settings.showNotes);
    }

    void makeTitle(bool add) {
        if (add)
            m_content.replace(m_titleTag, "<h1>" + m_crossword.title() + "</h1>");
        else
            m_content.replace(m_titleTag, "");
    }

    void makeCopyright(bool add) {
        if (add)
            m_content.replace(m_copyrightTag, "<table width='100%'><tr><td>" +
                                              m_crossword.authors() + "</td><td align='right'>" +
                                              m_crossword.copyright() + "</td></tr></table>");
        else
            m_content.replace(m_copyrightTag, "");
    }

    void makeNotes(bool add) {
        if (add)
            m_content.replace(m_notesTag, m_crossword.notes());
        else
            m_content.replace(m_notesTag, "");
    }

    void makeCrossword() {

    }

    void makeClues() {
        m_content.replace(m_cluesTag, "TODO");

        ClueCellList horizontalClues, verticalClues;
        m_crossword.clues(&horizontalClues, &verticalClues);
    }

private:
    KrossWord& m_crossword;
    AbstractFormatPolicy& m_policy;
    DocumentSettings m_settings;
    QString m_content;

    const QString m_headerTag    = "<crosswordHeader />";
    const QString m_cluesTag     = "<crosswordClues />";

    const QString m_titleTag     = "<crosswordTitle />";
    const QString m_copyrightTag = "<crosswordCopyright />";
    const QString m_notesTag     = "<crosswordNotes />";
};

/*
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

    float m_relativeCellSize;
    float m_titleHeight;
    QPointF m_translation;
};
*/


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
    void computeTitleHeight();
    void computeTranslationPoint();
    void computeCellSize();
    int computeCrosswordSize() const;
    void drawCell(QPainter *painter, QPair<int, int> p) const;
    void drawCellNumbers(QPainter *painter, QPair<int, int> p, int number) const;

private:
    KrossWord *m_krossWord;
    DocumentLayout m_docLayout;
    QPrinter *m_printer;

    float m_cellSize;
    float m_titleHeight;
    QPointF m_translation;
};

#endif // KROSSWORDDOCUMENT_H
