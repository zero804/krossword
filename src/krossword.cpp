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
#include "krosswordtheme.h"
#include "clueexpanderitem.h"
#include "cells/krosswordcell.h"
#include "cells/imagecell.h"
#include "io/krosswordpuzreader.h"
#include "io/krosswordxmlreader.h"
#include "io/krosswordxmlwriter.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QFile>
#include <QStandardItemModel>
#include <qfileinfo.h>

#include <QUrl>
#include <QIcon>
#include <kglobalsettings.h>

#include "animator.h"
#include <QPropertyAnimation>
#include <QFontDatabase>
#include <QMimeDatabase>


namespace Crossword
{

KrossWord::KrossWord(const KrosswordTheme *theme, int width, int height)
    : QGraphicsObject(0), m_animator(new Animator()),
    m_currentCell(0), m_previousCell(0),
    m_highlightedClue(0), m_previousHighlightedClue(0),
    m_focusItem(0), m_headerItem(0),
    m_theme(theme)
{
    init(width, height);
    fillWithEmptyCells();

    m_headerItem = new QGraphicsTextItem(this);
    m_headerItem->setVisible(false);
    //m_headerItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPointSize(qBound(10.0, m_cellSize.height()/3, 13.0));
    m_headerItem->setFont(font);
    m_headerItem->setDefaultTextColor(m_theme->fontColor());
    m_headerItem->setHtml("<strong>title</strong> by author<br><small>copyright</small>"); //placeholder just to update boundingrect
    m_headerItem->setPos(boundingRect().topLeft() - m_headerItem->boundingRect().bottomLeft());
}

void KrossWord::init(uint width, uint height)
{
    if (!m_theme) {
        m_theme = KrosswordTheme::defaultValues();
    }

    m_krossWordGrid = new KrosswordGrid(width, height);
    m_editable = false;
    m_interactive = true;
    m_drawForPrinting = false;
    m_animationEnabled = true;
    m_keyboardNavigation = DefaultKeyboardNavigation;
    m_emptyCellColorForPrinting = Qt::black;
    m_cellSize = QSizeF(50, 50);
    m_letterEditMode = AutomaticKeyboardEditing;
    m_focusItem = new FocusItem(this);
    m_focusItem->hide();
    m_focusItem->setZValue(500);
    m_focusItem->setBrush(QBrush(Qt::transparent));
    m_focusItem->setPen(QPen(m_theme->selectionColor(), 3));

    setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemIsSelectable);
}

KrossWord::~KrossWord()
{
//     qDebug() << "Cleaning crossword";
    deleteAllCells(); // Cells are delete'd by QGraphicsScene only if they are in a QGraphicsScene
}


void KrossWord::setTheme(const KrosswordTheme* theme)
{
    m_theme = theme;
    m_focusItem->setPen(QPen(m_theme->selectionColor()));
    clearCache();
}

void KrossWord::createNew(CrosswordType crosswordType, const QSize& crosswordSize)
{
    createNew(CrosswordTypeInfo::infoFromType(crosswordType), crosswordSize);
}

void KrossWord::createNew(const CrosswordTypeInfo &crosswordTypeInfo, const QSize &crosswordSize)
{
    removeAllCells();
    resizeGrid(crosswordSize.width(), crosswordSize.height());

//   m_crosswordTypeInfo = CrosswordTypeInformation::infoFromType( crosswordType );
    m_crosswordTypeInfo = crosswordTypeInfo;

    if (m_crosswordTypeInfo.clueType == NumberClues1To26
            && m_crosswordTypeInfo.clueMapping == CluesReferToCells
            && m_crosswordTypeInfo.letterCellContent == Characters) {
        m_codedPuzzleMapping = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    } else
        m_codedPuzzleMapping.clear();
}

bool KrossWord::has180DegreeRotationSymmetry() const
{
    KrossWordCellList cellList = cells(EmptyCellType | ClueCellType);

    while (!cellList.isEmpty()) {
        KrossWordCell *cell = cellList.first();
        KrossWordCell *symmetryCell = at(Coord(
                                             width() - 1 - cell->coord().first,
                                             height() - 1 - cell->coord().second));
        if (cellList.contains(symmetryCell)) {
            cellList.removeFirst(); // remove cell
            cellList.removeOne(symmetryCell);
        } else
            return false;
    }

    return true;
}

void KrossWord::setupSameLetterSynchronization()
{
    removeSameLetterSynchronization();

    QHash< QChar, KrossWordCellList > correctCharToLetters;
    LetterCellList letterList = letters();
    foreach(LetterCell * letter, letterList) {
        QChar ch = letter->correctLetter();
        if (ch.isSpace())
            continue;

        correctCharToLetters[ ch ] << letter;
    }

    for (QHash<QChar, KrossWordCellList>::iterator it = correctCharToLetters.begin();
            it != correctCharToLetters.end(); ++it) {
        while (!it.value().isEmpty()) {
            KrossWordCell *letter = it.value().takeFirst();
            letter->synchronizeWith(it.value(), SyncContent,
                                    SameCharacterLetterSynchronization);
        }
    }
}

void KrossWord::removeSameLetterSynchronization()
{
    removeSynchronization(SyncContent, SameCharacterLetterSynchronization);
}

QRectF KrossWord::boundingRect() const
{
    QRectF rect = QRectF(0, 0, getCellSize().width() * width(), getCellSize().height() * height());
    if (m_headerItem->isVisible()) {
        return rect.united(m_headerItem->boundingRect());
    }

    return rect;
}

void KrossWord::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (isDrawingForPrinting())
        painter->fillRect(boundingRect(), Qt::white);
}

KrossWord::ConversionInfo KrossWord::generateConversionInfo(
    CrosswordTypeInfo newInfo)
{
    ConversionInfo conversionInfo;
    conversionInfo.conversionCommands = NoCommand;
    conversionInfo.typeInfoSource = crosswordTypeInfo();
    conversionInfo.typeInfoTarget = newInfo;

    ClueCellList clueList = clues();
    if (m_crosswordTypeInfo.minAnswerLength < newInfo.minAnswerLength) {
        qDebug() << "Remove clues with answers shorter than the new minimal "
                 "answer length " << newInfo.minAnswerLength;

        for (int i = clueList.count() - 1; i >= 0; --i) {
            ClueCell *clue = clueList[ i ];
            if (clue->correctAnswer().length() < newInfo.minAnswerLength)
                conversionInfo.cluesToRemove << clueList.takeAt(i);
        }
    }

    // Add newly disallowed cell types to conversion info as "to be removed"
    foreach(CellType cellType, allCellTypes()) {
        if (!m_crosswordTypeInfo.cellTypes.testFlag(cellType))
            continue;

        if (!newInfo.cellTypes.testFlag(cellType)) {
            // Cell type isn't available in the new crossword type, so remove those cells
            KrossWordCellList cellList = cells(cellType);
            foreach(KrossWordCell * cell, cellList) {
                if (!conversionInfo.cellsToRemove.contains(cell)
                        && !conversionInfo.cluesToRemove.contains(qgraphicsitem_cast<ClueCell*>(cell)))
                    conversionInfo.cellsToRemove << cell;
            }
        }
    }

    if (m_crosswordTypeInfo.clueCellHandling != ClueCellsRequired
            && newInfo.clueCellHandling == ClueCellsRequired) {
        qDebug() << "Make clue cells visible in the crossword grid";

        QList<Coord> disallowedCoords, coordsWithNewClueCell;
        for (int i = clueList.count() - 1; i >= 0; --i) {
            ClueCell *clue = clueList[ i ];
            if (clue->answerOffset() != OnClueCell)
                continue; // Clue is already visible

//       qDebug() << "DISALLOWED ARE:" << disallowedCoords;
            AnswerOffset offset = clue->tryToMakeVisible(true, disallowedCoords);
            if (offset == OffsetInvalid) {
                conversionInfo.cluesToRemove << clueList.takeAt(i);
//  qDebug() << "Added clue to remove list:" << clue->clue();
            } else {
                Coord newCoord = clue->coord() - ClueCell::answerOffsetToOffset(offset);
                KrossWordCell *cell = at(newCoord);
                if (cell->isType(ClueCellType))
                    disallowedCoords.append(newCoord);
                else if (cell->isType(EmptyCellType)) {
                    if (coordsWithNewClueCell.contains(newCoord))
                        disallowedCoords.append(newCoord);
                    else
                        coordsWithNewClueCell.append(newCoord);
                }

                conversionInfo.cluesToMakeVisible.insert(clue, offset);
//  conversionInfo.cluesToMakeVisible << clue;
//  qDebug() << "Added clue to make visible list:" << clue->clue();
            }
        }
    } else if (m_crosswordTypeInfo.clueCellHandling != ClueCellsDisallowed
               && newInfo.clueCellHandling == ClueCellsDisallowed) {
        qDebug() << "Hide all clue cells from the crossword grid";

        for (int i = clueList.count() - 1; i >= 0; --i) {
            ClueCell *clue = clueList[ i ];
            if (clue->answerOffset() != OnClueCell
                    && !conversionInfo.cellsToRemove.contains(clue)
                    && !conversionInfo.cluesToRemove.contains(clue)) {
                // The clue cells will only get removed from the crossword grid,
                // but the ClueCell object stays stored.
                conversionInfo.cellsToRemove << clueList.takeAt(i);
            }
        }
    }

    if (m_crosswordTypeInfo.clueType == StringClues
            && newInfo.clueType == NumberClues1To26) {
        qDebug() << "Convert from clueType StringClues to NumberClues1To26";

        if (newInfo.clueMapping == CluesReferToCells
                && newInfo.letterCellContent == Characters) {
            qDebug() << "Convert to coded puzzle clue mapping";

            // Mark all clue cells to get removed from the crossword grid
            for (int i = clueList.count() - 1; i >= 0; --i) {
                ClueCell *clue = clueList[ i ];
                if (!conversionInfo.cellsToRemove.contains(clue) &&
                        !conversionInfo.cluesToRemove.contains(clue))
                    conversionInfo.cellsToRemove << clueList.takeAt(i);
            }

            conversionInfo.conversionCommands |= SetDefaultCodedPuzzleMapping;
            conversionInfo.conversionCommands |= SetupSameLetterSynchronization;
        } else {
            qDebug() << "Can't convert, removing all clues";
            conversionInfo.cluesToMakeVisible.clear();
            for (int i = clueList.count() - 1; i >= 0; --i) {
                ClueCell *clue = clueList[ i ];
                if (!conversionInfo.cluesToRemove.contains(clue))
                    conversionInfo.cluesToRemove << clueList.takeAt(i);
            }
        }
    }
    // NOTHING TO DO HERE (ie. no conversion needed, just works™):
//    else if ( m_crosswordTypeInfo.clueType == NumberClues1To26
//      && newInfo.clueType == StringClues ) {
//     qDebug() << "Can't convert, removing all clues (2)";
//     conversionInfo.cluesToMakeVisible.clear();
//
//     for ( int i = clueList.count() - 1; i >= 0; --i ) {
//       ClueCell *clue = clueList[ i ];
//       if ( !conversionInfo.cluesToRemove.contains(clue) &&
//        !conversionInfo.cluesToRemove.contains(clue) )
//  conversionInfo.cluesToRemove << clueList.takeAt( i );
//     }
//   }

    // Convert letter cell content if needed (characters <=> digits)
    if (m_crosswordTypeInfo.letterCellContent != Characters
            && newInfo.letterCellContent == Characters) {
        qDebug() << "Convert letter content to characters";
        LetterCellList letterList = letters();

        foreach(LetterCell * letter, letterList) {
            bool letterGetsRemoved = true;
            foreach(ClueCell * clue, letter->clues()) {
                if (!conversionInfo.cluesToRemove.contains(clue)) {
                    letterGetsRemoved = false;
                    break;
                }
            }
            if (letterGetsRemoved)
                continue;

            // Shift from numbers to characters in ASCII table
            if (!letter->correctLetter().isSpace() && !letter->correctLetter().isLetter())
                conversionInfo.letterEditCorrect.insert(letter,
                                                        QChar::fromLatin1(letter->correctLetter().toLatin1() + 17));
            if (!letter->currentLetter().isSpace() && !letter->currentLetter().isLetter())
                conversionInfo.letterEditCurrent.insert(letter,
                                                        QChar::fromLatin1(letter->currentLetter().toLatin1() + 17));
        }
    } else if (m_crosswordTypeInfo.letterCellContent != Digits
               && newInfo.letterCellContent == Digits) {
        qDebug() << "Convert letter content to digits";
        LetterCellList letterList = letters();
        foreach(LetterCell * letter, letterList) {
            bool letterGetsRemoved = true;
            foreach(ClueCell * clue, letter->clues()) {
                if (!conversionInfo.cluesToRemove.contains(clue)) {
                    letterGetsRemoved = false;
                    break;
                }
            }
            if (letterGetsRemoved)
                continue;

            // Shift from characters to numbers in ASCII table
            if (!letter->correctLetter().isSpace() && !letter->correctLetter().isDigit())
                conversionInfo.letterEditCorrect.insert(letter,
                                                        QChar::fromLatin1((letter->correctLetter().toLatin1() - 65) % 10 + 48));

            if (!letter->currentLetter().isSpace() && !letter->currentLetter().isDigit())
                conversionInfo.letterEditCurrent.insert(letter,
                                                        QChar::fromLatin1((letter->currentLetter().toLatin1() - 65) % 10 + 48));
        }
    }

    return conversionInfo;
}

