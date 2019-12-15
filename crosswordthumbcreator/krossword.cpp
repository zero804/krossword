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

#include "krossword.h"
#include "cells/krosswordcell.h"
#include "cells/imagecell.h"
#include "krosswordpuzreader.h"
#include "krosswordxmlreader.h"

#include <QtWidgets/QGraphicsScene>
#include <QFile>

#include <QUrl>
//#include <QMimeType>
#include <qfileinfo.h>
#include <QDebug>
#include <QRgb>
#include <QMimeDatabase>


KrossWord::KrossWord(int width, int height)
    : QGraphicsObject(nullptr)
{
    init(width, height);
    fillWithEmptyCells();
}

void KrossWord::init(uint width, uint height)
{
    m_krossWordGrid = new Grid2D::Generic<KrossWordCell*>(width, height);
    m_emptyCellColorForPrinting = Qt::black;
    m_cellSize = QSizeF(50, 50);
}

KrossWord::~KrossWord()
{
//     qDebug() << "Cleaning crossword";
    deleteAllCells(); // Cells are delete'd by QGraphicsScene only if they are in a QGraphicsScene
}

QRectF KrossWord::boundingRect() const
{
    return QRectF(pos(), QSizeF(cellSize().width() * width(), cellSize().height() * height()));
}

void KrossWord::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->fillRect(option->rect, Qt::white);
}

/*
void KrossWord::resizeTo(const QSizeF& size)
{
    // TODO When called twice with the same parameter, it will scale twice...
    QSizeF actualSize = boundingRect().size();
    float zoomH = size.width() / actualSize.width();
    float zoomV = size.height() / actualSize.height();
    float zoom = zoomH < zoomV ? zoomH : zoomV;
    scale(zoom, zoom);
}
*/

KrossWord::ErrorType KrossWord::insertImage(const Grid2D::Coord &coord,
        int horizontalCellSpan, int verticalCellSpan, QUrl url,
        ErrorTypes errorTypesToIgnore, ImageCell **insertedImage)
{
    ErrorType errorType = canInsertImage(coord, horizontalCellSpan,
                                         verticalCellSpan, errorTypesToIgnore);
    if (errorType != NoError)
        return errorType;

    ImageCell *imageCell = new ImageCell(this, coord, horizontalCellSpan,
                                         verticalCellSpan, url);
    for (int x = imageCell->coord().first;
            x < imageCell->coord().first + imageCell->horizontalCellSpan(); ++x) {
        for (int y = imageCell->coord().second;
                y < imageCell->coord().second + imageCell->verticalCellSpan(); ++y) {
            replaceCell(Coord(x, y), imageCell);
        }
    }

    if (insertedImage)
        *insertedImage = imageCell;
//     (*m_krossWordGrid->at( imageCell->coord() )) = imageCell;
    return NoError;
}

void KrossWord::assignClueNumbers()
{
    int curClueNumber = 0;

    //  Iterate through all cells
    for (uint y = 0; y < height(); ++y) {
        for (uint x = 0; x < width(); ++x) {
            LetterCell *letter = qgraphicsitem_cast<LetterCell*>(at(Coord(x, y)));

            if (!letter) {
                ClueCell *clue = qgraphicsitem_cast<ClueCell*>(at(Coord(x, y)));
                if (clue)
                    clue->setClueNumber(curClueNumber++);
            } else {
                bool assignedNumber = false;
                if (letter->clueHorizontal() && letter->clueHorizontal()->firstLetter() == letter
                        && letter->clueHorizontal()->answerOffset() == ClueCell::OnClueCell) {
                    letter->clueHorizontal()->setClueNumber(curClueNumber);
                    assignedNumber = true;
                }
                if (letter->clueVertical() && letter->clueVertical()->firstLetter() == letter
                        && letter->clueVertical()->answerOffset() == ClueCell::OnClueCell) {
                    letter->clueVertical()->setClueNumber(curClueNumber);
                    assignedNumber = true;
                }

                if (assignedNumber)
                    ++curClueNumber;
            }
        } // for x
    } // for y
}

ClueCell* KrossWord::findClueCell(const Coord& coord,
                                  Qt::Orientation orientation) const
{
    KrossWordCell *cell = at(coord);

    if (cell->isLetterCell())   // For hidden clues
        return ((LetterCell*)cell)->clue(orientation);
    else
        return qgraphicsitem_cast<ClueCell*>(cell);
}

KrossWord::FileFormat KrossWord::fileFormatFromFileName(const QString& fileName)
{
    QMimeDatabase db;
    QString extension = db.suffixForFileName(fileName);
    if (extension == "xml" || extension == "kwp")
        return KrossWordPuzzleXmlFile;
    else if (extension == "kwpz")
        return KrossWordPuzzleCompressedXmlFile;
    else if (extension == "puz")
        return AcrossLitePuzFile;
    else
        return DetermineByFileName; // couldn't determine file format
}

