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

#include "global.h"
#include <QDebug>
#include <KLocalizedString>

namespace Crossword
{
QList< CellType > allCellTypes()
{
    return QList< CellType >() << EmptyCellType << LetterCellType << ClueCellType
           << DoubleClueCellType << SolutionLetterCellType << ImageCellType;
}

QString displayStringFromCellType(CellType cellType)
{
    switch (cellType) {
    case EmptyCellType:
        return i18n("Empty cell");
    case ClueCellType:
        return i18n("Clue cell");
    case DoubleClueCellType:
        return i18n("Double clue cell");
    case LetterCellType:
        return i18n("Letter cell");
    case SolutionLetterCellType:
        return i18n("Solution letter cell");
    case ImageCellType:
        return i18n("Image cell");
    case AllCellTypes:
        return i18n("All cells");
    case InteractiveCellTypes:
        return i18n("Interactive cells");
    case UserType:
        return i18n("User cell type");

    default:
        return QString("Unknown cell type (%1)").arg(static_cast<int>(cellType));
    }
}

QString stringFromCellType(CellType cellType)
{
    // No "," allowed
    switch (cellType) {
    case EmptyCellType:
        return "empty";
    case ClueCellType:
        return "clue";
    case DoubleClueCellType:
        return "doubleClue";
    case LetterCellType:
        return "letter";
    case SolutionLetterCellType:
        return "solutionLetter";
    case ImageCellType:
        return "image";
    case AllCellTypes:
        return "all";
    case InteractiveCellTypes:
        return "interactive";
    case UserType:
        return "user";

    default:
        return QString("unknown(%1)").arg(static_cast<int>(cellType));
    }
}

CellType cellTypeFromString(const QString& sCellType)
{
    QString sl = sCellType.toLower();
    if (sl == "empty")
        return EmptyCellType;
    else if (sl == "clue")
        return ClueCellType;
    else if (sl == "doubleclue")
        return DoubleClueCellType;
    else if (sl == "letter")
        return LetterCellType;
    else if (sl == "solutionletter")
        return SolutionLetterCellType;
    else if (sl == "image")
        return ImageCellType;
    else if (sl == "all")
        return AllCellTypes;
    else if (sl == "interactive")
        return InteractiveCellTypes;
    else if (sl == "user")
        return UserType;
    else {
        qDebug() << "Couldn't get enumerable for" << sl;
        Q_ASSERT(false);
        return UserType;
    }
}

ClueCellHandling CrosswordTypeInfo::clueCellHandlingFromString(
    const QString& sClueCellHandling)
{
    QString sl = sClueCellHandling.toLower();
    if (sl == "cluecellsallowed")
        return ClueCellsAllowed;
    else if (sl == "cluecellsdisallowed")
        return ClueCellsDisallowed;
    else if (sl == "cluecellsrequired")
        return ClueCellsRequired;
    else {
        qDebug() << "Couldn't get enumerable for" << sl;
        Q_ASSERT(false);
        return ClueCellsAllowed;
    }
}

ClueMapping CrosswordTypeInfo::clueMappingFromString(
    const QString& sClueMapping)
{
    QString sl = sClueMapping.toLower();
    if (sl == "cluesrefertosetsofcells")
        return CluesReferToSetsOfCells;
    else if (sl == "cluesrefertocells")
        return CluesReferToCells;
    else {
        qDebug() << "Couldn't get enumerable for" << sl;
        Q_ASSERT(false);
        return CluesReferToSetsOfCells;
    }
}

ClueType CrosswordTypeInfo::clueTypeFromString(
    const QString& sClueType)
{
    QString sl = sClueType.toLower();
    if (sl == "stringclues")
        return StringClues;
    else if (sl == "numberclues1to26")
        return NumberClues1To26;
    else {
        qDebug() << "Couldn't get enumerable for" << sl;
        Q_ASSERT(false);
        return StringClues;
    }
}

LetterCellContent CrosswordTypeInfo::letterCellContentFromString(
    const QString& sLetterCellContent)
{
    QString sl = sLetterCellContent.toLower();
    if (sl == "characters")
        return Characters;
    else if (sl == "digits")
        return Digits;
    else if (sl == "charactersordigits")
        return CharactersOrDigits;
    else {
        qDebug() << "Couldn't get enumerable for" << sl;
        Q_ASSERT(false);
        return CharactersOrDigits;
    }
}

CellTypes CrosswordTypeInfo::cellTypesFromStringList(
    const QStringList& sCellTypes)
{
    CellTypes cellTypes;

    foreach(const QString & s, sCellTypes)
    cellTypes |= cellTypeFromString(s);

    return cellTypes;
}

QString CrosswordTypeInfo::stringFromClueCellHandling(
    ClueCellHandling clueCellHandling)
{
    switch (clueCellHandling) {
    case ClueCellsAllowed:
        return "clueCellsAllowed";
    case ClueCellsDisallowed:
        return "clueCellsDisallowed";
    case ClueCellsRequired:
        return "clueCellsRequired";
    }
    Q_ASSERT(false);
    return "";
}

QString CrosswordTypeInfo::stringFromClueMapping(ClueMapping clueMapping)
{
    switch (clueMapping) {
    case CluesReferToSetsOfCells:
        return "cluesReferToSetsOfCells";
    case CluesReferToCells:
        return "cluesReferToCells";
    }
    Q_ASSERT(false);
    return "";
}

QString CrosswordTypeInfo::stringFromClueType(ClueType clueType)
{
    switch (clueType) {
    case StringClues:
        return "stringClues";
    case NumberClues1To26:
        return "numberClues1To26";
    }
    Q_ASSERT(false);
    return "";
}

QString CrosswordTypeInfo::stringFromLetterCellContent(
    LetterCellContent letterCellContent)
{
    switch (letterCellContent) {
    case Characters:
        return "characters";
    case Digits:
        return "digits";
    case CharactersOrDigits:
        return "charactersOrDigits";
    }
    Q_ASSERT(false);
    return "";
}

QStringList CrosswordTypeInfo::stringListFromCellTypes(
    CellTypes cellTypes)
{
    QStringList strings;
    foreach(CellType cellType, allCellTypes()) {
        if (cellTypes.testFlag(cellType))
            strings << stringFromCellType(cellType);
    }
    return strings;
}

QString CrosswordTypeInfo::displayStringFromClueCellHandling(
    ClueCellHandling clueCellHandling)
{
    switch (clueCellHandling) {
    case ClueCellsAllowed:
        return i18n("Clue cells are allowed");
    case ClueCellsDisallowed:
        return i18n("Clue cells are disallowed");
    case ClueCellsRequired:
        return i18n("Clue cells are required");
    }
    Q_ASSERT(false);
    return "";
}

QString CrosswordTypeInfo::displayStringFromClueMapping(
    ClueMapping clueMapping)
{
    switch (clueMapping) {
    case CluesReferToSetsOfCells:
        return i18n("Clues refer to sets of cells");
    case CluesReferToCells:
        return i18n("Clues refer to cells");
    }
    Q_ASSERT(false);
    return "";
}

QString CrosswordTypeInfo::displayStringFromClueType(ClueType clueType)
{
    switch (clueType) {
    case StringClues:
        return i18n("String clues");
    case NumberClues1To26:
        return i18n("Number clues (1 to 26)");
    }
    Q_ASSERT(false);
    return "";
}

QString CrosswordTypeInfo::displayStringFromLetterCellContent(
    LetterCellContent letterCellContent)
{
    switch (letterCellContent) {
    case Characters:
        return i18n("Characters");
    case Digits:
        return i18n("Digits");
    case CharactersOrDigits:
        return i18n("Characters or digits");
    }
    Q_ASSERT(false);
    return "";
}

QList< ClueCellHandling > CrosswordTypeInfo::allClueCellHandlingValues()
{
    return QList< ClueCellHandling >() << ClueCellsAllowed
           << ClueCellsDisallowed << ClueCellsRequired;
}

QList< ClueMapping > CrosswordTypeInfo::allClueMappingValues()
{
    return QList< ClueMapping >() << CluesReferToSetsOfCells
           << CluesReferToCells;
}

QList< ClueType > CrosswordTypeInfo::allClueTypeValues()
{
    return QList< ClueType >() << StringClues << NumberClues1To26;
}

QList< LetterCellContent > CrosswordTypeInfo::allLetterCellContentValues()
{
    return QList< LetterCellContent >() << Characters << Digits
           << CharactersOrDigits;
}

CrosswordType CrosswordTypeInfo::typeFromString(const QString& crosswordType)
{
    QString type = crosswordType.toLower();
    if (type == "user")
        return UserDefinedCrossword;
    else if (type == "american")
        return American;
    else if (type == "swedish")
        return Swedish;
    else if (type == "crossnumber")
        return CrossNumber;
    else if (type == "codedpuzzle")
        return CodedPuzzle;
    else if (type == "free")
        return FreeCrossword;
    else if (type == "unknown")
        return UnknownCrosswordType;
    else {
        qDebug() << "Unknown crossword type:" << crosswordType
                 << "Using 'Unknown' as crossword type (same as 'Free')";
        return UnknownCrosswordType;
    }
}

const QString CrosswordTypeInfo::stringFromType(CrosswordType crosswordType)
{
    switch (crosswordType) {
    case UserDefinedCrossword:
        return "user";
    case American:
        return "american";
    case Swedish:
        return "swedish";
    case CrossNumber:
        return "crossNumber";
    case CodedPuzzle:
        return "codedPuzzle";
    case FreeCrossword:
        return "free";
    case UnknownCrosswordType:
    default:
        return "unknown";
    };
}

const CrosswordTypeInfo CrosswordTypeInfo::infoFromType(
    CrosswordType crosswordType)
{
    if (crosswordType == UnknownCrosswordType) {
        // Unknown is the type for crosswords from files without information about the type
        CrosswordTypeInfo info = free();
        info.crosswordType = UnknownCrosswordType;
        return info;
    }

    switch (crosswordType) {
    case UserDefinedCrossword:
        return defaultUserDefined();
    case American:
        return american();
    case Swedish:
        return swedish();
    case CrossNumber:
        return crossNumber();
    case CodedPuzzle:
        return codedPuzzle();
    case FreeCrossword:
        return free();
    default:
        qDebug() << "Crossword type unknown, using 'Free' as type" << crosswordType;
        return free();
    };
}

const CrosswordTypeInfo CrosswordTypeInfo::free()
{
    return CrosswordTypeInfo(FreeCrossword, i18n("Free Crossword"),
                             i18nc("Short description of the crossword type 'Free'",
                                   "This is a special type that tries to be least restrictive."),
                             i18nc("Long description of the crossword type 'Free'",
                                   "This is a special type that tries to be least restrictive. You can "
                                   "use all available cell types (eg. clue cells, double clue cells, image "
                                   "cells, solution letter cells). You can mix answers that have a clue "
                                   "cell with answers that don't. Answers may contain characters as well "
                                   "as numbers."),
                             "crossword-free",
                             ClueCellsAllowed, false, 1, CharactersOrDigits, CluesReferToSetsOfCells);
}

const CrosswordTypeInfo CrosswordTypeInfo::defaultUserDefined()
{
    return CrosswordTypeInfo(UserDefinedCrossword, i18n("User Defined Crossword"),
                             i18nc("Short description of the crossword type 'User Defined'",
                                   "Lets you specify your own rules."),
                             QString(),
                             "crossword-user-defined",
                             ClueCellsAllowed, false, 1, CharactersOrDigits, CluesReferToSetsOfCells);
}

const CrosswordTypeInfo CrosswordTypeInfo::swedish()
{
    return CrosswordTypeInfo(Swedish, i18n("Swedish Crossword"),
                             i18nc("Short description of the crossword type 'Swedish'",
                                   "Swedish crosswords contain clue cells which contain the clue text in it "
                                   "and an arrow to indicate in which direction the clues have to be answered."),
                             i18nc("Long description of the crossword type 'Swedish'",
                                   "The \"Swedish-Style\" grid uses no clue numbers - the clues are contained "
                                   "in the cells which would normally be black in other countries. Arrows indicate "
                                   "in which direction the clues have to be answered, vertical or horizontal. This "
                                   "style of grid is used in several countries other than Sweden, usually in "
                                   "magazines with pages of A4 or similar size. The grid often has a photo of a "
                                   "pop or movie star replacing a block of squares, as a clue to one answer. These "
                                   "puzzles usually have no symmetry in the grid.\n"
                                   "Crosswords with clue cells inside the crossword grid are also called "
                                   "\"Arrowwords\", \"Pointers\" or \"Tipwords\" in English, "
                                   "\"Autodefinidos\" in Spanish, \"Mots Fléchés\" in French, etc.\n"
                                   "(text taken from wikipedia)"),
                             "crossword-swedish",
                             ClueCellsRequired, false, 1, Characters,
                             CluesReferToSetsOfCells, StringClues, AllCellTypes);
}

const CrosswordTypeInfo CrosswordTypeInfo::american()
{
    return CrosswordTypeInfo(American, i18n("American Crossword"),
                             i18nc("Short description of the crossword type 'American'",
                                   "American crosswords have numbered clues and a list of clues on the "
                                   "side of the crossword. They have usually 180 degree rotational symmetry."),
                             i18nc("Long description of the crossword type 'American'",
                                   "Crossword grids such as those appearing in most North American "
                                   "newspapers and magazines feature solid areas of white squares. Every "
                                   "letter is checked, and usually each answer is required to contain at "
                                   "least three letters. In such puzzles shaded squares are traditionally "
                                   "limited to about one-sixth of the design. Crossword grids elsewhere, "
                                   "such as in Britain and Australia, have a lattice-like structure, with "
                                   "a higher percentage of shaded squares, leaving up to half the letters "
                                   "in an answer unchecked. For example, if the top row has an answer "
                                   "running all the way across, there will be no across answers in the "
                                   "second row.\n"
                                   "Another tradition in puzzle design (in North America and Britain "
                                   "particularly) is that the grid should have 180-degree rotational "
                                   "symmetry, so that its pattern appears the same if the paper is turned "
                                   "upside down. Most puzzle designs also require that all white cells be "
                                   "orthogonally contiguous (that is, connected in one mass through "
                                   "shared sides, to form a single polyomino).\n"
                                   "(text taken from wikipedia)"),
                             "crossword-american",
                             ClueCellsDisallowed, true, 3, Characters,
                             CluesReferToSetsOfCells, StringClues,
                             EmptyCellType | LetterCellType,
                             QList<QSize>() << QSize(15, 15) << QSize(21, 21) << QSize(23, 23) << QSize(25, 25));
}

const CrosswordTypeInfo CrosswordTypeInfo::codedPuzzle()
{
    return CrosswordTypeInfo(CodedPuzzle, i18n("Coded Puzzle"),
                             i18nc("Short description of the crossword type 'Coded Puzzle'",
                                   "Coded puzzles are a variant of crosswords in which each cell has a "
                                   "number between 1 and 26. The solver has to find out for which letter of "
                                   "the alphabet a number stands."),
                             QString(),
                             "crossword-codedpuzzle",
                             ClueCellsDisallowed, false, 1, Characters,
                             CluesReferToCells, NumberClues1To26,
                             EmptyCellType
                             | LetterCellType | ImageCellType);
}

const CrosswordTypeInfo CrosswordTypeInfo::crossNumber()
{
    return CrosswordTypeInfo(CrossNumber, i18n("Crossnumber"),
                             i18nc("Short description of the crossword type 'Crossnumber'",
                                   "Crossnumbers are the numerical analogy of a crossword, in which "
                                   "the solutions to the clues are numbers instead of words."),
                             i18nc("Long description of the crossword type 'Crossnumber'",
                                   "A crossnumber (also known as a cross-figure) is the numerical "
                                   "analogy of a crossword, in which the solutions to the clues are "
                                   "numbers instead of words. Clues are usually arithmetical "
                                   "expressions, but can also be general knowledge clues to which the "
                                   "answer is a number or year. There are also numerical fill-in "
                                   "crosswords.\n"
                                   "The Daily Mail Weekend magazine used to feature crossnumbers under "
                                   "the misnomer Number Word. This kind of puzzle should not be confused "
                                   "with a different puzzle that the Daily Mail refers to as Cross Number.\n"
                                   "(text taken from wikipedia)"),
                             "crossword-crossnumber",
                             ClueCellsAllowed, false, 1, Digits, CluesReferToSetsOfCells,
                             StringClues,
                             EmptyCellType | ClueCellType
                             | DoubleClueCellType | LetterCellType
                             | ImageCellType);
}

QString CrosswordTypeInfo::allowedChars() const
{
    switch (letterCellContent) {
    case Characters:
        return "A-ZÅÆŒØ";
    case Digits:
        return "0-9";
    case CharactersOrDigits:
        return "A-Z0-9";
    }

    return QString();
}

bool CrosswordTypeInfo::isCharacterLegal(const QChar& testCharacter) const
{
    qDebug() << testCharacter;
    switch (letterCellContent) {
    case Characters:
        return QString::fromUtf8("ABCDEFGHIJKLMNOPQRSTUVWXYZÅÆŒØ")
               .contains(testCharacter, Qt::CaseInsensitive);
    case Digits:
        return testCharacter.isDigit();
    case CharactersOrDigits:
        return testCharacter.isDigit() || QString::fromUtf8("ABCDEFGHIJKLMNOPQRSTUVWXYZÅÆŒØ")
               .contains(testCharacter, Qt::CaseInsensitive);
    }

    return false;
}

QDataStream &operator <<(QDataStream &stream, CrosswordTypeInfo typeInfo)
{
    stream << typeInfo.name;
    stream << typeInfo.description;
    stream << typeInfo.longDescription;
    stream << typeInfo.iconName;

    stream << static_cast< qint8 >(typeInfo.crosswordType);
    stream << static_cast< qint8 >(typeInfo.cellTypes);
    stream << static_cast< qint8 >(typeInfo.clueCellHandling);
    stream << static_cast< qint8 >(typeInfo.clueMapping);
    stream << static_cast< qint8 >(typeInfo.clueType);
    stream << static_cast< qint8 >(typeInfo.letterCellContent);

    stream << (qint8)typeInfo.defaultSizes.count();
    foreach(const QSize & size, typeInfo.defaultSizes) {
        stream << (qint16)size.width() << (qint16)size.height();
    }

    stream << (qint8)typeInfo.minAnswerLength;
    return stream << typeInfo.rotationSymmetryRequired;
}

QDataStream &operator >>(QDataStream &stream, CrosswordTypeInfo &typeInfo)
{
    stream >> typeInfo.name;
    stream >> typeInfo.description;
    stream >> typeInfo.longDescription;
    stream >> typeInfo.iconName;

    qint8 iCrosswordType, iCellTypes, iClueCellHandling, iClueMapping,
          iClueType, iLetterCellContent;
    stream >> iCrosswordType;
    stream >> iCellTypes;
    stream >> iClueCellHandling;
    stream >> iClueMapping;
    stream >> iClueType;
    stream >> iLetterCellContent;
    typeInfo.crosswordType = static_cast< CrosswordType >(iCrosswordType);
    typeInfo.cellTypes = static_cast< CellTypes >(iCellTypes);
    typeInfo.clueCellHandling = static_cast< ClueCellHandling >(iClueCellHandling);
    typeInfo.clueMapping = static_cast< ClueMapping >(iClueMapping);
    typeInfo.clueType = static_cast< ClueType >(iClueType);
    typeInfo.letterCellContent = static_cast< LetterCellContent >(iLetterCellContent);

    typeInfo.defaultSizes.clear();
    qint8 defaultSizesCount;
    stream >> defaultSizesCount;
    for (int i = 0; i < defaultSizesCount; ++i) {
        qint16 w, h;
        stream >> w >> h;
        typeInfo.defaultSizes << QSize(w, h);
    }

    qint8 minAnswerLength;
    stream >> minAnswerLength;
    typeInfo.minAnswerLength = minAnswerLength;

    return stream >> typeInfo.rotationSymmetryRequired;
}

} // namespace Crossword

Crossword::Offset operator *(const Crossword::Offset &offset, int factor)
{
    return Crossword::Offset(offset.first * factor, offset.second * factor);
}

Coord operator+=(Coord & coord, const Crossword::Offset & offset)
{
    return coord = coord + offset;
}