void KrossWord::executeConversionInfo(KrossWord::ConversionInfo conversionInfo)
{
    setHighlightedClue(NULL);

    foreach(KrossWordCell * cell, conversionInfo.cellsToRemove) {
        ClueCell *clue;
        SolutionLetterCell *solutionLetter;
        if ((clue = qgraphicsitem_cast<ClueCell*>(cell)))
            clue->setHidden();
        else if ((solutionLetter = qgraphicsitem_cast<SolutionLetterCell*>(cell)))
            solutionLetter->toLetter();
        else
            removeCell(cell);
    }

    emit cluesAboutToBeRemoved(conversionInfo.cluesToRemove);
    // Don't emit cluesAboutToBeRemoved() in removeClue(), it has just been
    // emitted for all clues to be removed
    bool wasBlocked = blockSignals(true);
    foreach(ClueCell * clue, conversionInfo.cluesToRemove)
    removeClue(clue);
    blockSignals(wasBlocked);


    for (QHash<ClueCell*, AnswerOffset>::const_iterator it =
                conversionInfo.cluesToMakeVisible.constBegin();
            it != conversionInfo.cluesToMakeVisible.constEnd(); ++it) {
//   foreach ( ClueCell *clue, conversionInfo.cluesToMakeVisible.keys() ) {
//     if ( clue->tryToMakeVisible() == ClueCell::OffsetInvalid )
        if (!it.key()->setUnhidden(it.value()))
            qDebug() << "Couldn't make clue cell visible" << it.key()->coord() << it.key()->clue();
    }

    QHash<LetterCell*, QChar>::const_iterator it;
    for (it = conversionInfo.letterEditCurrent.constBegin();
            it != conversionInfo.letterEditCurrent.constEnd(); ++it) {
        it.key()->setCurrentLetter(it.value(), it.key()->confidence());
    }

    for (it = conversionInfo.letterEditCorrect.constBegin();
            it != conversionInfo.letterEditCorrect.constEnd(); ++it) {
        it.key()->setCorrectLetter(it.value());
    }

    if (conversionInfo.conversionCommands.testFlag(SetDefaultCodedPuzzleMapping))
        m_codedPuzzleMapping = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    if (conversionInfo.conversionCommands.testFlag(SetupSameLetterSynchronization))
        setupSameLetterSynchronization();
    else
        removeSameLetterSynchronization();

    m_crosswordTypeInfo = conversionInfo.typeInfoTarget;
}

KrossWord::ConversionInfo KrossWord::convertToType(
    CrosswordTypeInfo newInfo, bool simulate)
{
    ConversionInfo conversionInfo = generateConversionInfo(newInfo);

    if (!simulate)
        executeConversionInfo(conversionInfo);

    return conversionInfo;
}

QString KrossWord::conversionInfoToString(
    KrossWord::ConversionInfo conversionInfo)
{
    int imageCells = 0;
    int clueCells = conversionInfo.cluesToRemove.count();
    int otherCells = 0;
    KrossWordCellList otherCellList;

    foreach(KrossWordCell * cell, conversionInfo.cellsToRemove) {
        switch (cell->getCellType()) {
//  case KrossWordCell::ClueCellType:
//       ++clueCells;
//    break;
//   case KrossWordCell::DoubleClueCellType:
//       clueCells += 2;
        case ImageCellType:
            ++imageCells;
            break;

        default:
            otherCellList << cell;
            ++otherCells;
        }
    }

    QHash< CellType, int > countPerType;
    foreach(KrossWordCell * cell, otherCellList)
    ++countPerType[ cell->getCellType()];

    QStringList otherInfos;
    for (QHash<CellType, int>::const_iterator it = countPerType.constBegin();
            it != countPerType.constEnd(); ++it) {
//   foreach ( CellType cellType, countPerType.keys() ) {
        switch (it.key()) {
        case ClueCellType:
            otherInfos << i18np("%1 clue cell", "%1 clue cells",
                                countPerType[it.key()], stringFromCellType(it.key()));
            break;

        case DoubleClueCellType:
            otherInfos << i18np("%1 double clue cell", "%1 double clue cells",
                                countPerType[it.key()], stringFromCellType(it.key()));
            break;

        case SolutionLetterCellType:
            otherInfos << i18np("%1 solution letter cell", "%1 solution letter cells",
                                countPerType[it.key()], stringFromCellType(it.key()));
            break;

        default:
            break;
        }
    }

    QString infoText;
    if (images().isEmpty()) {
        infoText = i18n("The conversion will remove %1 of %2 clues",
                        clueCells, clues().count());
    } else {
        infoText = i18n("The conversion will remove %1 of %2 clues, "
                        "%3 of %4 images",
                        clueCells, clues().count(),
                        imageCells, images().count());
    }

    if (otherInfos.isEmpty())
        infoText += '.';
    else
        infoText += ", " + otherInfos.join(", ");

    if (crosswordTypeInfo().letterCellContent != Digits
            && conversionInfo.typeInfoTarget.letterCellContent == Digits) {
        infoText += '\n' + i18n("Letters can't be converted back correctly after this conversion.");
    } else if ((conversionInfo.typeInfoSource.clueType != NumberClues1To26
                || conversionInfo.typeInfoSource.clueMapping != CluesReferToCells
                || conversionInfo.typeInfoSource.letterCellContent != Characters)
               && conversionInfo.typeInfoTarget.clueType == NumberClues1To26
               && conversionInfo.typeInfoTarget.clueMapping == CluesReferToCells
               && conversionInfo.typeInfoTarget.letterCellContent == Characters) {
        infoText += '\n' + i18n("All clue cells get removed. "
                                "The initial letter to clue number mapping is A=1, B=2, ..., Z=26.");
//     infoText += "\n" + i18n( "Although this conversion removes all clues, "
//  "the answer word grouping isn't removed. Instead for each letter cell "
//  "a number is shown as the clue according to the default letter to "
//  "clue number mapping (A=1, B=2, ..., Z=26)." );
    }

    return infoText;
}

void KrossWord::setLetterContentToClueNumberMapping(
    const QString &codedPuzzleMapping, bool apply)
{
    if (apply)
        applyLetterContentToClueNumberMapping(codedPuzzleMapping);
    else {
        if (m_codedPuzzleMapping == codedPuzzleMapping.toUpper())
            return;
        m_codedPuzzleMapping = codedPuzzleMapping.toUpper();
    }
}

void KrossWord::applyLetterContentToClueNumberMapping(
    const QString &codedPuzzleMapping)
{
    if (m_codedPuzzleMapping == codedPuzzleMapping.toUpper())
        return;

    m_codedPuzzleMapping = codedPuzzleMapping.toUpper();
    clearCache();
}

void KrossWord::setCurrentCell(KrossWordCell* cell)
{
    if (m_currentCell == cell) {
//  qDebug() << "current is equal to new current"
//      << (cell ? KrossWordCell::cellTypeToString(cell->cellType()) : "NULL" );
        return;
    }

//     qDebug() << "from" << (m_currentCell ? KrossWordCell::cellTypeToString(m_currentCell->cellType()) : "NULL");
//     qDebug() << "to" << (cell ? KrossWordCell::cellTypeToString(cell->cellType()) : "NULL" );
    if (m_currentCell) {
        disconnect(m_currentCell, SIGNAL(destroyed(QObject*)),
                   this, SLOT(currentCellDestroyed(QObject*)));
    }

    m_previousCell = m_currentCell;
    m_currentCell = cell;

    if (m_currentCell) {
        m_focusItem->setRect(QRectF(m_currentCell->pos() + m_currentCell->boundingRect().topLeft(),
                                    m_currentCell->boundingRect().size()));
        if (isAnimationEnabled()) {
            QPropertyAnimation *anim = new QPropertyAnimation(m_focusItem, "opacity");
            anim->setStartValue(m_focusItem->opacity());
            anim->setEndValue(1);
            animator()->startOrEnqueue(anim, Animator::Slowest);
        } else {
            m_focusItem->show();
        }
    }

    if (cell) {
        if (cell->isType(ImageCellType)) {
            setHighlightedClue(NULL);
        }
        connect(m_currentCell, SIGNAL(destroyed(QObject*)),
                this, SLOT(currentCellDestroyed(QObject*)));
    }
    emit currentCellChanged(cell, m_previousCell);
}

void KrossWord::currentCellDestroyed(QObject*)
{
//     if ( m_currentCell )
//  qDebug() << KrossWordCell::cellTypeToString( m_currentCell->cellType() );
//     else
//  qDebug() << "Nothing destroyed?!?";

    if (inside(m_currentCell->coord()) && at(m_currentCell->coord()) != m_currentCell)
        setCurrentCell(at(m_currentCell->coord()));
    else
        setCurrentCell(NULL);
}

float KrossWord::solutionProgress() const
{
    LetterCellList letterList = letters();
    if (letterList.isEmpty())
        return 0.0f;

    int solved = 0;
    foreach(LetterCell * letter, letterList) {
        if (!letter->isEmpty())
            ++ solved;
    }

    return (float)solved / (float)letterList.count();     // TODO Div/0!
}

QSize KrossWord::emptyCellSpan(const Coord& coordTopLeft, SpannedCell *excludedCell)
{
    KrossWordCell *cell = at(coordTopLeft);
    if (cell->getCellType() != EmptyCellType && cell != excludedCell)
        return QSize(0, 0);

    int maxWidth = width() - coordTopLeft.first;
    int maxHeight = height() - coordTopLeft.second;

    // TODO
    for (int x = 0; x < maxWidth - 1; ++x) {
        for (int y = 0; y < maxHeight - 1; ++y) {
            KrossWordCell *cell = at(coordTopLeft + Coord(x, y));
            if (cell->getCellType() != EmptyCellType && cell != excludedCell) {
                if (y != 0)
                    maxHeight = y;
                break;
            }
        }
    }
    for (int y = 0; y < maxHeight - 1; ++y) {
        for (int x = 0; x < maxWidth - 1; ++x) {
            KrossWordCell *cell = at(coordTopLeft + Coord(x, y));
            if (cell->getCellType() != EmptyCellType && cell != excludedCell) {
                maxWidth = x;
                break;
            }
        }
    }

    return QSize(maxWidth, maxHeight);
}

ErrorType KrossWord::insertImage(const KGrid2D::Coord &coord,
                                 int horizontalCellSpan, int verticalCellSpan, QUrl url,
                                 ErrorTypes errorTypesToIgnore, ImageCell **insertedImage)
{
    ErrorType errorType = canInsertImage(coord, horizontalCellSpan,
                                         verticalCellSpan, errorTypesToIgnore);
    if (errorType != ErrorNone)
        return errorType;

    ImageCell *imageCell = new ImageCell(this, coord,
                                         horizontalCellSpan, verticalCellSpan, url);

    for (int x = imageCell->coord().first;
            x < imageCell->coord().first + imageCell->horizontalCellSpan(); ++x) {
        for (int y = imageCell->coord().second;
                y < imageCell->coord().second + imageCell->verticalCellSpan(); ++y) {
            replaceCell(Coord(x, y), imageCell);
        }
    }

    if (insertedImage)
        *insertedImage = imageCell;

    return ErrorNone;
}