bool KrossWord::read(const QUrl &url, QString *errorString, FileFormat fileFormat)
{
    // Variable not used
    // bool removeTempFile;
    QString fileName;
    if (url.isLocalFile()) {
        fileName = url.path();
        // removeTempFile = false;
    } else {
        *errorString = i18n("Error while downloading from url");
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorString != nullptr)
            *errorString = file.errorString();
        qWarning() << file.errorString();
        return false;
    }

    removeAllCells();

    bool readOk = false, fileFormatDeterminationFailed = false;
    if (fileFormat == DetermineByFileName) {
        QString extension = QFileInfo(fileName).suffix();
        if (extension == "puz")
            fileFormat = AcrossLitePuzFile;
        else if (extension == "xml" || extension == "kwp")
            fileFormat = KrossWordPuzzleXmlFile;
        else if (extension == "kwpz")
            fileFormat = KrossWordPuzzleCompressedXmlFile;
        else {
            fileFormatDeterminationFailed = true;

            // Cycle through the available readers
            KrossWordPuzStream puzReader;
            readOk = puzReader.read(&file, this);

            if (!readOk) {
                if (file.isOpen()) file.seek(0);
                KrossWordXmlReader xmlReader;
                readOk = xmlReader.read(&file, this);

                if (!readOk) {
                    if (file.isOpen()) file.seek(0);
                    readOk = xmlReader.readCompressed(&file, this);
                }
            }

            if (!readOk && errorString)
                *errorString = "File format unknown";
        }
    }

    if (!fileFormatDeterminationFailed) {
        if (fileFormat == AcrossLitePuzFile) {
            KrossWordPuzStream puzReader;
            readOk = puzReader.read(&file, this);
            if (!readOk && errorString)
                *errorString = i18n("Error reading AcrossLite's .puz-format.");
        } else if (fileFormat == KrossWordPuzzleXmlFile) {
            KrossWordXmlReader xmlReader;
            readOk = xmlReader.read(&file, this);
            if (!readOk && errorString)
                *errorString = xmlReader.errorString();
        } else if (fileFormat == KrossWordPuzzleCompressedXmlFile) {
            KrossWordXmlReader xmlReader;
            readOk = xmlReader.readCompressed(&file, this);
            if (!readOk && errorString)
                *errorString = xmlReader.errorString();
        }
    }
    file.close();

    return readOk;
}

