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

#ifndef GLOBAL_HEADER
#define GLOBAL_HEADER

#include <qnamespace.h>
#include <QList>
#include <QSize>
#include <QRegExp>
#include <QDebug>

#include "kgrid2d.h"

using KGrid2D::Coord;

namespace Crossword
{
class ImageCell;
class SolutionLetterCell;
class LetterCell;
class ClueCell;
class EmptyCell;
class KrossWordCell;

/** Different cell types for crosswords. */
enum CellType {
    EmptyCellType = 0x001,
    ClueCellType = 0x002,
    DoubleClueCellType = 0x004,
    LetterCellType = 0x008,
    SolutionLetterCellType = 0x010,
    ImageCellType = 0x020,

    AllCellTypes = EmptyCellType | ClueCellType | DoubleClueCellType
                   | LetterCellType | SolutionLetterCellType | ImageCellType,
    /**< All cell types, but not user defined cell types. */

    InteractiveCellTypes = ClueCellType | DoubleClueCellType
                           | LetterCellType | SolutionLetterCellType,
    /**< All cell types except empty cells, but not user defined cell types. */

    UserType = 0x100 /**< For user defined cell classes derived from KrossWordCell.
            * To make use of this in the flags class CellTypes, only use values computed
            * like this: value=2^x with x>=4. */
};
Q_DECLARE_FLAGS(CellTypes, CellType);

QList< CellType > allCellTypes();

typedef Coord Offset;
typedef QList<KrossWordCell*> KrossWordCellList;
typedef QList<EmptyCell*> EmptyCellList;
typedef QList<ClueCell*> ClueCellList;
typedef QList<LetterCell*> LetterCellList;
typedef QList<SolutionLetterCell*> SolutionLetterCellList;
typedef QList<ImageCell*> ImageCellList;

// LETTER CELL
/** Describes the confidence of the correctness of a letter. */
enum Confidence {
    Solved, /**< The letter is definetly correct, because it was solved. */
    Confident, /**< Confident that the letter is correct. */
    Unsure, /**< Unsure if the letter is correct. */
    Unknown /**< Confidence is unknown, eg. when loaded from a file format
            * not supporting confidence. */
};

enum EditMode {
    NoEditing,
    AutomaticKeyboardEditing,
    EmitEditRequestsOnKeyboardEdit
};

enum ClearMode {
    ClearCurrentLetter,
    ClearCorrectLetter
};

// KROSSWORDCELL
/** Different types of synchronization between KrossWordCell objects. */
enum SyncMethod {
    SyncNothing = 0x0, /**< Synchronize nothing. */
    SyncSelection = 0x1, /**< Synchronize the selection. If one KrossWordCell
            * gets selected, the other one is also selected and vice versa. */
    SyncContent = 0x2, /**< Synchronize the contents, e.g. the current letter of a
            * LetterCell. If the content of one KrossWordCell gets changed, the other
            * one is also changed and vice versa. */
    SyncAll = SyncSelection | SyncContent /**< Same as SyncSelection and
            * SyncContent together. */
};
Q_DECLARE_FLAGS(SyncMethods, SyncMethod);

enum SyncCategory {
    OtherSynchronization = 0x1,
    SolutionLetterSynchronization = 0x2,
    SameCharacterLetterSynchronization = 0x4,

    AllSyncCategories = OtherSynchronization
                        | SolutionLetterSynchronization
                        | SameCharacterLetterSynchronization
};
Q_DECLARE_FLAGS(SyncCategories, SyncCategory);

/** Where the first letter cell of the answer to a clue is, relative to
 * the clue cell position. */
enum AnswerOffset {
    OffsetInvalid,

    OnClueCell, /**< The clue cell isn't shown and the first letter is
            * placed at the coordinates of the clue. */
    OffsetTop,
    OffsetBottom,
    OffsetLeft,
    OffsetRight,
    OffsetBottomLeft,
    OffsetBottomRight,
    OffsetTopLeft,
    OffsetTopRight
};

enum KeyboardNavigationType {
    NoKeyboardNavigation = 0x000, /**< Don't change current cell on keyboard events. */