void KrossWord::assignClueNumbers()
{
    int curClueNumber = 0;

    //  Iterate through all cells
    for (uint y = 0; y < height(); ++y) {
        for (uint x = 0; x < width(); ++x) {
            LetterCell *letter = qgraphicsitem_cast<LetterCell*>(at(Coord(x, y)));

            if (!letter) {
                ClueCell *clue;
                DoubleClueCell *doubleClueCell;
                if ((clue = qgraphicsitem_cast<ClueCell*>(at(Coord(x, y)))))
                    clue->setClueNumber(curClueNumber++);
                else if ((doubleClueCell = qgraphicsitem_cast<DoubleClueCell*>(
                                               at(Coord(x, y))))) {
                    doubleClueCell->clue1()->setClueNumber(curClueNumber++);
                    doubleClueCell->clue2()->setClueNumber(curClueNumber++);
                }
            } else {
                bool assignedNumber = false;
                if (letter->clueHorizontal() && letter->clueHorizontal()->firstLetter() == letter
                        && letter->clueHorizontal()->answerOffset() == OnClueCell) {
                    letter->clueHorizontal()->setClueNumber(curClueNumber);
                    assignedNumber = true;
                }
                if (letter->clueVertical() && letter->clueVertical()->firstLetter() == letter
                        && letter->clueVertical()->answerOffset() == OnClueCell) {
                    letter->clueVertical()->setClueNumber(curClueNumber);
                    assignedNumber = true;
                }

                if (assignedNumber)
                    ++curClueNumber;
            }
        } // for x
    } // for y

    m_maxClueNumber = curClueNumber - 1;
}

ClueCell* KrossWord::findClueCell(const Coord& coord,
                                  Qt::Orientation orientation,
                                  AnswerOffset answerOffset) const
{
    KrossWordCell *cell = at(coord);

    ClueCell *clueCell;
    DoubleClueCell *doubleClueCell;
    if (cell->isLetterCell())   // For hidden clues
        return ((LetterCell*)cell)->clue(orientation);
    else if ((doubleClueCell = qgraphicsitem_cast<DoubleClueCell*>(cell)))
        return doubleClueCell->clue(answerOffset, orientation);
    else if ((clueCell = qgraphicsitem_cast<ClueCell*>(cell))
             && clueCell->orientation() == orientation
             && clueCell->answerOffset() == answerOffset)
        return clueCell;
    else
        return NULL; // Clue cell not found
}

ClueCellList KrossWord::clueCellsFromClueNumber(int clueNumber) const
{
    ClueCellList clueList = clues();

    ClueCellList ret;
    foreach(ClueCell * clueCell, clueList) {
        if (clueCell->clueNumber() == clueNumber) {
            ret << clueCell;
            if (ret.count() == 2)
                return ret;
        }
    }

    return ret;
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

bool KrossWord::write(const QString& fileName, QString* errorString,
                      WriteMode writeMode, FileFormat fileFormat,
                      const QByteArray &undoData)
{
    QFile file(fileName);

    if (fileFormat == DetermineByFileName) {
        fileFormat = fileFormatFromFileName(fileName);
        if (fileFormat == DetermineByFileName) {
            qDebug() << QString("File format unknown for file '%1'").arg(fileName);
            return false;
        }
    }

    if (fileFormat == KrossWordPuzzleXmlFile) {
        KrossWordXmlWriter xmlWriter;
        bool writeOk = xmlWriter.write(&file, this, writeMode, undoData);
        if (!writeOk) {
            *errorString = i18n("Error writing crossword: %1", xmlWriter.errorString());
            return false;
        }
    } else if (fileFormat == KrossWordPuzzleCompressedXmlFile) {
        KrossWordXmlWriter xmlWriter;
        bool writeOk = xmlWriter.writeCompressed(&file, this, writeMode, undoData);
        if (!writeOk) {
            *errorString = i18n("Error writing compressed crossword: %1",
                                xmlWriter.errorString());
            return false;
        }
    } else if (fileFormat == AcrossLitePuzFile) {
        if (!undoData.isEmpty())
            qDebug() << "Can't store undoData to *.puz-files";

        KrossWordPuzStream puzWriter;
        bool writeOk = puzWriter.write(&file, this, writeMode);
        if (!writeOk && errorString != NULL) {
            *errorString = i18n("Error writing AcrossLite's .puz-format.");
            return false;
        }
    } else
        return false;

    return true;
}

bool KrossWord::read(const QUrl &url, QString *errorString, QWidget *mainWindow,
                     FileFormat fileFormat, QByteArray *undoData)
{
    QFile file(url.path());
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorString != NULL) {
            *errorString = file.errorString();
        }
        qWarning() << file.errorString();
        return false;
    }

    setHighlightedClue(NULL);
    removeAllCells();
    bool wasBlocking = blockSignals(true);

    bool readOk = false, fileFormatDeterminationFailed = false;
    if (fileFormat == DetermineByFileName) {
        QString extension = QFileInfo(url.path()).suffix();
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
                readOk = xmlReader.read(&file, this, undoData);

                if (!readOk) {
                    if (file.isOpen()) file.seek(0);
                    readOk = xmlReader.readCompressed(&file, this, undoData);
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
            readOk = xmlReader.read(&file, this, undoData);
            if (!readOk && errorString)
                *errorString = xmlReader.errorString();
        } else if (fileFormat == KrossWordPuzzleCompressedXmlFile) {
            KrossWordXmlReader xmlReader;
            readOk = xmlReader.readCompressed(&file, this, undoData);
            if (!readOk && errorString)
                *errorString = xmlReader.errorString();
        }
    }
    file.close();
    blockSignals(wasBlocking);
    emit cluesAdded(clues());   // All clues are new

    return readOk;
}

QPixmap KrossWord::toPixmap(const QSize& size)
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

    QGraphicsScene *sc = scene();
    if (!sc) {
        sc = new QGraphicsScene();
        sc->addItem(this);
    }

    bool wasDrawingForPrinting = isDrawingForPrinting();
    setDrawForPrinting();
    QPixmap pix(usedSize);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHints(QPainter::HighQualityAntialiasing | QPainter::Antialiasing
                     | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    scene()->render(&p, QRectF(0, 0, usedSize.width(), usedSize.height()), boundingRect());
    p.end();
    setDrawForPrinting(wasDrawingForPrinting);

    if (sc) {
        sc->removeItem(this);
        delete sc;
    }

    return pix;
}

void KrossWord::solve()
{
//     removeSynchronization( KrossWordCell::SyncContent );
    // TODO: Sync manually afterwards...

    bool wasBlocked = blockSignals(true);
//     bool wasEnabled = isSignalAnswerChangedEnabled();
//     enableSignalAnswerChanged( false );

    for (uint x = 0; x < width(); ++x) {
        for (uint y = 0; y < height(); ++y) {
            KrossWordCell *cell = at(Coord(x, y));

            if (cell->isLetterCell())
                ((LetterCell*)cell)->solve();
        }
    }

//     enableSignalAnswerChanged( wasEnabled );
    blockSignals(wasBlocked);
}

bool KrossWord::check() const
{
    for (uint x = 0; x < width(); ++x) {
        for (uint y = 0; y < height(); ++y) {
            KrossWordCell *cell = at(Coord(x, y));
            if (cell->isLetterCell() && !((LetterCell*)cell)->isCorrect())
                return false;
        }
    }

    return true;
}

void KrossWord::clear()
{
    LetterCellList letterList = letters();
    foreach(LetterCell * letter, letterList)
    letter->clear();
}

void KrossWord::removeSolutionSynchronizationTo(KrossWord* solutionKrossWord)
{
    SolutionLetterCellList letters2 = solutionKrossWord->m_solutionLetters;

    foreach(SolutionLetterCell * letter1, m_solutionLetters) {
        SolutionLetterCell *letter2 = letters2[ letter1->solutionWordIndex()];
        letter1->removeSynchronizationWith(letter2, SyncAll,
                                           SolutionLetterSynchronization);
    }
}

void KrossWord::setTitle(const QString& title)
{
    m_title = title;
    updateHeaderItem();
}

void KrossWord::setAuthors(const QString& authors)
{
    m_authors = authors;
    updateHeaderItem();
}

void KrossWord::setCopyright(const QString& copyright)
{
    m_copyright = copyright;
    updateHeaderItem();
}

void KrossWord::setNotes(const QString& notes)
{
    m_notes = notes;
}

void KrossWord::updateHeaderItem()
{
    if (m_title.isEmpty()) {
        m_headerItem->setVisible(false);
    } else {
        m_headerItem->setTextWidth(boundingRect().width()); // max width
        m_headerItem->setDefaultTextColor(m_theme->fontColor());
        m_headerItem->setHtml(QString(i18n("<strong>%1</strong> by %2<br><small>%3</small>"))
                              .arg(getTitle())
                              .arg(getAuthors())
                              .arg(getCopyright()));
        m_headerItem->setVisible(true);
    }
}

KrossWordCellList KrossWord::cells(CellTypes cellTypes) const
{
    KrossWordCellList list;
    for (uint i = 0; i < m_krossWordGrid->size(); ++i) {
        KrossWordCell *cell = m_krossWordGrid->at(i);
        if (!cell) {
            continue;
        }

        if (cellTypes.testFlag(cell->getCellType()) && !list.contains(cell)) {
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
    const Offset letterOffset = orientation == Qt::Horizontal
                                ? Offset(1, 0) : Offset(0, 1);
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
    for (uint i = 0; i < m_krossWordGrid->size(); ++i) {
        ImageCell *cell = qgraphicsitem_cast<ImageCell*>(m_krossWordGrid->at(i));
        if (cell && !list.contains(cell))
            list << cell;
    }
    return list;
}

EmptyCellList KrossWord::emptyCells() const
{
    EmptyCellList list;
    for (uint i = 0; i < m_krossWordGrid->size(); ++i) {
        EmptyCell *cell = qgraphicsitem_cast<EmptyCell*>(m_krossWordGrid->at(i));
        if (cell)
            list << cell;
    }
    return list;
}

LetterCellList KrossWord::letters() const
{
    LetterCellList list;
    for (uint i = 0; i < m_krossWordGrid->size(); ++i) {
        KrossWordCell *cell = m_krossWordGrid->at(i);
        if (cell->isLetterCell())
            list << (LetterCell*)cell;
    }
    return list;
}

LetterCellList KrossWord::emptyLetters() const
{
    LetterCellList list;
    for (uint i = 0; i < m_krossWordGrid->size(); ++i) {
        KrossWordCell *cell = m_krossWordGrid->at(i);
        if (cell->isLetterCell()) {
            LetterCell *letter = (LetterCell*)cell;
            if (letter->isEmpty())
                list << letter;
        }
    }
    return list;
}

void KrossWord::clues(ClueCellList* horizontalClues,
                      ClueCellList* verticalClues) const
{
    Q_ASSERT(horizontalClues);
    Q_ASSERT(verticalClues);

    foreach(ClueCell * clue, m_clues) {
        if (clue->isHorizontal())
            *horizontalClues << clue;
        else
            *verticalClues << clue;
    }

    qSort(horizontalClues->begin(), horizontalClues->end(), lessThanClueNumber);
    qSort(verticalClues->begin(), verticalClues->end(), lessThanClueNumber);
}

QList< AnswerOffset > KrossWord::legalAnswerOffsets(
    Coord clueCellCoord, Qt::Orientation orientation,
    int answerLength, ClueCell *excludedClue) const
{
    KrossWordCell *cell = at(clueCellCoord);
//   qDebug() << "KrossWord::legalAnswerOffsets()  " << orientation << clueCellCoord;
//   qDebug() << "  excludedClue:" << excludedClue;
    QList< AnswerOffset > offsets;

    // If adding a clue at coordinates containing a letter cell or if clue cells
    // are disallowed by the current crossword type, the clue cell must be hidden
    // (ie. the answer offset must be OnClueCell).
    if ((cell->isLetterCell() && !((LetterCell*)cell)->isAttachedToClueExclusivly(excludedClue))
            || m_crosswordTypeInfo.clueCellHandling == ClueCellsDisallowed) {
//     qDebug() << "Clue cells need to be hidden";
        offsets << OnClueCell;
    } else {
        offsets = ClueCell::allAnswerOffsets();

        // Remove OnClueCell if clue cells are required
        if (m_crosswordTypeInfo.clueCellHandling == ClueCellsRequired) {
//       qDebug() << "OnClueCell isn't legal because Clue cells are required";
            offsets.removeOne(OnClueCell);
        }

        if (orientation == Qt::Horizontal)
            offsets.removeOne(OffsetLeft);
        else
            offsets.removeOne(OffsetTop);

        // Don't allow answers to start outside the grid
        if (clueCellCoord.first == 0) {
            offsets.removeOne(OffsetTopLeft);
            offsets.removeOne(OffsetLeft);
            offsets.removeOne(OffsetBottomLeft);
        } else if ((uint)clueCellCoord.first == width() - 1) {
            offsets.removeOne(OffsetTopRight);
            offsets.removeOne(OffsetRight);
            offsets.removeOne(OffsetBottomRight);
        }

        if (clueCellCoord.second == 0) {
            offsets.removeOne(OffsetTopLeft);
            offsets.removeOne(OffsetTop);
            offsets.removeOne(OffsetTopRight);
        } else if ((uint)clueCellCoord.second == height() - 1) {
            offsets.removeOne(OffsetBottomLeft);
            offsets.removeOne(OffsetBottom);
            offsets.removeOne(OffsetBottomRight);
        }

        ClueCell *clueCell = qgraphicsitem_cast<ClueCell*>(cell);
        if (clueCell && clueCell != excludedClue) {   // When creating a double clue cell
            //     qDebug() << "Need to create a double clue cell, OnClueCell isn't legal";
            offsets.removeOne(OnClueCell);

            // If the clue has the same orientation like the new clue
            // they can't have the same answer offset
            if (clueCell->orientation() == orientation)
                offsets.removeOne(clueCell->answerOffset());
        }
    }

    KrossWordCell *cellAtPos;
    Offset offset = KrosswordGrid::letterOffset(orientation);
    for (int i = offsets.count() - 1; i >= 0; --i) {
        Coord letterCoord = clueCellCoord +
                            ClueCell::answerOffsetToOffset(offsets[i]);

        for (int n = 0; n < answerLength; ++n) {
            if (!inside(letterCoord)) {
                offsets.removeAt(i);   // Doesn't fit into the grid
                break;
            }

            cellAtPos = at(letterCoord);
            if (!cellAtPos) {
                letterCoord += offset;
                continue;
            }

//       qDebug() << "    " << offsets[i] << n << cellAtPos;
            if (cellAtPos->getCellType() != EmptyCellType
                    && !cellAtPos->isLetterCell()) {
                if (cellAtPos != excludedClue) {   // TEST
                    // Only allow empty or letter cells as cells for the first answer cell
//    qDebug() << offsets[ i ] << "isn't legal, because there isn't an empty "
//           "or letter cell";
                    offsets.removeAt(i);
                    break;
                }
            } else if (cellAtPos->isLetterCell()) {
                // Disallow crossed letter cells
                LetterCell *letter = (LetterCell*)cellAtPos;
                if (letter->isCrossed()) {
                    if (letter->isAttachedToClue(excludedClue)) {
                        // Letter isn't crossed when excluding excludedClue.
                        // Remove current offset if the orientation of the new clue is the
                        // same as the other clue of [letter] (ie. other than the orientation
                        // of [excludedClue]).
                        if (orientation != excludedClue->orientation()) {
//        qDebug() << offsets[ i ] << "isn't legal, because there is a letter "
//     "cell with a not excluded clue in the given orientation";
                            offsets.removeAt(i);
                            break;
                        }
                    } else {
//      qDebug() << offsets[ i ] << "isn't legal, because there is a crossed "
//   "letter cell";
                        offsets.removeAt(i);
                        break;
                    }
                } else if (orientation == letter->clue()->orientation()
                           && letter->clue() != excludedClue) {
//    qDebug() << offsets[ i ] << "isn't legal, because there is an uncrossed "
//      "letter cell with a not excluded clue in the given orientation";
                    offsets.removeAt(i);
                    break;
                }
            }

            letterCoord += offset;
        } // for 0 <= n < answerLength
    } // for i.. answer offsets

    return offsets;
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
            if (at(curCoord)->getCellType() != EmptyCellType &&
                    !isCellEmptyIfSpannedCellIsExcluded(curCoord, excludedSpannedCell))
                return false;
        }
    }

    return true;
}