QImage KrossWord::toImage(const QSize& size)
{
    // Adjust size to fit the crossword
    QRectF rc = boundingRect();
    qreal ratioCrossword = (qreal)rc.width() / (qreal)rc.height();
    qreal ratioSize = (qreal)size.width() / (qreal)size.height();
    QSize usedSize = size;

    if (ratioCrossword > ratioSize)
        usedSize.rheight() = (qreal)usedSize.width() / ratioCrossword;
    else
        usedSize.rwidth() = (qreal)usedSize.height() * ratioCrossword;

    if (usedSize.isNull())
        usedSize = QSize(5, 5);   // Minimal size

    QImage img(size, QImage::Format_ARGB32);
    img.fill(0x00ffffff);

    qDebug() << "Creating QPainter object";
    QPainter p(&img);
    p.setRenderHints(QPainter::HighQualityAntialiasing | QPainter::Antialiasing
                     | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

    QStyleOptionGraphicsItem *option = new QStyleOptionGraphicsItem();
    option->rect = QRect(0, 0, usedSize.width(), usedSize.height());
    paint(&p, option);

//     qDebug() << "Paint cells";
//     qDebug() << "targetCellSize" << cellSize().width() << "/" << boundingRect().width() << "*" << usedSize.width();
    QSize targetCellSize((cellSize().width() / boundingRect().width()) * (float)usedSize.width(),
                         (cellSize().height() / boundingRect().height()) * (float)usedSize.height());
    qreal xMult = targetCellSize.width();
    qreal yMult = targetCellSize.height();

    foreach(QGraphicsItem * item, childItems()) {
//  qDebug() << "xPos" << item->pos().x() << "*" << usedSize.width() << "/" << boundingRect().width();
        KrossWordCell *cell = dynamic_cast<KrossWordCell*>(item);
        if (SpannedCell *spannedCell = dynamic_cast<SpannedCell*>(cell))
            option->rect = QRect(spannedCell->coord().first * xMult,
                                 spannedCell->coord().second * yMult,
                                 targetCellSize.width() * spannedCell->horizontalCellSpan() + 1,
                                 targetCellSize.height() * spannedCell->verticalCellSpan() + 1);
        else
            option->rect = QRect(cell->coord().first * xMult, cell->coord().second * yMult,
                                 targetCellSize.width() + 1, targetCellSize.height() + 1);
//  qDebug() << "Paint cell" << KrossWordCell::cellTypeToString( cell->cellType() );
//  qDebug() << "Draw rect" << option->rect;

        cell->paint(&p, option);
    }

    p.end();

    return img;
}

KrossWordCellList KrossWord::cells(KrossWordCell::CellTypes cellTypes) const
{
    KrossWordCellList list;
    for (uint x = 0; x < width(); ++x) {
        for (uint y = 0; y < height(); ++y) {
            KrossWordCell *cell = at(Coord(x, y));
            if (cellTypes.testFlag(cell->cellType()) && !list.contains(cell))
                list << cell;
        }
    }
    return list;
}

KrossWordCellList KrossWord::cells(const Coord& coord,
                                   Qt::Orientation orientation, int count) const
{
    KrossWordCellList list;
    Coord curCoord = coord;
    const Offset letterOffset = orientation == Qt::Horizontal ? Offset(1, 0) : Offset(0, 1);
    if (count == -1)
        count = orientation == Qt::Horizontal ? (int)width() - coord.first
                : (int)height() - coord.second;
    while (count > 0 && inside(curCoord)) {
        list << at(curCoord);

        curCoord += letterOffset;
        --count;
    }

    return list;
}

ImageCellList KrossWord::images() const
{
    ImageCellList list;
    for (uint x = 0; x < width(); ++x) {
        for (uint y = 0; y < height(); ++y) {
            KrossWordCell *cell = at(Coord(x, y));
            if (cell && cell->cellType() == KrossWordCell::ImageCellType
                    && !list.contains((ImageCell*)cell))
                list << (ImageCell*)cell;
        }
    }
    return list;
}

EmptyCellList KrossWord::emptyCells() const
{
    EmptyCellList list;
    for (uint x = 0; x < width(); ++x) {
        for (uint y = 0; y < height(); ++y) {
            KrossWordCell *cell = at(Coord(x, y));
            if (cell && cell->cellType() == KrossWordCell::EmptyCellType)
                list << (EmptyCell*)cell;
        }
    }
    return list;
}

LetterCellList KrossWord::letters() const
{
    LetterCellList list;
    for (uint x = 0; x < width(); ++x) {
        for (uint y = 0; y < height(); ++y) {
            KrossWordCell *cell = at(Coord(x, y));
            if (cell && cell->isLetterCell())
                list << (LetterCell*)cell;
        }
    }
    return list;
}

LetterCellList KrossWord::emptyLetters() const
{
    LetterCellList list;
    for (uint x = 0; x < width(); ++x) {
        for (uint y = 0; y < height(); ++y) {
            KrossWordCell *cell = at(Coord(x, y));
            if (cell && cell->isLetterCell() && ((LetterCell*)cell)->isEmpty())
                list << (LetterCell*)cell;
        }
    }
    return list;
}

ClueCell* KrossWord::firstClue() const
{
    for (uint x = 0; x < width(); ++x) {
        for (uint y = 0; y < height(); ++y) {
            KrossWordCell *cell = at(Coord(x, y));
            if (cell && cell->cellType() == KrossWordCell::ClueCellType)
                return (ClueCell*)cell;
            else if (cell->isLetterCell()) {   // Needed to get hidden clues
                LetterCell *letter = (LetterCell*)cell;
                if (letter->clueHorizontal() != nullptr)
                    return letter->clueHorizontal();
                else if (letter->clueVertical() != nullptr)
                    return letter->clueVertical();
            }
        } // for x
    } // for y

    return nullptr;
}

ClueCellList KrossWord::clues() const
{
    // TODO Store list of clues instead of constructing it every time
    ClueCellList list;
    for (uint x = 0; x < width(); ++x) {
        for (uint y = 0; y < height(); ++y) {
            KrossWordCell *cell = at(Coord(x, y));

            if (cell) {
                ClueCell *clue;
                DoubleClueCell *doubleClue;
                if ((clue = qgraphicsitem_cast<ClueCell*>(cell)))
                    // Add clue, that isn't hidden
                    list << clue;
                else if (cell->isLetterCell()) {   // Needed to get hidden clues
                    // Only add clues that are hidden, because other clues are added above.
                    // It is also checked that the letter cell is the first one of the clue
                    // cell, to not add clues multiple times (for each letter cell of the clue).
                    LetterCell *letter = (LetterCell*)cell;
                    if (letter->clueHorizontal() && letter->clueHorizontal()->isHidden() &&
                            letter == letter->clueHorizontal()->firstLetter())
                        list << letter->clueHorizontal();
                    if (letter->clueVertical() && letter->clueVertical()->isHidden() &&
                            letter == letter->clueVertical()->firstLetter())
                        list << letter->clueVertical();
                } else if ((doubleClue = qgraphicsitem_cast<DoubleClueCell*>(cell))) {
                    list << doubleClue->clue1();
                    list << doubleClue->clue2();
                }
            }
        } // for y
    } // for x
    return list;
}

void KrossWord::clues(ClueCellList* horizontalClues, ClueCellList* verticalClues) const
{
    Q_ASSERT(horizontalClues);
    Q_ASSERT(verticalClues);

    for (uint x = 0; x < width(); ++x) {
        for (uint y = 0; y < height(); ++y) {
            KrossWordCell *cell = at(Coord(x, y));

            if (cell) {
                ClueCell *clue;
                DoubleClueCell *doubleClue;
                if ((clue = qgraphicsitem_cast<ClueCell*>(cell))) {
                    // Add clue, that isn't hidden
                    if (clue->isHorizontal())
                        *horizontalClues << clue;
                    else
                        *verticalClues << clue;
                } else if (cell->isLetterCell()) {   // Needed to get hidden clues
                    // Only add clues that are hidden, because other clues are added above.
                    // It is also checked that the letter cell is the first one of the clue
                    // cell, to not add clues multiple times (for each letter cell of the clue).
                    LetterCell *letter = (LetterCell*)cell;
                    if (letter->clueHorizontal() && letter->clueHorizontal()->isHidden()  &&
                            letter == letter->clueHorizontal()->firstLetter())
                        *horizontalClues << letter->clueHorizontal();
                    if (letter->clueVertical() && letter->clueVertical()->isHidden()  &&
                            letter == letter->clueVertical()->firstLetter())
                        *verticalClues << letter->clueVertical();
                } else if ((doubleClue = qgraphicsitem_cast<DoubleClueCell*>(cell))) {
                    if (doubleClue->clue1()->isHorizontal())
                        *horizontalClues << doubleClue->clue1();
                    else
                        *verticalClues << doubleClue->clue1();

                    if (doubleClue->clue2()->isHorizontal())
                        *horizontalClues << doubleClue->clue2();
                    else
                        *verticalClues << doubleClue->clue2();
                }
            }
        } // for y
    } // for x

    qSort(horizontalClues->begin(), horizontalClues->end(), lessThanClueNumber);
    qSort(verticalClues->begin(), verticalClues->end(), lessThanClueNumber);
}

bool KrossWord::isCellEmptyIfSpannedCellIsExcluded(const Coord& coord,
        SpannedCell* excludedSpannedCell) const
{
    if (!excludedSpannedCell)
        return false;

    KrossWordCell *oldCell = at(coord);
    return oldCell == excludedSpannedCell;
}

bool KrossWord::canTakeSpannedCell(const Coord& coord,
                                   int horizontalCellSpan, int verticalCellSpan,
                                   SpannedCell* excludedSpannedCell) const
{
    Coord bottomRight = Coord(coord.first + horizontalCellSpan - 1,
                              coord.second + verticalCellSpan - 1);
    for (int x = coord.first; x <= bottomRight.first; ++x) {
        for (int y = coord.second; y <= bottomRight.second; ++y) {
            Coord curCoord = Coord(x, y);
            if (at(curCoord)->cellType() != KrossWordCell::EmptyCellType &&
                    !isCellEmptyIfSpannedCellIsExcluded(curCoord, excludedSpannedCell))
                return false;
        }
    }

    return true;
}

bool KrossWord::isCellEmptyIfClueIsExcluded(const Coord& coord, ClueCell* excludedClue) const
{
    if (!excludedClue)
        return false;

    KrossWordCell *oldCell = at(coord);
    return oldCell == excludedClue
           || (oldCell->isLetterCell()
               && ((LetterCell*)oldCell)->isAttachedToClueExclusivly(excludedClue));
}

bool KrossWord::canTakeClueCell(const Coord& coord,
                                const Offset &firstLetterOffset,
                                bool allowDoubleClueCells, ClueCell *excludedClue) const
{
    // Check if the cell for the new clue is empty
    // or if it is a ClueCell and double clue cells are allowed
    bool cellIsEmptyIfClueIsExcluded = isCellEmptyIfClueIsExcluded(coord, excludedClue);
    KrossWordCell *cellAtCoord = at(coord);
    switch (cellAtCoord->cellType()) {
    case KrossWordCell::EmptyCellType:
        // Clue can be added here
        return true;
        break;

    case KrossWordCell::LetterCellType:
    case KrossWordCell::SolutionLetterCellType:
        if ((firstLetterOffset != Offset(0, 0) || ((LetterCell*)cellAtCoord)->isCrossed())
                && !cellIsEmptyIfClueIsExcluded) {
//   qDebug() << "Can only add clue's with hidden clue cells at coordinates containing letter cells";
            return false;
        }
        break;

    case KrossWordCell::ClueCellType:
        if (!allowDoubleClueCells && !cellIsEmptyIfClueIsExcluded) {
            qDebug() << "Double clue cells not allowed";
            return false;
        }
        break;

    case KrossWordCell::DoubleClueCellType:
        if (!((DoubleClueCell*)cellAtCoord)->hasClue(excludedClue)) {
            qDebug() << "Can't add clue cells to double clue cells";
            return false;
        }
        break;

    default:
        qDebug() << "Can't add clue cells at coordinates containing cells with type"
                 << KrossWordCell::displayStringFromCellType(cellAtCoord->cellType());
        return false;
        break;
    }

    return true;
}

KrossWord::ErrorType KrossWord::canInsertImage(const Grid2D::Coord& coord,
        int horizontalCellSpan, int verticalCellSpan,
        KrossWord::ErrorTypes correctnessesToIgnore, ImageCell* excludedImage)
{
    // Check if the image will fit into the grid
    if (!correctnessesToIgnore.testFlag(ErrorImageDoesntFit)
            && (!inside(coord) || !inside(coord + Offset(horizontalCellSpan - 1, verticalCellSpan - 1)))) {
        qDebug() << "Image doesn't fit into the grid" << coord
                 << horizontalCellSpan << verticalCellSpan;
        return ErrorImageDoesntFit;
    }

    // Check if the cells for the new image are empty
    if (!canTakeSpannedCell(coord, horizontalCellSpan, verticalCellSpan, excludedImage))
        return ErrorImageCellsArentEmpty;

    return NoError;
}

KrossWord::ErrorType KrossWord::canInsertClue(const Grid2D::Coord& coord,
        Qt::Orientation orientation, const Offset &firstLetterOffset,
        const QString& answer, ErrorTypes correctnessesToIgnore,
        bool allowDoubleClueCells, ClueCell *excludedClue)
{
    // Check if the clue and the answer will fit into the grid
    if (!correctnessesToIgnore.testFlag(ErrorClueDoesntFit)
            && (!inside(coord) || !inside(coord + firstLetterOffset)
                || (orientation == Qt::Horizontal
                    && !inside(coord + firstLetterOffset + Grid2D::Coord(answer.length() - 1, 0)))
                || (orientation == Qt::Vertical
                    && !inside(coord + firstLetterOffset + Grid2D::Coord(0, answer.length() - 1))))) {
        qDebug() << "Clue doesn't fit into the grid" << coord << firstLetterOffset << answer
                 << "orientation =" << orientation << "answer-length =" << answer.length();
        return ErrorClueDoesntFit;
    }

    // Check if the cell for the new clue is empty
    // or if it is a ClueCell and double clue cells are allowed
    if (!canTakeClueCell(coord, firstLetterOffset, allowDoubleClueCells, excludedClue))
        return ErrorClueCellIsntEmpty;

    bool cellIsEmptyIfClueIsExcluded = isCellEmptyIfClueIsExcluded(coord, excludedClue);

    // Check if the answers letter cells are legal
    QString answerUpper = answer.toUpper();
    Coord letterCoord = coord + firstLetterOffset;
    const Offset letterOffset = orientation == Qt::Horizontal ? Offset(1, 0) : Offset(0, 1);
    for (int i = 0; i < answerUpper.length(); ++i) {
        if (!correctnessesToIgnore.testFlag(ErrorAnswerContainsIllegalCharacters)
                && !ALLOWED_CHARACTERS.contains(answerUpper[i]) && answerUpper[i] != ' ') {
            qDebug() << "Illegal character" << answerUpper[i];
            return ErrorAnswerContainsIllegalCharacters;
        }

        if (!cellIsEmptyIfClueIsExcluded) {
            KrossWordCell *oldCell = at(letterCoord);
            if (!correctnessesToIgnore.testFlag(ErrorAnswerCrossesClueCell)
                    && oldCell->cellType() == KrossWordCell::ClueCellType) {
                qDebug() << "Answer crosses a clue cell at" << letterCoord
                         /*<< clue*/ << answer << "crossed clue:"
                         << qgraphicsitem_cast<ClueCell*>(oldCell)->clue();
                return ErrorAnswerCrossesClueCell;
            } else if (oldCell->isLetterCell()) {
                LetterCell *letterCell = (LetterCell*)oldCell;
                if (!correctnessesToIgnore.testFlag(ErrorAnswerIsIllegal)
                        && letterCell->correctLetter() != answerUpper.at(i)) {
                    qDebug() << "Answer is illegal in the current crossword at" << letterCoord
                             /*<< clue*/ << answer << "wanted letter:" << answerUpper.at(i)
                             << "letter from other clue:" << letterCell->correctLetter();
                    return ErrorAnswerIsIllegal;
                } else if (!correctnessesToIgnore.testFlag(ErrorAnswerOverwritesClueInSameOrientation)
                           && letterCell->hasClueInDirection(orientation)
                           && letterCell->clue(orientation) != excludedClue) {
                    qDebug() << "Answer overwrites answer to clue"
                             << letterCell->clue(orientation)->clue() << "at" << letterCoord;
                    return ErrorAnswerOverwritesClueInSameOrientation;
                }
            }
        }

        // Go to the next letter cell
        letterCoord = letterCoord + letterOffset;
    }

    return KrossWord::NoError;
}

KrossWord::ErrorType KrossWord::insertClue(const Grid2D::Coord &coord,
        Qt::Orientation orientation,
        ClueCell::AnswerOffset answerOffset,
        const QString &clue, const QString &answer,
        KrossWordCell::CellType cellType,
        ErrorTypes correctnessesToIgnore, bool allowDoubleClueCells,
        ClueCell **insertedClue)
{
    Q_ASSERT(cellType == KrossWordCell::LetterCellType
             || cellType == KrossWordCell::SolutionLetterCellType);

    Offset offset = ClueCell::answerOffsetToOffset(answerOffset);
    ErrorType correctness = canInsertClue(coord, orientation, offset, answer,
                                          correctnessesToIgnore, allowDoubleClueCells);
    if (correctness != NoError)
        return correctness;

    // Delete empty cell and insert new clue cell
    // or combine existing clue cell with the new clue cell to a double clue cell
    QString answerUpper = answer.toUpper();
    ClueCell *clueCell = new ClueCell(this, coord, orientation,
                                      answerOffset, clue, answerUpper/*, scene()*/);
    if (answerOffset != ClueCell::OnClueCell) {
        KrossWordCell *cellAtCoord = at(coord);
        if (cellAtCoord->cellType() == KrossWordCell::ClueCellType) {
            DoubleClueCell *doubleClueCell = new DoubleClueCell(
                this, coord, (ClueCell*)cellAtCoord, clueCell/*, scene()*/);
            (*m_krossWordGrid)[ coord ] = doubleClueCell;
        } else
            replaceCell(coord, clueCell);
    }

    // Insert letter cells for the answer
    Coord letterCoord = coord + offset;
    const Offset letterOffset = orientation == Qt::Horizontal ? Offset(1, 0) : Offset(0, 1);
    for (int i = 0; i < answerUpper.length(); ++i) {
        KrossWordCell *oldCell = at(letterCoord);

        if (oldCell->cellType() == KrossWordCell::EmptyCellType) {
            // Replace empty cell with new letter cell
            if (cellType == KrossWordCell::LetterCellType)
                replaceCell(letterCoord, new LetterCell(this, letterCoord, clueCell/*, scene()*/));
            else if (cellType == KrossWordCell::SolutionLetterCellType)
                replaceCell(letterCoord, new SolutionLetterCell(this, letterCoord, clueCell, i/*, scene()*/));
        } else if (oldCell->isLetterCell())
            ((LetterCell*)oldCell)->setClue(clueCell);      // Add clue to existing letter cell

        letterCoord = letterCoord + letterOffset;
    }

    if (insertedClue)
        *insertedClue = clueCell;

    emit cluesAdded(QList<ClueCell*>() << clueCell);

    return NoError;
}

void KrossWord::removeCell(KrossWordCell* cell)
{
    removeCell(cell->coord());
}

void KrossWord::removeCell(const Grid2D::Coord& coord)
{
    replaceCell(coord, new EmptyCell(this, coord));
}

void KrossWord::replaceCell(KrossWordCell* cell, KrossWordCell* newCell)
{
    replaceCell(cell->coord(), newCell);
}

void KrossWord::replaceCell(const Grid2D::Coord& coord, KrossWordCell* newCell)
{
    Q_ASSERT(newCell);
    replaceCell(coord, newCell, true);
}

void KrossWord::replaceCell(const Grid2D::Coord& coord,
                            KrossWordCell* newCell, bool deleteOldCell)
{
    KrossWordCell *oldCell = at(coord);
    SpannedCell *spannedCell;

    // Remove solution letter cells from the list
    if (oldCell->cellType() == KrossWordCell::SolutionLetterCellType)
        m_solutionLetters.removeOne((SolutionLetterCell*)oldCell);
    // Replace "other" cells (not at oldCell->coord()) with empty cells.
    // All those "other" cells point to the same spanned cell.
    else if ((spannedCell = dynamic_cast<SpannedCell*>(oldCell))) {
        Coord topLeft = spannedCell->coordTopLeft();
        Coord bottomRight = spannedCell->coordBottomRight();
        for (int x = topLeft.first; x <= bottomRight.first; ++x) {
            for (int y = (x == topLeft.first ? topLeft.second + 1 : topLeft.second);
                    y <= bottomRight.second; ++y) {
                (*m_krossWordGrid)[ Coord(x, y)] = new EmptyCell(this, Coord(x, y));
            }
        }
    }

    // Remove cell from scene
    if (oldCell->scene())
        oldCell->scene()->removeItem(oldCell);

    // Remove synchronization
//     oldCell->removeSynchronization(); // moved to KrossWordCell::~KrossWordCell()

    // Insert new cell
    (*m_krossWordGrid)[ coord ] = newCell;
    if (!(spannedCell = dynamic_cast<SpannedCell*>(newCell))
            || spannedCell->coordTopLeft() == coord) {  // Needed to not crash or
        // wrongly move spanned cells when adding spanned cells because they
        // are referenced by all contained cells with m_coord pointing to the
        // top left cell.
        newCell->m_coord = coord;
        newCell->setPositionFromCoordinates();
    }

    if (deleteOldCell) {
        // Delete old cell
        delete oldCell;
    }
}

// TODO Move to LetterCell::toSolutionLetter?
bool KrossWord::convertToSolutionLetter(const Grid2D::Coord& coord, int solutionLetterIndex)
{
    LetterCell *letter = qgraphicsitem_cast<LetterCell*>(at(coord));
    if (!letter) {   // letter isn't a LetterCell
        return false;
    }

    SolutionLetterCell *solutionLetter = SolutionLetterCell::fromLetterCell(letter, solutionLetterIndex);

    //CHECK: What the heck is this???? Hypotesis: this[coord] = solutionLetter  ---  maybe?
    operator[](coord) = solutionLetter;

    return true;
}

bool KrossWord::canTakeClueLetterCell(const Coord &coord, Qt::Orientation orientation,
                                      ClueCell *excludedClue)
{
    KrossWordCell *cell = at(coord);
    if (cell->cellType() == KrossWordCell::EmptyCellType || cell == excludedClue)
        return true;
    else if (cell->isLetterCell()) {
        LetterCell *letter = (LetterCell*)cell;
        if (!letter->hasClueInDirection(orientation))
            return true;
        else
            return letter->clue(orientation) == excludedClue;
    } else
        return false;
}

void KrossWord::resizeGrid(uint width, uint height)
{
//     if ( width < this->width() || height < this->height() )
    deleteAllCells();
    m_krossWordGrid->resize(width, height);
    fillWithEmptyCells();

    resizeScene();
}

void KrossWord::resizeScene()
{
    if (!scene()) {
        return;
    }
    QRect rect = QRect(QPoint(), boundingRect().size().toSize());
    scene()->setSceneRect(rect.adjusted(-150, -150, 150, 150));
}

bool KrossWord::isEmpty() const
{
    return clues().isEmpty();
}

void KrossWord::removeAllCells()
{
    emit cluesAboutToBeRemoved(clues());

    KrossWordCellList cellList = cells();
    foreach(KrossWordCell * cell, cellList) {
        removeCell(cell);
    }
}

void KrossWord::deleteAllCells()
{
    emit cluesAboutToBeRemoved(clues());

    KrossWordCellList cellList = cells();
    qDeleteAll(cellList);

    m_krossWordGrid->resize(0, 0);
    m_solutionLetters.clear();
}

void KrossWord::fillWithEmptyCells()
{
    Coord coord;
    for (coord.first = 0; (uint)coord.first < width(); ++coord.first) {
        for (coord.second = 0; (uint)coord.second < height(); ++coord.second) {
            if (!(*m_krossWordGrid)[coord])
                (*m_krossWordGrid)[ coord ] = new EmptyCell(this, coord/*, scene()*/);
        }
    }
}

QSize KrossWord::minimalSize() const
{
    Coord minCoord = m_krossWordGrid->coord(m_krossWordGrid->size());
    Coord maxCoord(0, 0);

    KrossWordCellList cellList = cells();
    foreach(KrossWordCell * cell, cellList) {
        minCoord = minimum(minCoord, cell->coord());
        maxCoord = maximum(maxCoord, cell->coord());
    }

    Coord size = maxCoord - minCoord;
    return QSize(size.first, size.second);
}

QString KrossWord::errorMessageFromErrorType(KrossWord::ErrorType errorType)
{
    switch (errorType) {
    case NoError:
        return i18n("No error.");

    case ErrorClueDoesntFit:
        return i18n("The clue doesn't fit in the grid with the given settings.");
    case ErrorClueCellIsntEmpty:
        return i18n("The cell for the new clue isn't empty.");
    case ErrorAnswerContainsIllegalCharacters:
        return i18n("The answer contains illegal characters. Allowed are A-Z.");
    case ErrorAnswerIsIllegal:
        return i18n("The answer produces mismatching letter cells at the same positions.");
    case ErrorAnswerOverwritesClueInSameOrientation:
        return i18n("The answer overwrites the answer to another clue in the same orientation");
    case ErrorAnswerCrossesClueCell:
        return i18n("The answer's letter cells cross a clue cell.");

    case ErrorImageDoesntFit:
        return i18n("The image doesn't fit in the grid with the given settings.");
    case ErrorImageCellsArentEmpty:
        return i18n("The cells for the new image aren't empty.");
    }

    return "Unknown error.";
}

ClueCell::AnswerOffset KrossWord::answerOffsetFromString(const QString &s)
{
    QString sl = s.toLower();
    if (sl == "cluehidden")
        return ClueCell::OnClueCell;
    else if (sl == "right")
        return ClueCell::OffsetRight;
    else if (sl == "bottom")
        return ClueCell::OffsetBottom;
    else if (sl == "left")
        return ClueCell::OffsetLeft;
    else if (sl == "top")
        return ClueCell::OffsetTop;
    else if (sl == "topleft")
        return ClueCell::OffsetTopLeft;
    else if (sl == "topright")
        return ClueCell::OffsetTopRight;
    else if (sl == "bottomleft")
        return ClueCell::OffsetBottomLeft;
    else if (sl == "bottomright")
        return ClueCell::OffsetBottomRight;
    else {
        qDebug() << "Couldn't get enumerable for" << s;
        return ClueCell::OffsetInvalid;
    }
}

QString KrossWord::answerOffsetToString(ClueCell::AnswerOffset answerOffset)
{
    switch (answerOffset) {
    case ClueCell::OffsetTop:
        return "Top";
    case ClueCell::OffsetRight:
        return "Right";
    case ClueCell::OffsetLeft:
        return "Left";
    case ClueCell::OffsetBottom:
        return "Bottom";
    case ClueCell::OffsetTopLeft:
        return "TopLeft";
    case ClueCell::OffsetTopRight:
        return "TopRight";
    case ClueCell::OffsetBottomLeft:
        return "BottomLeft";
    case ClueCell::OffsetBottomRight:
        return "BottomRight";
    case ClueCell::OffsetInvalid: // Shouldn't appear here..
        qDebug() << "Got an invalid answerOffset";
    case ClueCell::OnClueCell:
    default:
        return "ClueHidden";
    }
}

/*
KrossWordReader::KrossWordReader( const QString& fileName, KrossWord *krossWord,
    KrossWord::FileFormat fileFormat, QObject* parent )
    : QThread( parent ),
    m_krossWord( krossWord ),
    m_device( new QFile(fileName) ),
    m_fileFormat( fileFormat ),
    m_fileName( fileName )
{
}

void KrossWordReader::run()
{
//     QThread::run();
    QString errorString;
qDebug() << "Running read inside a new thread";

//     QFile file( m_fileName );
    if ( !m_device->open(QIODevice::ReadOnly) ) {
 errorString = m_device->errorString();
 qWarning() << errorString;
//  if ( removeTempFile )
//      KIO::NetAccess::removeTempFile( fileName );
 return;
    }

    m_krossWord->setHighlightedClue( NULL );
    m_krossWord->removeAllCells();
    bool wasBlocking = m_krossWord->blockSignals( true );

    bool readOk = false, fileFormatDeterminationFailed = false;
    if ( m_fileFormat == KrossWord::DetermineByFileName ) {
 QString extension = QFileInfo( m_fileName ).suffix();
 if ( extension == "puz" )
     m_fileFormat = KrossWord::AcrossLitePuzFile;
 else if ( extension == "xml" || extension == "kwp" )
     m_fileFormat = KrossWord::KrossWordPuzzleXmlFile;
 else if ( extension == "kwpz" )
     m_fileFormat = KrossWord::KrossWordPuzzleCompressedXmlFile;
 else {
     fileFormatDeterminationFailed = true;

     // Cycle through the available readers
     KrossWordPuzStream puzReader;
     readOk = puzReader.read( m_device, m_krossWord );

     if ( !readOk ) {
  if ( m_device->isOpen() ) m_device->seek( 0 );
  KrossWordXmlReader xmlReader;
  readOk = xmlReader.read( m_device, m_krossWord );

  if ( !readOk ) {
      if ( m_device->isOpen() ) m_device->seek( 0 );
      readOk = xmlReader.readCompressed( m_device, m_krossWord );
  }
     }

     if ( !readOk )
  errorString = "File format unknown";
 }
    }

    if ( !fileFormatDeterminationFailed ) {
 if ( m_fileFormat == KrossWord::AcrossLitePuzFile ) {
     KrossWordPuzStream puzReader;
     readOk = puzReader.read( m_device, m_krossWord );
     if ( !readOk )
  errorString = i18n("Error reading AcrossLite's .puz-format.");
 } else if ( m_fileFormat == KrossWord::KrossWordPuzzleXmlFile ) {
     KrossWordXmlReader xmlReader;
     readOk = xmlReader.read( m_device, m_krossWord );
     if ( !readOk )
  errorString = xmlReader.errorString();
 } else if ( m_fileFormat == KrossWord::KrossWordPuzzleCompressedXmlFile ) {
     KrossWordXmlReader xmlReader;
     readOk = xmlReader.readCompressed( m_device, m_krossWord );
     if ( !readOk )
  errorString = xmlReader.errorString();
 }
    }
    m_device->close();
    blockSignals( wasBlocking );
}*/





