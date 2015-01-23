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


struct DocumentSettings
{
    bool showNotes         = true;
    bool showTitle         = true;
    bool showCopyright     = true;

    bool showCrosswordOnly = false;
    bool showCluesOnly     = false;

    bool showSolutions     = false;
};

class AbstractFormatPolicy
{
public:
    AbstractFormatPolicy();
    virtual ~AbstractFormatPolicy();

    virtual QString getTitle(const QString& title);
    virtual QString getCopyright(const QString& authors, const QString& copyright);
    virtual QString getNotes(const QString& notes);
    virtual QString getCrossword(const KrossWordCellList& cells, uint width, uint height);

    virtual void print(const QString& content) = 0;

    virtual QString drawBlackCell(KrossWordCell& cell) = 0;
    virtual QString drawClueCell(KrossWordCell& cell) = 0;
    virtual QString drawDoubleClueCell(KrossWordCell& cell) = 0;
    virtual QString drawLetterCell(KrossWordCell& cell) = 0;
    virtual QString drawCellNumber(KrossWordCell& cell) = 0;
    virtual QString drawImageCell(KrossWordCell& cell)  = 0;
};

class PdfFormatPolicy : public AbstractFormatPolicy
{
public:
    PdfFormatPolicy(QPrinter& printer);

    void print(const QString& content) override;

    QString drawBlackCell(KrossWordCell& cell) override;
    QString drawClueCell(KrossWordCell& cell) override;
    QString drawDoubleClueCell(KrossWordCell& cell) override;
    QString drawLetterCell(KrossWordCell& cell) override;
    QString drawCellNumber(KrossWordCell& cell) override;
    QString drawImageCell(KrossWordCell& cell)  override;

    const QPrinter& getPrinter() const;

    QString getCrossword(const KrossWordCellList &cells, uint width, uint height);

private:
    QPrinter& m_printer;
    QPainter m_painter;
    const QSize m_size;
    QImage m_image;
};


class HtmlFormatPolicy : public AbstractFormatPolicy
{
};

class Document
{
public:
    Document(KrossWord& crossword, AbstractFormatPolicy& policy, const DocumentSettings& settings);

    void print();

private:
    void setup();

    void makeHeader(bool show);
    void makeTitle(bool add);
    void makeCopyright(bool add);
    void makeNotes(bool add);
    void makeCrossword(bool show);

    void makeClues(bool show);
    void makeCluesTitle();
    void makeCluesList(const ClueCellList& horizontal, const ClueCellList& vertical);

private:
    KrossWord& m_crossword;
    AbstractFormatPolicy& m_policy;
    DocumentSettings m_settings;
    QString m_content;

    const QString m_headerTag     = "<crosswordHeader />";
    const QString m_crosswordTag  = "<crossword />";
    const QString m_cluesTag      = "<crosswordClues />";

    const QString m_titleTag      = "<crosswordTitle />";
    const QString m_copyrightTag  = "<crosswordCopyright />";
    const QString m_notesTag      = "<crosswordNotes />";

    const QString m_cluesTitleTag      = "<crosswordCluesTitle />";
    const QString m_horizontalCluesTag = "<crosswordCluesHor />";
    const QString m_verticalCluesTag   = "<crosswordCluesVer />";
};


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