    NavigateForthOnLetterEdit = 0x001, /**< Navigate to the next letter of the
            * highlighted clue (if not at last letter), when the current letter was edited. */
    NavigateBackOnBackspace = 0x002, /**< Navigate to the previous letter of the
            * highlighted clue (if not at first letter), when the backspace key was pressed. */
    NavigateWithArrowKeys = 0x004, /**< Navigate with the arrow keys (left, up,
            * right, down). */
    NavigateJump = 0x008, /**< Enables the current cell to 'jump' over non-letter-cells
            * or to the opposite edge of the crossword grid on arrow key navigation.
            * For example when the current cell is at the right edge and the right arrow
            * key gets pressed, the cell on the left side of the grid gets selected,
            * if it's selectable.
            * Only used when @ref NavigateWithArrowKeys is set. */
    NavigateSwitchOrientationOnArrowKeyNavigation = 0x010, /**< Whether or not a
            * clue with the other orientation should be highlighted, if any. If this flag
            * is set, the orientation gets switched in these cases:
            *   @li Horizontal (across) clue is highlighted: Try to switch orientation
            *       on up and down arrow keys to vertical.
            *   @li Vertical (down) clue is highlighted: Try to switch orientation on
            *       left and right arrow keys to horizontal.
            * Only used when @ref NavigateWithArrowKeys is set. */

    DefaultKeyboardNavigation = NavigateForthOnLetterEdit
                                | NavigateBackOnBackspace
                                | NavigateWithArrowKeys
                                | NavigateJump,
    /**< Enable default keyboard navigation types. */
    AllKeyboardNavigationTypes = NavigateForthOnLetterEdit
                                 | NavigateBackOnBackspace
                                 | NavigateWithArrowKeys
                                 | NavigateJump
                                 | NavigateSwitchOrientationOnArrowKeyNavigation
                                 /**< Enable all keyboard navigation types. */
};
Q_DECLARE_FLAGS(KeyboardNavigation, KeyboardNavigationType);

/** Types of error when trying to change the crossword. For example, the
* @ref insertClue and @ref canInsertClue methods returns a value of type
* ErrorType. */
enum ErrorType {
    ErrorNone = 0x0000, /** No error. */
    DontIgnoreErrors = ErrorNone,

    ErrorClueDoesntFit = 0x0001, /**< The clue doesn't fit in the grid with the
            * given settings. */
    ErrorClueCellIsntEmpty = 0x0002, /**< The cell for a new clue isn't empty.
            * For hidden clue cells the old cell can also be a letter cell. */
    ErrorAnswerContainsIllegalCharacters = 0x0004, /**< The answer contains one
            * or more illegal characters. Allowed are characters A-Z. */
    ErrorAnswerIsIllegal = 0x0008, /**< The answer produces mismatching letter
            * cells at the same positions. */
    ErrorAnswerOverwritesClueInSameOrientation = 0x0010, /** At least one of
            * the answer cells overwrites an answer letter cell of another clue
            * cell with the same orientation. */
    ErrorAnswerCrossesClueCell = 0x0020, /**< The answer's letter cells cross
            * a clue cell. */
    ErrorClueCellsDisallowed = 0x0040, /**< (Visible) clue cells aren't allowed
            * in the current crossword type. */
    ErrorClueCellsRequired = 0x0080, /**< (Visible) clue cells are required in
            * the current crossword type. */
    ErrorAnswerIsTooShort = 0x0100, /**< The answer is shorter than allowed by
            * the current crossword type. */
    ErrorAnswerOverwritesClueCell = 0x0200, /** The answer overwrites the clue
            * cell. This only happens for horizontal clues with answer offset
            * OffsetLeft and for vertical clues with answer offset OffsetTop.
            * @note Answers with only one character actually wouldn't overwrite
            * the clue cell, but this error is given nevertheless. */

    ErrorImageDoesntFit = 0x1000, /**< The image doesn't fit in the grid with
            * the given settings. */
    ErrorImageCellsArentEmpty = 0x2000, /**< The cells for a new image aren't empty. */
    ErrorImageCellsDisallowed = 0x4000, /**< Image cells aren't allowed in
            * the current crossword type. */
};
Q_DECLARE_FLAGS(ErrorTypes, ErrorType);

/** Types of animations. */
enum AnimationType {
    NoAnimation = 0x000, /**< No animation. */