bool KrossWord::isCellEmptyIfClueIsExcluded(const Coord& coord, ClueCell* excludedClue) const
{
    if (!excludedClue) {
        return false;
    }

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
    switch (cellAtCoord->getCellType()) {
    case EmptyCellType:
        // Clue can be added here
        return true;
        break;

    case LetterCellType:
    case SolutionLetterCellType:
        if ((firstLetterOffset != Offset(0, 0) && !cellIsEmptyIfClueIsExcluded)
                || (((LetterCell*)cellAtCoord)->isCrossed()
                    && !((LetterCell*)cellAtCoord)->clues().contains(excludedClue))) {
//   qDebug() << "Can only add clue's with hidden clue cells at coordinates containing letter cells";
            return false;
        }
        break;

    case ClueCellType:
        if ((!allowDoubleClueCells || !m_crosswordTypeInfo.cellTypes.testFlag(DoubleClueCellType))
                && !cellIsEmptyIfClueIsExcluded) {
            qDebug() << "Double clue cells not allowed";
            return false;
        }
        break;

    case DoubleClueCellType:
        if (!((DoubleClueCell*)cellAtCoord)->hasClue(excludedClue)) {
            qDebug() << "Can't add clue cells to double clue cells";
            return false;
        }
        break;

    default:
//      qDebug() << "Can't add clue cells at coordinates containing cells with type"
//    << KrossWordCell::displayStringFromCellType( cellAtCoord->cellType() );
        return false;
        break;
    }

    return true;
}

ErrorType KrossWord::canInsertImage(const KGrid2D::Coord& coord,
                                    int horizontalCellSpan, int verticalCellSpan,
                                    ErrorTypes errorTypesToIgnore, ImageCell* excludedImage)
{
    if (!m_crosswordTypeInfo.cellTypes.testFlag(ImageCellType)) {
        return ErrorImageCellsDisallowed;
    }

    // Check if the image will fit into the grid
    if (!errorTypesToIgnore.testFlag(ErrorImageDoesntFit)
            && (!inside(coord) || !inside(coord + Offset(horizontalCellSpan - 1, verticalCellSpan - 1)))) {
        qDebug() << "Image doesn't fit into the grid" << coord
                 << horizontalCellSpan << verticalCellSpan;
        return ErrorImageDoesntFit;
    }

    // Check if the cells for the new image are empty
    if (!canTakeSpannedCell(coord, horizontalCellSpan, verticalCellSpan, excludedImage)) {
        return ErrorImageCellsArentEmpty;
    }

    return ErrorNone;
}

ErrorType KrossWord::canChangeClueProperties(ClueCell* clue,
        Qt::Orientation newOrientation, Offset newAnswerOffset,
        const QString &newCorrectAnswer,
        ErrorTypes errorTypesToIgnore, bool allowDoubleClueCells,
        SolutionLetterCellList *removedSolutionLetterCells)
{
    // Check if the given clue can be inserted with the new settings
    // if it wouldn't already be in the grid
    ErrorType errorType = canInsertClue(
                              clue->coord(), newOrientation, newAnswerOffset, clue->correctAnswer(), /*newCorrectAnswer,*/
                              errorTypesToIgnore, allowDoubleClueCells, clue);

    if (removedSolutionLetterCells) {
        QList< Coord > oldCoords = ClueCell::answerCoordList(clue->coord(),
                                   clue->answerOffset(), clue->orientation(), clue->answerLength());
        QList< Coord > newCoords = ClueCell::answerCoordList(clue->coord(),
                                   ClueCell::offsetToAnswerOffset(newAnswerOffset), newOrientation,
                                   newCorrectAnswer.length());

        // All solution letter cells that get moved to a cell already containing
        // a solution letter cell will get removed by changeClueProperties().
        for (int i = 0; i < qMin(oldCoords.length(), newCoords.length()); ++i) {
            SolutionLetterCell *solutionLetterCell =
                qgraphicsitem_cast< SolutionLetterCell* >(at(oldCoords[i]));

            if (solutionLetterCell && at(newCoords[i])->isType(SolutionLetterCellType))
                removedSolutionLetterCells->append(solutionLetterCell);
        }
    }

    return errorType;
}

ErrorType KrossWord::changeClueProperties(ClueCell* clue,
        Qt::Orientation newOrientation, AnswerOffset newAnswerOffset,
        const QString &newCorrectAnswer,
        ErrorTypes errorTypesToIgnore, bool allowDoubleClueCells)
{
    bool changeOrientation = clue->orientation() != newOrientation;
    bool changeAnswerOffset = clue->answerOffset() != newAnswerOffset;
    bool changeCorrectAnswer = clue->correctAnswer() != newCorrectAnswer;
    if (!changeOrientation && !changeAnswerOffset && !changeCorrectAnswer) {
        qDebug() << "No changes";
        return ErrorNone; // Nothing to change
    }

    // Check if the given clue can be inserted if it wouldn't already be in the grid
    Offset offset = ClueCell::answerOffsetToOffset(newAnswerOffset);
    ErrorType errorType = canChangeClueProperties(clue,
                          newOrientation, offset, newCorrectAnswer,
                          errorTypesToIgnore, allowDoubleClueCells);
    if (errorType != ErrorNone) {
        return errorType; // Can't change clue properties
    }

    clue->setCorrectAnswer(newCorrectAnswer);

    SolutionLetterCellList solLettersBefore = solutionWordLetters(); // TODO
    bool wasHighlighted = clue->isHighlighted();
    clue->setHighlight(false);
    LetterCellList letterList = clue->letters();
    clue->beginAddLetters();
    bool signalsBlocked = blockSignals(true);   // TODO Shouldn't block solution letter cell removed

    // FIXME: "QGraphicsScene:removeItem: cannot remove 0-item" occurs here:
    QList<Coord> removedCoords = removeClue(clue, newAnswerOffset == OnClueCell
                                            ? RemoveFromGrid : DontRemove,
                                            RemoveFromGrid);
    // FIXME: DOUBLE CLUES IN THE CLUE LIST! WHEN ONCLUECELL

    // Delete empty cell and reinsert clue cell when the answer offset changed
    // from OnClueCell or combine existing clue cell with the new clue cell to a
    // double clue cell
    Coord coord = clue->coord();
    if (newAnswerOffset != OnClueCell && clue->answerOffset() == OnClueCell) {
        if (!clue->scene()) {
            scene()->addItem(clue);
        }

        KrossWordCell *cellAtCoord = at(coord);
        if (cellAtCoord->isType(ClueCellType)) {
            DoubleClueCell *doubleClueCell = new DoubleClueCell(
                this, coord, qgraphicsitem_cast<ClueCell*>(cellAtCoord), clue);
            (*m_krossWordGrid)[ coord ] = doubleClueCell;
        } else {
            replaceCell(coord, clue);
        }
    } else if (newAnswerOffset == OnClueCell && clue->scene()) {
        clue->scene()->removeItem(clue);
    }

    // Insert/attach/move letter cells for the answer
    QString answerUpper = newCorrectAnswer.toUpper();
    Coord letterCoord = coord + offset;
    const Offset letterOffset = newOrientation == Qt::Horizontal
                                ? Offset(1, 0) : Offset(0, 1);
    int i;
    for (i = 0; i < answerUpper.length(); ++i) {
        KrossWordCell *targetCell = at(letterCoord);

        if (i < letterList.count()) {
            LetterCell *oldLetter = letterList[ i ];
            if (removedCoords.indexOf(oldLetter->coord()) >= 0) {
                // Letter cell wasn't crossed and therefore removed from the grid
                if (targetCell->isType(EmptyCellType)) {
                    // Put letter into grid at new coords
                    oldLetter->setCoord(letterCoord);   // Put letter into grid at new coords
                    oldLetter->attachClue(clue);
                } else if (targetCell->isLetterCell()) {
                    // Add clue to existing letter cell and delete old letter
                    LetterCell *targetLetterCell = (LetterCell*)targetCell;
                    targetLetterCell->setClue(clue, newOrientation);   // Add clue to existing letter cell
                    targetLetterCell->setPropertiesFrom(oldLetter);

                    if (oldLetter->isType(SolutionLetterCellType)) {
                        if (!targetLetterCell->isType(SolutionLetterCellType)) {
                            // Convert new letter to solution letter
                            targetLetterCell->toSolutionLetter(
                                qgraphicsitem_cast<SolutionLetterCell*>(oldLetter)->solutionWordIndex());
                        }
                    }

                    if (isAnimationEnabled()) {
                        QAbstractAnimation *anim = animator()->animate(
                                                       Animator::AnimatePositionChange, oldLetter, targetCell->pos(),
                                                       Animator::VerySlow, Animator::AnimateDontStart);

                        oldLetter->setFlag(ItemIsFocusable, false);
                        connect(anim, SIGNAL(finished()), oldLetter,
                                SLOT(deleteAndRemoveFromSceneLater()));
                        animator()->startOrEnqueue(anim);
                    } else {
                        delete oldLetter;
                    }
                }
            } else {
                // Letter cell was crossed, so create a new one
                if (targetCell->isType(EmptyCellType)) {
                    // Letter cell was crossed, create a new one
                    KrossWordCell *newCell = new LetterCell(this, letterCoord, clue, newAnswerOffset);
                    replaceCell(letterCoord, newCell);
                    ((LetterCell*)newCell)->setPropertiesFrom(oldLetter);
                } else if (targetCell->isLetterCell()) {
                    // Letter cell was crossed, add clue to existing letter cell
                    ((LetterCell*)targetCell)->setClue(clue, newOrientation);
                    ((LetterCell*)targetCell)->setPropertiesFrom(oldLetter);
                }
            }
        } else {
            if (targetCell->isType(EmptyCellType)) {
                // New letter on empty cell
                KrossWordCell *newCell = new LetterCell(this, letterCoord, clue, newAnswerOffset);
                replaceCell(letterCoord, newCell);
            } else if (targetCell->isLetterCell()) {
                // New letter on clue cell
                ((LetterCell*)targetCell)->setClue(clue, newOrientation);      // Add clue to existing letter cell
            }
        }

        letterCoord = letterCoord + letterOffset;
    }

    while (i < letterList.count()) {
        LetterCell *letter = letterList[ i ];

        // Remove cell from scene
        if (letter->scene()) {
            letter->scene()->removeItem(letter);
        }

        // Delete old cell
        letter->setFlag(ItemIsFocusable, false);
        letter->deleteLater();
        ++i;
    }

    blockSignals(signalsBlocked);
    clue->endAddLetters(newOrientation, newAnswerOffset);
    clue->setHighlight(wasHighlighted);
    clue->setCorrectAnswer(newCorrectAnswer);

//   qDebug() << "AFTER CHANGE" << this;
    insertCluePostProcessing(clue);

    SolutionLetterCellList solLettersAfter = solutionWordLetters(); // TODO
    foreach(SolutionLetterCell * solLetterCell, solLettersBefore) {
        if (!solLettersAfter.contains(solLetterCell)) {
            emit solutionWordLetterRemoved(solLetterCell);
        }
    }
    foreach(SolutionLetterCell * solLetterCell, solLettersAfter) {
        if (!solLettersBefore.contains(solLetterCell)) {
            emit solutionWordLetterAdded(solLetterCell);
        }
    }

    return ErrorNone;
}