    AnimatePosChange = 0x001, /**< Animate changes to items positions. */
    AnimateSizeChange = 0x002, /**< Animate changes to items sizes. */
    AnimateAppear = 0x004, /**< Animate appearance of items. */
    AnimateDisappear = 0x008, /**< Animate disappearance of items. */
    AnimateFocusIn = 0x010, /**< Animate items when getting focus. */
    AnimateChangeLetter = 0x020, /**< Animate letter cells when changing their letter. */
    AnimateTransition = 0x040, /**< Animate changes to items appearance. */

    AllAnimations = AnimatePosChange | AnimateSizeChange | AnimateAppear
                    | AnimateDisappear | AnimateFocusIn | AnimateChangeLetter
                    | AnimateTransition /**< All animations. */
};
Q_DECLARE_FLAGS(AnimationTypes, AnimationType);

/** Types of crosswords. */
enum CrosswordType {
    UnknownCrosswordType, /**< For crosswords that are read from files without information
            * about the crossword type. */
    UserDefinedCrossword, /**< A crossword with user defined rules. */
    FreeCrossword, /**< This is a special type that allows everything. */
    American, /**< American crosswords have numbered clues and a list of clues
            * on the side of the crossword. They have usually 180 degree rotational symmetry. */
    Swedish, /**< Swedish crosswords contain clue cells which contain the clue text
            * in it and an arrow to indicate in which direction the clues have to be answered. */
    CrossNumber, /**< Crossnumbers are the numerical analogy of a crossword, in
            * which the solutions to the clues are numbers instead of words. */
    CodedPuzzle /**< Coded puzzles are a variant of crosswords in which each cell
            * has a number between 1 and 26. The solver has to find out for which letter
            * of the alphabet a number stands. */
};

/** The content of letter cells in the crossword. */
enum LetterCellContent {
    Characters, /**< Letter cells contain single characters. */
    Digits, /**< Letter cells contain single digits. */
// Strings /**< Letter cells contain one or more characters. This can be used for
//     * languages for which diacritics are respected in crosswords. For example
//     * in Czech and Slovak crosswords, 'ch' is considered one letter that
//     * occupies one cell. */
    CharactersOrDigits /**< Letter cells contain single characters or digits. */
};

/** If clue cells are allowed, required or disallowed. */
enum ClueCellHandling {
    ClueCellsAllowed, /**< Clue cells are allowed but not required. This is mainly
            * used for free style crossword types. */
    ClueCellsRequired, /**< Clue cells are required. Clues without a visible clue
            * cell are disallowed. */
    ClueCellsDisallowed /**< Clue cells are disallowed. Clues with a visible clue
            * cell are disallowed. */
    //NoClues /** The crossword has no clues, eg. for coded puzzles. */
};

/** The type of clues used by the crossword. */
enum ClueType {
    StringClues, /**< Clues are strings. */
    NumberClues1To26 /**< Clues are numbers between 1 and 26, eg. used by
            * coded puzzles. */
};

/** The type of clue mapping used by the crossword. */
enum ClueMapping {
    CluesReferToSetsOfCells, /**< Each clue refers to a set of cells, eg. a set of letter cells, ie. answers. */
    CluesReferToCells /**< Each clue refers to a single letter.
            * The crossword has a fixed set of clues, eg. numbers. This is used by
            * coded puzzles. */
};

struct CrosswordTypeInfo {
public:
    CrosswordTypeInfo() {
        *this = free();
    };

    /** Constructs a crossword type information object for user defined
    * crosswords. */
    CrosswordTypeInfo(
        const QString &name,
        const QString &description,
        const QString &longDescription,
        ClueCellHandling clueCellHandling,
        bool rotationSymmetryRequired = false,
        int minAnswerLength = 1,
        LetterCellContent letterCellContent = Characters,
        ClueMapping clueMapping = CluesReferToSetsOfCells,
        ClueType clueType = StringClues,
        CellTypes cellTypes = AllCellTypes,
        QList< QSize > defaultSizes = QList< QSize >(),
        const QString &iconName = QString()) {
        init(UserDefinedCrossword, name, description, longDescription, iconName,
             clueCellHandling, rotationSymmetryRequired, minAnswerLength,
             letterCellContent, clueMapping, clueType, cellTypes, defaultSizes);
    };

    static const QList< CrosswordType > typeList() {
        return QList< CrosswordType >()
               << American << Swedish << CrossNumber << CodedPuzzle
               << FreeCrossword << UserDefinedCrossword;
    };

    /** Constructs a crossword type information object for the given crossword
    * type. */
    static const CrosswordTypeInfo infoFromType(CrosswordType crosswordType);

    static const QString stringFromType(CrosswordType crosswordType);
    static CrosswordType typeFromString(const QString &crosswordType);

    static QList<ClueCellHandling> allClueCellHandlingValues();
    static QList<ClueType> allClueTypeValues();
    static QList<ClueMapping> allClueMappingValues();
    static QList<LetterCellContent> allLetterCellContentValues();

    static QString stringFromClueCellHandling(ClueCellHandling clueCellHandling);
    static QString stringFromClueType(ClueType clueType);
    static QString stringFromClueMapping(ClueMapping clueMapping);
    static QString stringFromLetterCellContent(LetterCellContent letterCellContent);
    static QStringList stringListFromCellTypes(CellTypes cellTypes);

    static ClueCellHandling clueCellHandlingFromString(const QString &sClueCellHandling);
    static ClueType clueTypeFromString(const QString &sClueType);
    static ClueMapping clueMappingFromString(const QString &sClueMapping);
    static LetterCellContent letterCellContentFromString(const QString &sLetterCellContent);
    static CellTypes cellTypesFromStringList(const QStringList &sCellTypes);

    static QString displayStringFromClueCellHandling(ClueCellHandling clueCellHandling);
    static QString displayStringFromClueType(ClueType clueType);
    static QString displayStringFromClueMapping(ClueMapping clueMapping);
    static QString displayStringFromLetterCellContent(LetterCellContent letterCellContent);

    static const CrosswordTypeInfo free();
    static const CrosswordTypeInfo defaultUserDefined();
    static const CrosswordTypeInfo swedish();
    static const CrosswordTypeInfo american();
    static const CrosswordTypeInfo codedPuzzle();
    static const CrosswordTypeInfo crossNumber();

    QString typeString() const {
        return stringFromType(crosswordType);
    };

    QString allowedChars() const;
    bool containsIllegalCharacters(const QString &testString) const {
        return testString.contains(QRegExp(QString("[^%1]").arg(allowedChars())));
    };
    bool isCharacterLegal(const QChar &testCharacter) const;

    CrosswordType crosswordType; /**< The type of crossword this description belongs to. */
    QString name; /**< The name of the crossword type. */
    QString iconName; /**< The name of the icon for the crossword type. */
    QString description; /**< A short description of the crossword type. */
    QString longDescription; /**< A more detailed description of the crossword type. */
    bool rotationSymmetryRequired; /**< If 180 degree rotational symmetry is required. */
    int minAnswerLength; /**< The minimal length of an answer. */
    ClueCellHandling clueCellHandling; /**< If the crossword contain clue cells,
            * with the clue text in it. If clue cells are disallowed, clues are numbered
            * and the number is shown in the first answer letter cell. */
    ClueType clueType; /**< The type of clues. */
    LetterCellContent letterCellContent; /**< The content of letter cells. */
    ClueMapping clueMapping; /**< The mapping of clues. */
    CellTypes cellTypes; /**< The cell types available for the crossword type. */
    QList< QSize > defaultSizes; /**< A list of default sizes of the crossword grid. */

private:
    CrosswordTypeInfo(CrosswordType crosswordType,
                      const QString &name,
                      const QString &description,
                      const QString &longDescription,
                      const QString &iconName,
                      ClueCellHandling clueCellHandling,
                      bool rotationSymmetryRequired = false,
                      int minAnswerLength = -1,
                      LetterCellContent letterCellContent = Characters,
                      ClueMapping clueMapping = CluesReferToSetsOfCells,
                      ClueType clueType = StringClues,
                      CellTypes cellTypes = AllCellTypes,
                      QList< QSize > defaultSizes = QList< QSize >()) {
        init(crosswordType, name, description, longDescription, iconName, clueCellHandling,
             rotationSymmetryRequired, minAnswerLength, letterCellContent,
             clueMapping, clueType, cellTypes, defaultSizes);
    };