ErrorType KrossWord::canInsertClue(const KGrid2D::Coord& coord,
                                   Qt::Orientation orientation, const Offset &answerOffset,
                                   const QString& answer, ErrorTypes errorTypesToIgnore,
                                   bool allowDoubleClueCells, ClueCell *excludedClue)
{
    if (m_crosswordTypeInfo.clueCellHandling == ClueCellsDisallowed && answerOffset != Offset(0, 0)) {
        return ErrorClueCellsDisallowed;
    } else if (m_crosswordTypeInfo.clueCellHandling == ClueCellsRequired && answerOffset == Offset(0, 0)) {
        return ErrorClueCellsRequired;
    }

    if (answer.length() < m_crosswordTypeInfo.minAnswerLength) {
        return ErrorAnswerIsTooShort;
    }

    // Check if the clue and the answer will fit into the grid
    if (!errorTypesToIgnore.testFlag(ErrorClueDoesntFit)
            && (!inside(coord) || !inside(coord + answerOffset)
                || (orientation == Qt::Horizontal
                    && !inside(coord + answerOffset + KGrid2D::Coord(answer.length() - 1, 0)))
                || (orientation == Qt::Vertical
                    && !inside(coord + answerOffset + KGrid2D::Coord(0, answer.length() - 1))))) {
        qDebug() << "Clue doesn't fit into the grid" << coord << answerOffset << answer
                 << "orientation =" << orientation << "answer-length =" << answer.length();
        return ErrorClueDoesntFit;
    }

    // Check if the orientation and answer offset combination isn't illegal
    if ((orientation == Qt::Horizontal && answerOffset == Offset(-1, 0))
            || (orientation == Qt::Vertical && answerOffset == Offset(0, -1))) {
        qDebug() << "ErrorAnswerOverwritesClueCell";
        return ErrorAnswerOverwritesClueCell;
    }

    // Check if the cell for the new clue is empty
    // or if it is a ClueCell and double clue cells are allowed
    if (!canTakeClueCell(coord, answerOffset, allowDoubleClueCells, excludedClue))
        return ErrorClueCellIsntEmpty;

    bool cellIsEmptyIfClueIsExcluded = isCellEmptyIfClueIsExcluded(coord,
                                       excludedClue);

    // Check if the answers letter cells are legal
    QString answerUpper = answer.toUpper();
    Coord letterCoord = coord + answerOffset;
    const Offset letterOffset = orientation == Qt::Horizontal
                                ? Offset(1, 0) : Offset(0, 1);
    for (int i = 0; i < answerUpper.length(); ++i) {
        if (!errorTypesToIgnore.testFlag(
                    ErrorAnswerContainsIllegalCharacters)
                && !m_crosswordTypeInfo.isCharacterLegal(answerUpper[i])
                && answerUpper[i] != ' ') {
//       && !ALLOWED_CHARACTERS.contains(answerUpper[i]) && answerUpper[i] != ' ' ) {
            qDebug() << "Illegal character" << answerUpper[i];
            return ErrorAnswerContainsIllegalCharacters;
        }

        if (!cellIsEmptyIfClueIsExcluded) {
            KrossWordCell *oldCell = at(letterCoord);
            if (!errorTypesToIgnore.testFlag(ErrorAnswerCrossesClueCell)
                    && oldCell->isType(ClueCellType)) {
                qDebug() << "Answer crosses a clue cell at" << letterCoord
                         /*<< clue*/ << answer << "crossed clue:"
                         << qgraphicsitem_cast<ClueCell*>(oldCell)->clue();
                return ErrorAnswerCrossesClueCell;
            } else if (oldCell->isLetterCell()) {
                LetterCell *letterCell = (LetterCell*)oldCell;
                if (!errorTypesToIgnore.testFlag(ErrorAnswerIsIllegal)
                        && letterCell->correctLetter() != answerUpper.at(i)) {
                    qDebug() << "Answer is illegal in the current crossword at" << letterCoord
                             /*<< clue*/ << answer << "wanted letter:" << i << answerUpper.at(i)
                             << "letter from other clue:" << letterCell->correctLetter() << endl
                             << "for clue at" << coord << "answer offset" << answerOffset
                             << "other clue is" << letterCell->clues().first()->clue()
                             << "at" << letterCell->clues().first()->coord()
                             << "with answer offset" << letterCell->clues().first()->firstLetterOffset();
                    return ErrorAnswerIsIllegal;
                } else if (!errorTypesToIgnore.testFlag(ErrorAnswerOverwritesClueInSameOrientation)
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

    return ErrorNone;
}

ErrorType KrossWord::insertClue(const Coord &coord,
                                Qt::Orientation orientation,
                                AnswerOffset answerOffset,
                                const QString &clue, const QString &answer,
                                CellType cellType,
                                ErrorTypes errorTypesToIgnore, bool allowDoubleClueCells,
                                ClueCell **insertedClue)
{
    Q_ASSERT(cellType == LetterCellType
             || cellType == SolutionLetterCellType);

    Offset offset = ClueCell::answerOffsetToOffset(answerOffset);
    ErrorType errorType = canInsertClue(coord, orientation, offset,
                                        answer, errorTypesToIgnore,
                                        allowDoubleClueCells);
    if (errorType != ErrorNone) {
        if (insertedClue)
            *insertedClue = NULL;
        return errorType;
    }

    // Delete empty cell and insert new clue cell
    // or combine existing clue cell with the new clue cell to a double clue cell
    QString answerUpper = answer.toUpper();
    ClueCell *clueCell = new ClueCell(this, coord, orientation,
                                      answerOffset, clue, answerUpper);
    clueCell->beginAddLetters();
    if (answerOffset != OnClueCell) {
        KrossWordCell *cellAtCoord = at(coord);
        if (cellAtCoord->isType(ClueCellType)) {
            DoubleClueCell *doubleClueCell = new DoubleClueCell(
                this, coord, qgraphicsitem_cast<ClueCell*>(cellAtCoord), clueCell);
            (*m_krossWordGrid)[ coord ] = doubleClueCell;
        } else
            replaceCell(coord, clueCell);
    }

    // Insert letter cells for the answer
    Coord letterCoord = coord + offset;
    const Offset letterOffset = orientation == Qt::Horizontal
                                ? Offset(1, 0) : Offset(0, 1);
    for (int i = 0; i < answerUpper.length(); ++i) {
        KrossWordCell *oldCell = at(letterCoord);

        if (oldCell->isType(EmptyCellType)) {
            // Replace empty cell with new letter cell
            KrossWordCell *newCell = NULL;
            if (cellType == LetterCellType)
                newCell = new LetterCell(this, letterCoord, clueCell);
            else if (cellType == SolutionLetterCellType)
                newCell = new SolutionLetterCell(this, letterCoord, clueCell, i);

            if (newCell)
                replaceCell(letterCoord, newCell);
        } else if (oldCell->isLetterCell())
            ((LetterCell*)oldCell)->setClue(clueCell);      // Add clue to existing letter cell

        letterCoord = letterCoord + letterOffset;
    }

    clueCell->endAddLetters();
    if (insertedClue)
        *insertedClue = clueCell;

    insertCluePostProcessing(clueCell);
    return ErrorNone;
}

void KrossWord::insertCluePostProcessing(ClueCell* clue)
{
    // Assign clue numbers
    assignClueNumbers();

    // Add clue to clue list and emit cluesAdded signal
    if (!m_clueExpanderItems.contains(clue)) {
        // Add expander item
        ClueExpanderItem *expanderItem = new ClueExpanderItem(this, clue);
        m_clueExpanderItems.insert(clue, expanderItem);
        connect(expanderItem, SIGNAL(addLettersToClueRequest(ClueCell*, int)),
                this, SLOT(addLettersToClueRequestSlot(ClueCell*, int)));
    }
    if (!m_clues.contains(clue)) {
        m_clues << clue;
        emit cluesAdded(QList<ClueCell*>() << clue);
    }

    // Highlight and focus new (or changed) clue
    setHighlightedClue(clue);
    setCurrentCell(clue);
    clue->setFocus(Qt::OtherFocusReason);
}

QList<Coord> KrossWord::removeClue(ClueCell* clueCell,
                                   RemoveMode clueCellRemoveMode,
                                   RemoveMode letterCellsRemoveMode)
{
    Q_ASSERT(clueCell);

    if (clueCellRemoveMode != DontRemove) {
        if (clueCellRemoveMode == RemoveFromGridAndDelete)
            emit cluesAboutToBeRemoved(ClueCellList() << clueCell);

        // m_highlightedClue shouldn't point to a deleted/removed clue cell
        if (clueCell == m_highlightedClue)
            setHighlightedClue(NULL);
    }

    QList<Coord> removedCoords;
    if (letterCellsRemoveMode != DontRemove) {
        LetterCellList letters = clueCell->letters();
        foreach(LetterCell * cell, letters) {
            if (!cell->isCrossed()) {
                removedCoords << cell->coord();
                removeCell(cell->coord(), letterCellsRemoveMode == RemoveFromGridAndDelete);
            }
            cell->detachClue(clueCell);
        }
    }

    if (clueCellRemoveMode != DontRemove) {
        DoubleClueCell *doubleClueCell;
        if (clueCell->answerOffset() == OnClueCell) {
            if (clueCell->scene())
                clueCell->scene()->removeItem(clueCell);

            if (clueCellRemoveMode == RemoveFromGridAndDelete) {
                clueCell->setFlag(ItemIsFocusable, false);
                clueCell->deleteLater();
            }
        } else if ((doubleClueCell = qgraphicsitem_cast<DoubleClueCell*>(
                                         clueCell->parentItem()))) {
            if (clueCellRemoveMode == RemoveFromGridAndDelete)
                doubleClueCell->removeClueCell(clueCell);
            else
                doubleClueCell->takeClueCell(clueCell);
        } else {
            removedCoords << clueCell->coord();
            removeCell(clueCell->coord(), clueCellRemoveMode == RemoveFromGridAndDelete);
        }

        if (clueCellRemoveMode == RemoveFromGridAndDelete) {
            ClueExpanderItem *clueExpander = m_clueExpanderItems[ clueCell ];
            if (clueExpander) {
                scene()->removeItem(clueExpander);
                m_clueExpanderItems.remove(clueCell);
                clueExpander->deleteLater();
            }

            m_clues.removeOne(clueCell);
        }
    }

    return removedCoords;
}

void KrossWord::removeImage(ImageCell* imageCell)
{
    Q_ASSERT(imageCell);

    removeCell(imageCell);
}

void KrossWord::removeCell(KrossWordCell* cell)
{
    removeCell(cell->coord());
}

void KrossWord::removeCell(const KGrid2D::Coord& coord)
{
    replaceCell(coord, new EmptyCell(this, coord));
}

void KrossWord::removeCell(const KGrid2D::Coord& coord, bool deleteOldCell)
{
    replaceCell(coord, new EmptyCell(this, coord), deleteOldCell);
}

void KrossWord::replaceCell(KrossWordCell* cell, KrossWordCell* newCell)
{
    replaceCell(cell->coord(), newCell);
}

void KrossWord::replaceCell(const Coord& coord,
                            KrossWordCell* newCell, bool deleteOldCell)
{
    Q_ASSERT(newCell);

    KrossWordCell *oldCell = at(coord);
//   qDebug() << "KrossWord::replaceCell(): oldCell =" << oldCell;
    SpannedCell *spannedCell;
    SolutionLetterCell *solutionLetterCell;
    ClueCell *clueCell;
    Q_ASSERT(oldCell);

    // Remove solution letter cells from the list
    if ((solutionLetterCell = qgraphicsitem_cast<SolutionLetterCell*>(oldCell))) {
        m_solutionLetters.removeOne(solutionLetterCell);
        emit solutionWordLetterRemoved(solutionLetterCell);
        // Replace "other" cells (not at oldCell->coord()) with empty cells.
        // All those "other" cells point to the same spanned cell.
    } else if ((spannedCell = dynamic_cast<SpannedCell*>(oldCell))) {
        if (spannedCell->inside(coord)) {
            Coord topLeft = spannedCell->coordTopLeft();
            Coord bottomRight = spannedCell->coordBottomRight();
            for (int x = topLeft.first; x <= bottomRight.first; ++x) {
                for (int y = (x == topLeft.first ? topLeft.second + 1 : topLeft.second);
                        y <= bottomRight.second; ++y) {
                    EmptyCell *emptyCell = new EmptyCell(this, Coord(x, y));
                    (*m_krossWordGrid)[ Coord(x, y)] = emptyCell;

                    if (isAnimationEnabled()) {
                        animator()->animate(Animator::AnimateFadeIn, emptyCell);
                    } else {
                        emptyCell->setOpacity(1);
                    }
                }
            }
        }
        // Remove highlight from highlighted clues to be removed
        // and remove clue expander from all clues to be removed
    } else if ((clueCell = qgraphicsitem_cast<ClueCell*>(oldCell))) {
        if (clueCell == m_highlightedClue) {
            m_highlightedClue = NULL;
        }

        ClueExpanderItem *clueExpander = m_clueExpanderItems[ clueCell ];
        if (clueExpander) {
            scene()->removeItem(clueExpander);
            clueExpander->deleteLater();
            m_clueExpanderItems.remove(clueCell);
        }
    }

    // Insert new cell
    bool newCellMoving = false;
    (*m_krossWordGrid)[ coord ] = newCell;
    if (!(spannedCell = dynamic_cast<SpannedCell*>(newCell))
            || spannedCell->coordTopLeft() == coord) {  // Needed to not crash or
        // wrongly move spanned cells when adding spanned cells because they
        // are referenced by all contained cells with m_coord pointing to the
        // top left cell.
        if (isAnimationEnabled()) {
            animator()->animate(Animator::AnimateFadeIn, newCell);
        } else {
            newCell->show();
            newCell->setOpacity(1);
        }

        newCell->setCoord(coord, false);
        newCellMoving = newCell->setPositionFromCoordinates();
    }

    // Add solution letter cells to the list
    if (newCell->isType(SolutionLetterCellType)) {
        m_solutionLetters.append((SolutionLetterCell*)newCell);
        qSort(m_solutionLetters.begin(), m_solutionLetters.end(), lessThanSolutionWordIndex);
        emit solutionWordLetterAdded((SolutionLetterCell*)newCell);
    }

    if (isAnimationEnabled() && !(dynamic_cast<SpannedCell*>(oldCell))) {
        QAbstractAnimation *anim = animator()->animate(Animator::AnimateFadeOut, oldCell,
                                   newCellMoving ? Animator::VerySlow : Animator::DefaultDuration,
                                   Animator::AnimateDontStart);

        if (newCellMoving) {
            oldCell->setZValue(-100);    // Set to back so that the new cell moves over the old one
            QVariantAnimation *varAnim = dynamic_cast< QVariantAnimation* >(anim);
            if (varAnim) {
                varAnim->setEasingCurve(QEasingCurve(QEasingCurve::InQuart));
            }
        } else {
            oldCell->setZValue(100);   // Set on top when fading out
        }

        if (anim) {
            animator()->startOrEnqueue(anim);
        }

        if (deleteOldCell) {
            if (anim) {   // Delete old cell after the animation
                oldCell->setFlag(ItemIsFocusable, false);
                connect(anim, SIGNAL(finished()), oldCell, SLOT(deleteAndRemoveFromSceneLater()));
            } else {
                oldCell->deleteAndRemoveFromSceneLater();
            }
        }
    } else {
        if (deleteOldCell) {
            oldCell->deleteAndRemoveFromSceneLater();
        }
    }
}

KrossWordCellList KrossWord::invalidateCell(const Coord& coord, bool simulate)
{
    KrossWordCellList removedCells;
    KrossWordCell *oldCell = at(coord);

    if (!oldCell)
        return removedCells; // Cell at coord is already NULL, ie. invalidated

//   qDebug() << "invalidate cell" << simulate << oldCell;
    SpannedCell *spannedCell;
    ClueCell *clueCell;
    DoubleClueCell *doubleClueCell;
    LetterCell *letterCell;

    // Remove highlight from highlighted clues to be removed
    if (!simulate && oldCell == m_highlightedClue)
        m_highlightedClue = NULL;

    // Remove solution letter cells from the list
    if ((spannedCell = dynamic_cast<SpannedCell*>(oldCell))) {
        if (!removedCells.contains(spannedCell))
            removedCells << spannedCell;

        if (!simulate) {
            Coord topLeft = spannedCell->coordTopLeft();
            Coord bottomRight = spannedCell->coordBottomRight();
            for (int xSpanned = topLeft.first; xSpanned <= bottomRight.first; ++xSpanned) {
                for (int ySpanned = topLeft.second; ySpanned <= bottomRight.second; ++ySpanned) {
//    if ( xSpanned >= coordTopLeft.first
//   && xSpanned <= coordBottomRight.first
//   && ySpanned >= coordTopLeft.second
//   && ySpanned <= coordBottomRight.second ) {
                    (*m_krossWordGrid)[ Coord(xSpanned, ySpanned)] = NULL;
//    } else {
//      (*m_krossWordGrid)[ Coord(xSpanned, ySpanned) ] =
//     new EmptyCell( this, Coord(xSpanned, ySpanned) );
//    }
                }
            }
        } // if ( !simulate )
    } else if ((clueCell = qgraphicsitem_cast<ClueCell*>(oldCell))) {
        if (!removedCells.contains(clueCell))
            removedCells << clueCell;

        if (!simulate) {
            QList<Coord> removedCoords = removeClue(clueCell);
            foreach(const Coord & coord, removedCoords) {
                KrossWordCell *cell = at(coord);
                if (cell) {
                    qDebug() << "Delete cell" << cell;
                    if (cell->scene())
                        cell->scene()->removeItem(cell);
                    (*m_krossWordGrid)[ coord ] = NULL;
                    cell->deleteLater();
                }
            }
        } // if ( !simulate )
    } else if ((doubleClueCell = qgraphicsitem_cast<DoubleClueCell*>(oldCell))) {
        if (!removedCells.contains(doubleClueCell))
            removedCells << doubleClueCell;

        if (simulate) {
            foreach(ClueCell * clue, doubleClueCell->clues())
            removedCells << clue;
        } else {
            foreach(ClueCell * clue, doubleClueCell->clues()) {
                removedCells << clue;
                QList<Coord> removedCoords = removeClue(clue);
                foreach(const Coord & coord, removedCoords) {
                    KrossWordCell *cell = at(coord);
                    if (cell) {
                        qDebug() << "Delete cell" << cell;
                        if (cell->scene())
                            cell->scene()->removeItem(cell);
                        (*m_krossWordGrid)[ coord ] = NULL;
                        cell->deleteLater();
                    }
                }
            }
        } // if ( !simulate )
    } else if (oldCell->isLetterCell()
               && (letterCell = (LetterCell*)oldCell)) {
        // is done in removeClue => removeCell
//     if ( !simulate ) {
//       if ( oldCell->isType(SolutionLetterCellType) ) {
//  m_solutionLetters.removeOne( (SolutionLetterCell*)oldCell );
//  emit solutionWordLetterRemoved( (SolutionLetterCell*)oldCell );
//       }
//     }

        foreach(ClueCell * const & clueCell, letterCell->clues()) {  // krazy:exclude=foreach
            if (!removedCells.contains(clueCell))
                removedCells << clueCell;

            if (!simulate) {
                QList<Coord> removedCoords = removeClue(clueCell);
                foreach(const Coord & coord, removedCoords) {
                    KrossWordCell *cell = at(coord);
                    if (cell) {
                        qDebug() << "Delete cell" << cell;
                        if (cell->scene())
                            cell->scene()->removeItem(cell);
                        (*m_krossWordGrid)[ coord ] = NULL;
                        cell->deleteLater();
                    }
                }
            } // if ( !simulate )
        } // foreach( clueCell, letterCell->clues() )
    } else
        removedCells << oldCell;

    if (!simulate) {
        removeCell(oldCell);   // Call removeCell to get fade out animation
        KrossWordCell *newEmptyCell = at(coord);   // there is now an EmptyCell from removeCell

        // Invalidate cell, by setting it to NULL
        (*m_krossWordGrid)[ coord ] = NULL;
//     qDebug() << "Cell at" << coord << "is now NULL";

        // Remove cell from scene
//     if ( oldCell->scene() )
//       oldCell->scene()->removeItem( oldCell );

        // Delete old cell
//     oldCell->setFlag( ItemIsFocusable, false );
        /*delete *//*oldCell->deleteLater();*/ // TODO: use deleteLater()?
        if (newEmptyCell) {
            if (newEmptyCell->scene())
                newEmptyCell->scene()->removeItem(newEmptyCell);
            newEmptyCell->deleteLater();
        }
    } // if ( !simulate )

//   qDebug() << "  END: invalidate cell" << this;
    return removedCells;
}

// TODO
KrossWordCellList KrossWord::invalidateCellRegion(const Coord& coordTopLeft,
        const Coord& coordBottomRight, bool simulate)
{
    KrossWordCellList removedCells;
//   qDebug() << coordTopLeft << "to" << coordBottomRight;
    for (int x = coordTopLeft.first; x <= coordBottomRight.first; ++x) {
        for (int y = coordTopLeft.second; y <= coordBottomRight.second; ++y) {
            removedCells << invalidateCell(Coord(x, y), simulate);
        } // for y
    } // for x

    return removedCells;
}

void KrossWord::setEditable(bool editable)
{
    if (m_editable == editable) {
        return;
    }

    m_editable = editable;

    EmptyCellList cells = emptyCells();

    m_animator->beginEnqueueAnimations();

    foreach(EmptyCell * cell, cells) {
        cell->setFlag(QGraphicsItem::ItemIsFocusable, editable);
        cell->setFlag(QGraphicsItem::ItemIsSelectable, editable);

        if ((editable && isAnimationEnabled()) || (!editable && isAnimationEnabled())) {
            animator()->animate(editable ? Animator::AnimateFadeIn : Animator::AnimateFadeOut, cell);
        } else {
            cell->setOpacity(1);
        }
        cell->clearCache();
        cell->update();
    }

    ImageCellList imageCells = images();
    foreach(ImageCell * image, imageCells) {
        image->setFlag(QGraphicsItem::ItemIsFocusable, editable);
        image->setFlag(QGraphicsItem::ItemIsSelectable, editable);
        image->clearCache();
        image->update();
    }

    LetterCellList letterCells = letters();
    foreach(LetterCell * letter, letterCells) {
        letter->clearCache();
        letter->update();
    }

//   if ( m_titleItem ) {
//     if ( m_titleItem->titleItem() ) {
//       m_titleItem->titleItem()->setTextInteractionFlags(
//    editable ? Qt::TextEditable : Qt::NoTextInteraction );
//     }
//   }

    emit editModeChanged(editable);

    m_animator->endEnqueueAnimations();
}

void KrossWord::setInteractive(bool interactive)
{
    if (m_interactive == interactive)
        return;

    m_interactive = interactive;

    Qt::MouseButtons mouseButtons = interactive
                                    ? Qt::LeftButton | Qt::MidButton | Qt::RightButton
                                    : Qt::NoButton;

    setFlag(QGraphicsItem::ItemIsFocusable, interactive);
    setFlag(QGraphicsItem::ItemIsSelectable, interactive);
    setAcceptedMouseButtons(mouseButtons);

    LetterCellList letterCells = letters();
    foreach(LetterCell * letter, letterCells) {
        letter->setFlag(QGraphicsItem::ItemIsFocusable, interactive);
        letter->setFlag(QGraphicsItem::ItemIsSelectable, interactive);
        letter->setAcceptedMouseButtons(mouseButtons);
//  letter->update();
    }

    ClueCellList clueCells = clues();
    foreach(ClueCell * clue, clueCells) {
        clue->setFlag(QGraphicsItem::ItemIsFocusable, interactive);
        clue->setFlag(QGraphicsItem::ItemIsSelectable, interactive);
        clue->setAcceptedMouseButtons(mouseButtons);
//  clue->update();
    }

    if (!interactive)
        setHighlightedClue(NULL);
}

void KrossWord::setDrawForPrinting(bool drawForPrinting)
{
    m_drawForPrinting = drawForPrinting;

    /*
    EmptyCellList emptys = emptyCells();
    foreach(EmptyCell * emptyCell, emptys) {
        emptyCell->setFlag(QGraphicsItem::ItemHasNoContents, !drawForPrinting && !m_editable);
    }
    */
}

void KrossWord::clearCache()
{
    m_animator->beginEnqueueAnimations();

    KrossWordCellList cellList = cells();
    foreach(KrossWordCell * cell, cellList) {
        if (!cell) {
            qDebug() << "NULL cell";
            continue;
        }

        cell->clearCache();
        cell->update();
    }

    updateHeaderItem();

    m_animator->endEnqueueAnimations();
}

int KrossWord::maxAnswerLengthAt(const Coord& coord, Qt::Orientation orientation,
                                 ClueCell *excludedClue)
{
    if (!inside(coord) || !canTakeClueLetterCell(coord, orientation, excludedClue)) {
        return 0;
    }

    int length = 1;
    const Offset offset = orientation == Qt::Horizontal ? Offset(1, 0) : Offset(0, 1);
    Coord curCoord = coord + offset;
    while ((uint)curCoord.first < width() && (uint)curCoord.second < height()) {
        if (!canTakeClueLetterCell(curCoord, orientation, excludedClue)) {
            break;
        }
        ++length;

        curCoord += offset;
    }

    return length;
}

bool KrossWord::correctLettersAt(const Coord& coord, Qt::Orientation orientation,
                                 int letterCount, QString* correctLetters, ClueCell *excludeClue)
{
    Q_ASSERT(correctLetters);

    if (!inside(coord)) {
        return false;
    }

    correctLetters->clear();
    const Offset offset = orientation == Qt::Horizontal ? Offset(1, 0) : Offset(0, 1);
    Coord curCoord = coord;
    while ((uint)curCoord.first < width() && (uint)curCoord.second < height()
            && letterCount-- > 0) {
        KrossWordCell *cell = at(curCoord);
        if (cell->isLetterCell()) {
            LetterCell *letter = (LetterCell*)cell;
            // Check if this letter is only associated to the clue to be excluded (excludeClue)
            if (excludeClue && !letter->isCrossed()
                    && letter->hasClueInDirection(excludeClue->orientation())
                    && letter->clue(excludeClue->orientation()) == excludeClue)
                correctLetters->append(' ');
            else
                correctLetters->append(((LetterCell*)cell)->correctLetter());
        } else if (cell->isType(EmptyCellType))
            correctLetters->append(' ');
        else
            return false;

        curCoord += offset;
    }

    return true;
}

bool KrossWord::canTakeClueLetterCell(const Coord &coord, Qt::Orientation orientation,
                                      ClueCell *excludedClue)
{
    if (!inside(coord))
        return false;

    KrossWordCell *cell = at(coord);
    if (cell->isType(EmptyCellType) || cell == excludedClue)
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

void KrossWord::removeSynchronization(SyncMethods syncMethods,
                                      SyncCategories syncCategories)
{
    KrossWordCellList cellList = cells();
    foreach(KrossWordCell * cell, cellList)
    cell->removeSynchronization(syncMethods, syncCategories);
}

void KrossWord::focusCellChanged(KrossWordCell* /*currentCell*/)
{
//     emit currentCellChanged( currentCell );
}

KrossWordCellList KrossWord::moveCells(int dx, int dy, bool simulate)
{
    KrossWordCellList removedCells;

    // Delete cells that are moved outside the crossword grid.
    // Also removes clues of removed letter cells
    // and all cells of removed spanned cells.
    int maxX = width() - 1;
    int maxY = height() - 1;
    if (dx < 0) {
        removedCells << invalidateCellRegion(Coord(0, 0),
                                             Coord(-dx - 1, maxY), simulate);
    } else if (dx > 0) {
        removedCells << invalidateCellRegion(Coord(maxX - dx + 1, 0),
                                             Coord(maxX, maxY), simulate);
    }

    if (dy < 0) {
        removedCells << invalidateCellRegion(Coord(0, 0),
                                             Coord(maxX, -dy - 1), simulate);
    } else if (dy > 0) {
        removedCells << invalidateCellRegion(Coord(0, maxY - dy + 1),
                                             Coord(maxX, maxY), simulate);
    }

    if (simulate) {
        KrossWordCellList uniqueRemovedCells;
        for (KrossWordCellList::iterator it = removedCells.begin();
                it != removedCells.end(); ++it) {
            if (!uniqueRemovedCells.contains(*it))
                uniqueRemovedCells << *it;
        }
        return uniqueRemovedCells;
    }

    ClueCellList clueList = clues();
    QRect movingCellRect(dx < 0 ? -dx : 0, dy < 0 ? -dy : 0,
                         width() - abs(dx), height() - abs(dy));

    // Store old grid, create a new one
    // and put the cells into the new grid at the moved positions.
    KrosswordGrid *krossWordGrid = m_krossWordGrid;
    m_krossWordGrid = new KrosswordGrid(width(), height());

    Offset offset(dx, dy);
    for (uint y = movingCellRect.top(); y <= (uint)movingCellRect.bottom(); ++y) {
        for (uint x = movingCellRect.left(); x <= (uint)movingCellRect.right(); ++x) {
            Coord sourceCoord(x, y);
            Coord targetCoord = sourceCoord + offset;

            KrossWordCell *cell = krossWordGrid->at(sourceCoord);
            if (!cell)
                continue;

            SpannedCell *spannedCell = dynamic_cast<SpannedCell*>(cell);
            DoubleClueCell *doubleClueCell;
            if ((doubleClueCell = qgraphicsitem_cast<DoubleClueCell*>(cell))) {
                doubleClueCell->clue1()->setCoord(targetCoord, false);
                doubleClueCell->clue2()->setCoord(targetCoord, false);
            } else if ((spannedCell = dynamic_cast<SpannedCell*>(cell))) {
                QList< Coord > spannedSourceCoords = spannedCell->spannedCoords();
                spannedSourceCoords.removeOne(sourceCoord);
                foreach(const Coord & spannedSourceCoord, spannedSourceCoords) {
                    Coord spannedTargetCoord = spannedSourceCoord + offset;
                    (*krossWordGrid)[ spannedSourceCoord ] = NULL;
                    (*m_krossWordGrid)[ spannedTargetCoord ] = cell;
                }
            }

            (*m_krossWordGrid)[ targetCoord ] = cell;
            cell->setCoord(targetCoord, false);
            cell->setPositionFromCoordinates();
        }
    }
    // Delete old crossword grid
    delete krossWordGrid;

    // Move coordinates of hidden clue cells
    foreach(ClueCell * clue, clueList) {
        if (clue->answerOffset() == OnClueCell)
            clue->setCoord(clue->coord() + offset, false);
    }

    fillWithEmptyCells();

    KrossWordCellList uniqueRemovedCells;
    for (KrossWordCellList::iterator it = removedCells.begin();
            it != removedCells.end(); ++it) {
        if (!uniqueRemovedCells.contains(*it))
            uniqueRemovedCells << *it;
    }
    return uniqueRemovedCells;
}

KrossWordCellList KrossWord::resizeGrid(uint width, uint height, ResizeAnchor anchor, bool simulate)
{
    KrossWordCellList removedCells;
    uint prevWidth = this->width();
    uint prevHeight = this->height();

    if (prevWidth == width && prevHeight == height)
        return removedCells;
//   if ( simulate )
//     qDebug() << "SIMULATION";
//   qDebug() << "Resizing from" << QString("%1x%2").arg(prevWidth).arg(prevHeight)
//     << "to" << QString("%1x%2").arg(width).arg(height);

    QRect sourceRect;
    Offset offset;
    int dw = width - prevWidth, dh = height - prevHeight;
    switch (anchor) {
    case AnchorTopLeft:
        offset = Offset(0, 0);
        break;
    case AnchorTop:
        offset = Offset(dw / 2, 0);
        break;
    case AnchorTopRight:
        offset = Offset(dw, 0);
        break;
    case AnchorLeft:
        offset = Offset(0, dh / 2);
        break;
    case AnchorCenter:
        offset = Offset(dw / 2, dh / 2);
        break;
    case AnchorRight:
        offset = Offset(dw, dh / 2);
        break;
    case AnchorBottomLeft:
        offset = Offset(0, dh);
        break;
    case AnchorBottom:
        offset = Offset(dw / 2, dh);
        break;
    case AnchorBottomRight:
        offset = Offset(dw, dh);
        break;
    }
//   qDebug() << "Moving the crossword content according to anchor" << negOffset << anchor;
//   qDebug() << this;

    // Move coords of hidden clue cells
    KrosswordGrid *krossWordGrid = new KrosswordGrid(width, height);
    foreach(ClueCell * clue, clues()) {
        if (clue->answerOffset() != OnClueCell)
            continue;

        Coord sourceCoord = clue->coord();
        Coord targetCoord = sourceCoord + offset;

        if (krossWordGrid->inside(targetCoord)) {
            if (simulate)
                continue;

//       qDebug() << "inside" << sourceCoord << "=>" << targetCoord;
            clue->setCoord(targetCoord, false);
        }
    }

    // Move all cells in the crossword grid
    for (uint y = 0; y < prevHeight; ++y) {
        for (uint x = 0; x < prevWidth; ++x) {
            Coord sourceCoord(x, y);
            Coord targetCoord = sourceCoord + offset;

            if (krossWordGrid->inside(targetCoord)) {
                if (simulate)
                    continue;

                KrossWordCell *cell = m_krossWordGrid->at(sourceCoord);
                if (!cell)
                    continue;

                SpannedCell *spannedCell = dynamic_cast<SpannedCell*>(cell);
                DoubleClueCell *doubleClueCell;
                if ((doubleClueCell = qgraphicsitem_cast<DoubleClueCell*>(cell))) {
                    doubleClueCell->clue1()->setCoord(targetCoord, false);
                    doubleClueCell->clue2()->setCoord(targetCoord, false);
                } else if ((spannedCell = dynamic_cast<SpannedCell*>(cell))) {
                    QList< Coord > spannedSourceCoords = spannedCell->spannedCoords();
                    spannedSourceCoords.removeOne(sourceCoord);
                    foreach(const Coord & spannedSourceCoord, spannedSourceCoords) {
                        Coord spannedTargetCoord = spannedSourceCoord + offset;
                        (*m_krossWordGrid)[ spannedSourceCoord ] = NULL;
                        (*krossWordGrid)[ spannedTargetCoord ] = cell;
                    }
                }

                (*krossWordGrid)[ targetCoord ] = cell;
                cell->setCoord(targetCoord, false);
                cell->setPositionFromCoordinates();
            } else {
                removedCells << invalidateCell(sourceCoord, simulate);
            }
        } // for x
    } // for y

    if (!simulate) {
        delete m_krossWordGrid;
        m_krossWordGrid = krossWordGrid;

        fillWithEmptyCells();
    } else
        delete krossWordGrid;

    KrossWordCellList uniqueRemovedCells;
    for (KrossWordCellList::iterator it = removedCells.begin(); it != removedCells.end(); ++it) {
        if (!uniqueRemovedCells.contains(*it)) {
            uniqueRemovedCells << *it;
        }
    }

    emit gridResized(this, width, height);
    return uniqueRemovedCells;
}

QString KrossWord::contentString() const
{
    QString s = QString("Crossword size is %1x%2").arg(width()).arg(height());

    QStringList lines;
    lines << QString();

    for (int y = 0; (uint)y < height(); ++y) {
        QString line = QString("%1:\t").arg(y);
        for (int x = 0; (uint)x < width(); ++x) {
            KrossWordCell *cell = at(Coord(x, y));
            if (cell) {
                if (cell->isLetterCell()) {
                    QChar corrChar = ((LetterCell*)cell)->correctLetter();
                    line += QString("| %1").arg(corrChar.isSpace() ? '-' : corrChar);

                    LetterCell *letter = (LetterCell*)cell;
                    if (letter->isCrossed())
                        line += '+';

                    int firstLetterOfHiddenClueCount = 0;
                    foreach(ClueCell * letterClue, letter->clues()) {
                        if (letterClue->answerOffset() == OnClueCell
                                && letterClue->firstLetter() == letter)
                            ++firstLetterOfHiddenClueCount;
                    }
                    if (firstLetterOfHiddenClueCount > 0)
                        line += '(' + QString::number(firstLetterOfHiddenClueCount) + ')';

                    line += '\t';
                } else
                    line += "| " + stringFromCellType(cell->getCellType()).left(7) + '\t';
            } else
                line += "| NULL\t";
        }
        lines << line;
//     qDebug() << "        " << line;
    }

    QString firstLine('\t');
    for (int x = 0; (uint)x < width(); ++x)
        firstLine += QString("| %1:\t").arg(x);
    lines[ 0 ] = firstLine;
    s += '\n' + lines.join("\n");

    return s;
}

bool KrossWord::isEmpty() const
{
    return clues().isEmpty();
}

void KrossWord::removeAllCells()
{
    emit cluesAboutToBeRemoved(clues());

    QHash< ClueCell*, ClueExpanderItem* >::iterator it;
    for (it = m_clueExpanderItems.begin(); it != m_clueExpanderItems.end(); ++it) {
        scene()->removeItem(it.value());
        it.value()->deleteLater();
    }
    m_clueExpanderItems.clear();
    m_clues.clear();

    setHighlightedClue(NULL);
    KrossWordCellList cellList = cells();
    foreach(KrossWordCell * cell, cellList)
    removeCell(cell);
}

void KrossWord::deleteAllCells()
{
    /**** THIS IS CURRENTLY ONLY CALLED BY THE DESTRUCTOR ****/
//   emit cluesAboutToBeRemoved( clues() );

    m_clues.clear();
    setHighlightedClue(NULL);
    removeSynchronization(); // Prevents crash when calling qDeleteAll

//   foreach ( ClueCell *clue, m_clueExpanderItems.keys() ) {
//     ClueExpanderItem *expanderItem = m_clueExpanderItems[ clue ];
//     scene()->removeItem( expanderItem );
//     expanderItem->deleteLater();
//   }
    m_clueExpanderItems.clear();

    qDeleteAll(cells());

//     foreach ( KrossWordCell *cell, cellList ) {
//  replaceCell( cell, NULL );
//     }

    m_krossWordGrid->resize(0, 0);
    m_solutionLetters.clear();
    foreach(SolutionLetterCell * solutionLetter, m_solutionLetters)
    emit solutionWordLetterRemoved(solutionLetter);
    m_highlightedClue = NULL;

//   if ( m_focusItem ) {
//     scene()->removeItem( m_focusItem );
//     delete m_focusItem;
//     m_focusItem = NULL;
//   }
}

void KrossWord::fillWithEmptyCells()
{
    fillWithEmptyCells(Coord(0, 0),
                       Coord(width() - 1, height() - 1));
}

void KrossWord::fillWithEmptyCells(const Coord& coordTopLeft,
                                   const Coord& coordBottomRight)
{
    Coord coord;
    for (coord.first = coordTopLeft.first;
            coord.first <= coordBottomRight.first; ++coord.first) {
        for (coord.second = coordTopLeft.second;
                coord.second <= coordBottomRight.second; ++coord.second) {
            if (!(*m_krossWordGrid)[coord]) {
                EmptyCell *newEmptyCell = new EmptyCell(this, coord);
                (*m_krossWordGrid)[ coord ] = newEmptyCell;
                if (isAnimationEnabled()) {
                    animator()->animate(Animator::AnimateFadeIn, newEmptyCell);
                } else {
                    newEmptyCell->show();
                    newEmptyCell->setOpacity(1);
                }
            }
        }
    }
}

QString KrossWord::currentSolutionWord() const
{
    QString solutionWord;
    SolutionLetterCellList letters = solutionWordLetters();
    foreach(SolutionLetterCell * letter, letters)
    solutionWord += letter->currentLetter();

    return solutionWord;
}

void KrossWord::setHighlightedClue(ClueCell* clue)
{
    if (clue && !clue->flags().testFlag(ItemIsFocusable)) {
        qDebug() << "Clue isn't focusable";
        return;
    }

//     if ( m_highlightedClue )
//  qDebug() << "old highlight for" << m_highlightedClue->clue()
//      << m_highlightedClue->orientation();
//     if ( clue )
//  qDebug() << "new highlight for" << clue->clue() << clue->orientation();
//     else
//  qDebug() << "no new highlighted clue";
//     qDebug() << "";

    if (m_highlightedClue == clue)
        return;

    m_previousHighlightedClue = m_highlightedClue;
    if (m_previousHighlightedClue)
        m_previousHighlightedClue->setHighlight(false);

    m_highlightedClue = clue;
    if (m_highlightedClue)
        m_highlightedClue->setHighlight(true);

    // Give some time to animations TODO: fix crash here
//     QApplication::processEvents( QEventLoop::ExcludeUserInputEvents, 35 );

    emit currentClueChanged(clue);
}

void KrossWord::answerChangedSlot(ClueCell* clue, const QString& currentAnswer)
{
//     if ( m_signalAnswerChanged )
    emit answerChanged(clue, currentAnswer);
}

void KrossWord::solutionLetterChanged(LetterCell* letter, const QChar& newLetter)
{
    Q_UNUSED(newLetter);

    SolutionLetterCell *solutionLetter = qgraphicsitem_cast<SolutionLetterCell*>(letter);
    emit solutionLetterChanged(solutionLetter, currentSolutionWord(),
                               solutionLetter->solutionWordIndex());
}

QString KrossWord::solutionWord(const QChar &charNonLinkedLetters) const
{
    QString solutionWord;
    foreach(SolutionLetterCell * solutionLetter, m_solutionLetters) {
        while (solutionLetter->solutionWordIndex() > solutionWord.length())
            solutionWord += charNonLinkedLetters;
        solutionWord += solutionLetter->correctLetter();
    }

    return solutionWord;
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

QStandardItemModel* KrossWord::createCrosswordTypeModel(
    const QList<CrosswordTypeInfo> &additionalTypes)
{
    QStandardItemModel *model = new QStandardItemModel(0, 2);

    model->setHeaderData(0, Qt::Horizontal, i18nc("Name", "Used as header label "
                         "for the column containing the names in the crossword type model."),
                         Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, i18nc("Description", "Used as header "
                         "label for the column containing the descriptions in the crossword "
                         "type model."),
                         Qt::DisplayRole);

    QList<CrosswordTypeInfo> typeInfoList;
    QList< CrosswordType > typeList = CrosswordTypeInfo::typeList();
    foreach(CrosswordType crosswordType, typeList)
    typeInfoList << CrosswordTypeInfo::infoFromType(crosswordType);
    typeInfoList << additionalTypes;

    // Add a row for each crossword type
    int i = 0;
    foreach(CrosswordTypeInfo info, typeInfoList) {
        QStandardItem *itemTitle = new QStandardItem(QIcon::fromTheme(info.iconName), info.name);
        QStandardItem *itemDescription = new QStandardItem(info.description);
        itemTitle->setData(static_cast<int>(info.crosswordType), Qt::UserRole + 1);

        if (i >=  typeList.count()) {
            // info is an additional type
            itemTitle->setData(qVariantFromValue<CrosswordTypeInfo>(info),
                               Qt::UserRole + 2);
        }

        QString whatsThis = info.longDescription.isEmpty() ? info.description : info.longDescription;
        itemTitle->setWhatsThis(whatsThis);
        itemDescription->setWhatsThis(whatsThis);
        model->appendRow(QList<QStandardItem*>() << itemTitle << itemDescription);

        ++i;
    }

    return model;
}

KrossWord::Statistics KrossWord::statistics()
{
    Statistics stats;
    stats.cellCount = width() * height();

    LetterCellList letterList = stats.letters = letters();
    stats.letterCellCount = letterList.count();
    stats.crossedLetterCells = stats.uncrossedLetterCells = 0;
    foreach(LetterCell * letter, letterList) {
        if (letter->isCrossed())
            ++stats.crossedLetterCells;
        else
            ++stats.uncrossedLetterCells;

        ++stats.letterCellCountByChar[ letter->correctLetter()];
    }

    ClueCellList clueList = stats.clues = clues();
    stats.clueCount = clueList.count();
    stats.horizontalClues = stats.verticalClues = 0;
    stats.minAnswerLength = 999999999;
    stats.maxAnswerLength = 0;
    int answerLengthSum = 0;
    foreach(ClueCell * clue, clueList) {
        if (clue->isHorizontal())
            ++stats.horizontalClues;
        else
            ++stats.verticalClues;

        int answerLength = clue->correctAnswer().length();
        answerLengthSum += answerLength;
        if (answerLength < stats.minAnswerLength)
            stats.minAnswerLength = answerLength;
        if (answerLength > stats.maxAnswerLength)
            stats.maxAnswerLength = answerLength;
    }
    stats.avgAnswerLength = (float)answerLengthSum / (float)stats.clueCount;

    EmptyCellList emptyList = stats.emptyCells = emptyCells();
    stats.emptyCellCount = emptyList.count();

    return stats;
}

QString KrossWord::errorMessageFromErrorType(ErrorType errorType)
{
    switch (errorType) {
    case ErrorNone:
        return i18n("No error.");

    case ErrorClueDoesntFit:
        return i18n("The clue doesn't fit in the grid with the given settings.");
    case ErrorClueCellIsntEmpty:
        return i18n("The cell for the new clue isn't empty.");
    case ErrorAnswerContainsIllegalCharacters:
        return i18n("The answer contains illegal characters. Allowed are A-Z and/or 0-9.");
    case ErrorAnswerIsIllegal:
        return i18n("The answer produces mismatching letter cells at the same positions.");
    case ErrorAnswerOverwritesClueInSameOrientation:
        return i18n("The answer overwrites the answer to another clue with the same orientation.");
    case ErrorAnswerCrossesClueCell:
        return i18n("The answer's letter cells cross a clue cell.");
    case ErrorClueCellsDisallowed:
        return i18n("The current crossword type doesn't allow clue cells.");
    case ErrorClueCellsRequired:
        return i18n("The current crossword type requires a clue cell for each clue.");
    case ErrorAnswerIsTooShort:
        return i18n("The answer is too short for the current crossword type.");
    case ErrorAnswerOverwritesClueCell:
        return i18n("The answer overwrites the clue cell.");

    case ErrorImageDoesntFit:
        return i18n("The image doesn't fit in the grid with the given settings.");
    case ErrorImageCellsArentEmpty:
        return i18n("The cells for the new image aren't empty.");
    case ErrorImageCellsDisallowed:
        return i18n("The current crossword type doesn't allow image cells.");
    }

    return "Unknown error.";
}

AnswerOffset KrossWord::answerOffsetFromString(const QString &s)
{
    QString sl = s.toLower();
    if (sl == "cluehidden")
        return OnClueCell;
    else if (sl == "right")
        return OffsetRight;
    else if (sl == "bottom")
        return OffsetBottom;
    else if (sl == "left")
        return OffsetLeft;
    else if (sl == "top")
        return OffsetTop;
    else if (sl == "topleft")
        return OffsetTopLeft;
    else if (sl == "topright")
        return OffsetTopRight;
    else if (sl == "bottomleft")
        return OffsetBottomLeft;
    else if (sl == "bottomright")
        return OffsetBottomRight;
    else {
        qDebug() << "Couldn't get enumerable for" << s;
        return OffsetInvalid;
    }
}

QString KrossWord::answerOffsetToString(AnswerOffset answerOffset)
{
    switch (answerOffset) {
    case OffsetTop:
        return "Top";
    case OffsetRight:
        return "Right";
    case OffsetLeft:
        return "Left";
    case OffsetBottom:
        return "Bottom";
    case OffsetTopLeft:
        return "TopLeft";
    case OffsetTopRight:
        return "TopRight";
    case OffsetBottomLeft:
        return "BottomLeft";
    case OffsetBottomRight:
        return "BottomRight";
    case OffsetInvalid: // Shouldn't appear here..
        qDebug() << "Got an invalid answerOffset";
    case OnClueCell:
    default:
        return "ClueHidden";
    }
}

} // namespace Crossword

//#include "krossword.moc"