    void init(CrosswordType crosswordType,
              const QString &name,
              const QString &description,
              const QString &longDescription,
              const QString &iconName,
              ClueCellHandling clueCellHandling,
              bool rotationSymmetryRequired = false,
              int minAnswerLength = -1,
              LetterCellContent letterCellContent = Characters,
              ClueMapping clueMapping = CluesReferToSetsOfCells,
              ClueType clueType = StringClues,
              CellTypes cellTypes = AllCellTypes,
              QList< QSize > defaultSizes = QList< QSize >()) {
        this->crosswordType = crosswordType;
        this->name = name;
        this->description = description;
        this->longDescription = longDescription;
        this->iconName = iconName.isEmpty() ? "crossword-user-defined" : iconName;
        this->clueCellHandling = clueCellHandling;
        this->rotationSymmetryRequired = rotationSymmetryRequired;
        this->minAnswerLength = minAnswerLength;
        this->letterCellContent = letterCellContent;
        this->clueMapping = clueMapping;
        this->clueType = clueType;
        this->cellTypes = cellTypes;
        this->defaultSizes = defaultSizes;
    };
}; // struct CrosswordTypeInformation

QString displayStringFromCellType(CellType cellType);
QString stringFromCellType(CellType cellType);
CellType cellTypeFromString(const QString& sCellType);

inline QDebug &operator <<(QDebug debug,
                           CrosswordType crosswordType)
{
    return debug << CrosswordTypeInfo::stringFromType(crosswordType);
};

inline QDebug &operator <<(QDebug debug,
                           ClueCellHandling clueCellHandling)
{
    return debug << CrosswordTypeInfo::stringFromClueCellHandling(
               clueCellHandling);
};

inline QDebug &operator <<(QDebug debug, ClueMapping clueMapping)
{
    return debug << CrosswordTypeInfo::stringFromClueMapping(
               clueMapping);
};

inline QDebug &operator <<(QDebug debug, ClueType clueType)
{
    return debug << CrosswordTypeInfo::stringFromClueType(clueType);
};

QDataStream &operator <<(QDataStream &stream, CrosswordTypeInfo typeInfo);
QDataStream &operator >>(QDataStream &stream, CrosswordTypeInfo &typeInfo);
}; // namespace Crossword

Q_DECLARE_OPERATORS_FOR_FLAGS(Crossword::CellTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(Crossword::SyncMethods)
Q_DECLARE_OPERATORS_FOR_FLAGS(Crossword::SyncCategories)
Q_DECLARE_OPERATORS_FOR_FLAGS(Crossword::ErrorTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(Crossword::AnimationTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(Crossword::KeyboardNavigation)
// Q_DECLARE_METATYPE( Crossword::ErrorTypes )

Coord operator +=(Coord & coord, const Crossword::Offset & offset);
Crossword::Offset operator *(const Crossword::Offset &offset, int factor);

inline bool operator<(const Coord &coord1, const Coord &coord2)
{
    return (coord1.first < coord2.first && coord1.second <= coord2.second)
           || (coord1.first <= coord2.first && coord1.second < coord2.second);
};
inline bool operator<= (const Coord &coord1, const Coord &coord2)
{
    return coord1.first <= coord2.first && coord1.second <= coord2.second;
};
inline bool operator>(const Coord &coord1, const Coord &coord2)
{
    return coord2 < coord1;
};
inline bool operator>=(const Coord &coord1, const Coord &coord2)
{
    return coord2 <= coord1;
};

inline QDebug &operator <<(QDebug debug, Crossword::SyncCategory syncCategory)
{
    switch (syncCategory) {
    case Crossword::OtherSynchronization:
        return debug << "OtherSynchronization";
    case Crossword::SolutionLetterSynchronization:
        return debug << "SolutionLetterSynchronization";
    case Crossword::SameCharacterLetterSynchronization:
        return debug << "SameCharacterLetterSynchronization";
    case Crossword::AllSyncCategories :
        return debug << "AllSyncCategories";
    default:
        return debug << "SyncCategory unknown" << syncCategory;
    }
};

inline QDebug &operator <<(QDebug debug, Crossword::SyncMethod syncMethod)
{
    switch (syncMethod) {
    case Crossword::SyncNothing:
        return debug << "SyncNothing";
    case Crossword::SyncSelection:
        return debug << "SyncSelection";
    case Crossword::SyncContent:
        return debug << "SyncContent";
    case Crossword::SyncAll:
        return debug << "SyncAll";
    default:
        return debug << "SyncMethod unknown" << syncMethod;
    }
};

inline QDebug &operator <<(QDebug debug,
                           Crossword::LetterCellContent letterCellContent)
{
    return debug << Crossword::CrosswordTypeInfo::stringFromLetterCellContent(
               letterCellContent);
};

#endif // Multiple inclusion guard
