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

#ifndef KROSSWORD_H
#define KROSSWORD_H

#include <QSizeF>

#include <KLocalizedString>
#include <QUrl>
#include <QDebug>

#include <QGraphicsObject>
#include <QGraphicsTextItem>

class CrosswordData;

class QGraphicsDropShadowEffect;

#include "global.h"

class KrosswordTheme;
class QStandardItemModel;
class ClueExpanderItem;

namespace Crossword
{
class ClueCell;
class DoubleClueCell;
class SpannedCell;
class ImageCell;

class Animator;

/**
 * @class KrosswordGrid krossword.h <Crossword>
 *
 * This is a square bidimensionnal grid for crosswords (@ref KGrid2D::Square).
 */
class KrosswordGrid : public KGrid2D::Square< KrossWordCell* >
{
public:
    explicit KrosswordGrid(uint width = 0, uint height = 0)
        : KGrid2D::Square< KrossWordCell* >(width, height) {}

    /**
      * @return the offset for the given neighbour.
      */
    static Offset neighbourOffset(Neighbour n) {
        switch (n) {
        case Left:      return Offset(-1,  0);
        case Right:     return Offset(1,  0);
        case Up:        return Offset(0, -1);
        case Down:      return Offset(0,  1);
        case LeftUp:    return Offset(-1, -1);
        case LeftDown:  return Offset(-1,  1);
        case RightUp:   return Offset(1, -1);
        case RightDown: return Offset(1,  1);
        case Nb_Neighbour: Q_ASSERT(false);
        }
        return Offset(0, 0);
    }

    static Offset letterOffset(Qt::Orientation orientation) {
        return orientation == Qt::Horizontal ? Offset(1, 0) : Offset(0, 1);
    }
};

class FocusItem : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

    Q_PROPERTY(QRectF rect READ rect WRITE setRect)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_INTERFACES(QGraphicsItem)
public:
    FocusItem(QGraphicsItem* parent = 0) : QObject(0), QGraphicsRectItem(parent) {}

};

class HeaderItem;

/** @class KrossWord krossword.h <Crossword>
 *
 * It consists of a grid of cells. You can access the cells with @ref at().
 * There are different types of cells, enumerated in @ref CellType. All share
 * the same base class @ref KrossWordCell. The type of a cell can be determined
 * with @ref cellType(). The basic cell types are: @ref EmptyCell, @ref ClueCell,
 * @ref LetterCell and @ref SolutionLetterCell.
 * There are methods to get lists of crossword cells by type: @ref cells()  gets
 * all cells of the crossword, @ref clues() gets all clue cells,
 * @ref letters() gets all letter cells. More special methods are:
 * @ref firstClue() which gets one clue cell if there is one (useful for a
 * solution crossword to easily get the solution clue), @ref emptyLetters()
 * gets all empty letter cells and @ref solutionWordLetters gets all solution
 * letter cells.
 * Letter cells are connected to up to two clue cells, a horizontal and a
 * vertical clue cell.
 * Use @ref insertClue() to insert new clues into the crossword and
 * @ref convertToSolutionLetter() to convert a letter cell into a solution letter cell.
 * A solution letter is a letter of the solution word of the crossword. Each
 * solution letter has a unique index specifying the position in the solution word.
 * You don't need to set a solution word. To check if a crossword has a solution
 * word use @ref hasSolutionWord() and @ref solutionWord() to get the solution
 * word.
 * To create a new KrossWord object that only contains one clue, which answer
 * is the solution word use @ref createSeparateSolutionKrossWord (REMOVED). This solution
 * crossword can be displayed in a toolbar or dock widget, for example. By default
 * the selection and the content of the solution letter cells is synchronized
 * between the solution crossword and the actual crossword. You can change this
 * behavior by changing the parameter, see @ref KrossWordCell::SyncMethod for
 * a list of possible synchronizations.
 * You can also manually synchronize cells with  @ref KrossWordCell::synchronizeWith()
 * and remove the synchronization again with
 * @ref KrossWordCell::removeSynchronizationWith() or remove synchronization
 * with all synchronized cells using @ref KrossWordCell::removeSynchronization().
 * KrossWord also has a method @ref KrossWord::removeSynchronization(), to remove
 * the synchronization of all cells of the crossword.
 * @brief An interactive crossword to be displayed in a QGraphicsScene.
 */
class KrossWord : public QGraphicsObject
{
    friend class KrossWordCell;
    friend class EmptyCell;
    friend class ClueCell;
    friend class LetterCell;
    friend class SolutionLetterCell;
    friend class DoubleClueCell; // To call replaceCell()
    friend class SpannedCell; // To call replaceCell()
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle)
    Q_PROPERTY(QString authors READ getAuthors WRITE setAuthors)
    Q_PROPERTY(QString copyright READ getCopyright WRITE setCopyright)
    Q_PROPERTY(QString notes READ getNotes WRITE setNotes)
    Q_PROPERTY(bool editable READ isEditable WRITE setEditable)
    Q_PROPERTY(bool interactive READ isInteractive WRITE setInteractive)

public:
    enum WriteMode {
        Normal,
        Template
    };

    enum ResizeAnchor {
        AnchorTopLeft,    AnchorTop,  AnchorTopRight,
        AnchorLeft,   AnchorCenter,   AnchorRight,
        AnchorBottomLeft, AnchorBottom,   AnchorBottomRight
    };

    enum FileFormat {
        DetermineByType,
        KwpFormat,
        KwpzFormat,
        PuzFormat
    };

    enum ConversionCommand {
        NoCommand = 0x00,

        SetupSameLetterSynchronization = 0x01,
        SetDefaultCodedPuzzleMapping = 0x02
    };
    Q_DECLARE_FLAGS(ConversionCommands, ConversionCommand)

    struct Statistics {
        int letterCellCount;
        int crossedLetterCells;
        int uncrossedLetterCells;
        QHash< QChar, int > letterCellCountByChar;

        int clueCount;
        int horizontalClues;
        int verticalClues;

        int minAnswerLength;
        int maxAnswerLength;
        float avgAnswerLength;

        int cellCount;
        int emptyCellCount;

        LetterCellList letters;
        ClueCellList clues;
        EmptyCellList emptyCells;
    }; // struct Statistics

    struct ConversionInfo {
        CrosswordTypeInfo typeInfoSource;
        CrosswordTypeInfo typeInfoTarget;

        KrossWordCellList cellsToRemove;
        ClueCellList cluesToRemove;
        QHash<ClueCell*, AnswerOffset> cluesToMakeVisible;
        QHash<LetterCell*, QChar> letterEditCorrect;
        QHash<LetterCell*, QChar> letterEditCurrent;
        ConversionCommands conversionCommands;
    }; // struct ConversionInfo

    /** Creates a new KrossWord with size @p width x @p height.
    * vertical cells. All cells are initially set to an EmptyCell. You can
    * resize the crossword later using @ref resizeGrid().
    * @param width The width of the new crossword. That is, how many horizontal
    * cells should be created.
    * @param height The height of the new crossword. That is, how many vertical
    * cells should be created.
    * @see resizeGrid() */
    explicit KrossWord(const KrosswordTheme *theme = NULL, int width = 0, int height = 0);

    virtual ~KrossWord();

    /** Gets the type information of the current crossword type. */
    CrosswordTypeInfo crosswordTypeInfo() const {
        return m_crosswordTypeInfo;
    }
    void setCrosswordTypeInfo(CrosswordTypeInfo typeInfo) {
        m_crosswordTypeInfo = typeInfo;
    }

    /** Creates and returns a model with all standard crossword types and
    * all crossword types in @p additionalTypes. */
    static QStandardItemModel *createCrosswordTypeModel(
        const QList<CrosswordTypeInfo> &additionalTypes = QList<CrosswordTypeInfo>());

    /** Gets statistics of the crossword. */
    Statistics statistics();

    inline Animator *animator() const {
        return m_animator;
    }

    void setAnimationEnabled(bool animationEnabled) {
        m_animationEnabled = animationEnabled;
    }

    inline bool isAnimationEnabled() {
        return m_animationEnabled;
    }

    void createNew(const CrosswordData &crosswordData, QByteArray *undoData);
    void createNew(const CrosswordTypeInfo &crosswordTypeInfo, const QSize &crosswordSize);

    /** This method is provided for convenience. It calls
    * @ref generateConversionInfo and then @ref executeConversionInfo,
    * if @p simulate is true.
    * @returns The conversion information to convert from the current
    * crossword type to the one described in @p newInfo. */
    ConversionInfo convertToType(CrosswordTypeInfo newInfo,
                                 bool simulate = false);
    /** Generates conversion information to convert from the current
    * crossword type to the one described in @p newInfo. To perform the
    * conversion described in the conversion information, pass it to
    * @ref executeConversionInfo. To get a string with user readable
    * information about the conversion use @ref conversionInfoToString. */
    ConversionInfo generateConversionInfo(CrosswordTypeInfo newInfo);
    /** Executes the conversion described in conversionInfo. To get
    * conversion information use @ref generateConversionInfo. */
    void executeConversionInfo(ConversionInfo conversionInfo);
    /** Returns a user readable info string describing what cells get removed
    * when performing the conversion described in @p conversionInfo. */
    QString conversionInfoToString(ConversionInfo conversionInfo);

    /** Wheater or not the crossword has 180 degree rotation symmetry. */
    bool has180DegreeRotationSymmetry() const;

    /** Gets the file format for a given @p fileName. */
    static FileFormat fileFormatFromFileName(const QString &fileName);

    /** Reads a crossword from a file.
    * @param url The URL to the file to read.
    * @param errorString Contains a string describing the error, if false was returned.
    * @return False, if there was an error. */
    bool read(const QUrl &url, QString *errorString = NULL, FileFormat fileFormat = DetermineByType, QByteArray *undoData = NULL);

    /** Convert the crossword into a data model.*/
    CrosswordData getCrosswordData(WriteMode writeMode, const QByteArray &undoData = QByteArray()); // CHECK: temporary name

    /** Write the crossword into a file.
    * @param fileName The path to the file to write to.
    * @param errorString Contains a string describing the error, if false was returned.
    * @param fileFormat The format of the file to write.
    * @param undoData Undo data to be written into the crossword file, works
    * only for XML files.
    * @return False, if there was an error. */
    bool write(const QString &fileName, QString *errorString = NULL,
               WriteMode writeMode = Normal,
               FileFormat fileFormat = DetermineByType,
               const QByteArray &undoData = QByteArray());

    /** Gets the clue cell at the coordinates @p coord with the given @p orientation.
    * If the clue cell is hidden, it gets the clue cell with @p orientation of the
    * letter cell at @p coord. If no clue cell could be found, it returns NULL. */
    ClueCell *findClueCell(const Coord &coord, Qt::Orientation orientation,
                           AnswerOffset answerOffset) const;

    /** Gets the clue cell with the given zero-based clue number @p clueNumber.
    * If no clue cell could be found, it returns NULL. */
    ClueCellList clueCellsFromClueNumber(int clueNumber) const;
    int maxClueNumber() const {
        return m_maxClueNumber;
    }

    /** The title of the crossword. */
    QString getTitle() const {
        return m_title;
    }
    /** Sets the title of the crossword to @p title. */
    void setTitle(const QString &title);

    /** The authors of the crossword. */
    QString getAuthors() const {
        return m_authors;
    }
    /** Sets the authors string to @p authors. */
    void setAuthors(const QString &authors);

    /** The copyright information string of the crossword. */
    QString getCopyright() const {
        return m_copyright;
    }
    /** Sets the copyright information string to @p copyright. */
    void setCopyright(const QString &copyright);

    /** Notes of the crossword. */
    QString getNotes() const {
        return m_notes;
    }
    /** Sets the notes of the crossword to @p notes. */
    void setNotes(const QString &notes);

    /** Returns the percentage of solved letter cells. */
    float solutionProgress() const;

    /** Gets the span of empty cells from the given coordinates @p coordTopLeft. */
    QSize emptyCellSpan(const Coord &coordTopLeft, SpannedCell *excludedCell = 0);

    bool canTakeSpannedCell(const Coord &coord,
                            int horizontalCellSpan, int verticalCellSpan,
                            SpannedCell* excludedSpannedCell = NULL) const;

    /** Inserts a new image into the crossword at @p coord.
    * @note This method first checks if the new image with the given span
    * values will fit into the current crossword by calling
    * @ref canInsertImage().
    * @param coord The top left coordinates of the new image cell.
    * @param horizontalCellSpan The number of horizontal cells the image should
    * be spanned.
    * @param verticalCellSpan The number of vertical cells the image should
    * be spanned.
    * @param errorTypesToIgnore Error types to ignore.
    * @param insertedImage A pointer to the newly created image cell will
    * be put into @c *insertedImage, if @p insertedImage isn't NULL.
    * If another value than ErrorType::NoError is returned, @c *insertedClue
    * won't be changed.
    * @returns ErrorType::NoError, if the image with the given span values
    * could be inserted successfully.
    * @returns Other values of ErrorType, if there was an error when inserting
    * the image.
    * @see canInsertClue() */
    ErrorType insertImage(const KGrid2D::Coord &coord,
                          int horizontalCellSpan, int verticalCellSpan, QUrl url,
                          ErrorTypes errorTypesToIgnore = DontIgnoreErrors,
                          ImageCell **insertedImage = NULL);

    /** Gets a list with all legal answer offsets for a new clue at the given
    * coordinates @p clueCellCoord with the given @p orientation. You can set
    * excludeClue to a clue cell to exclude when generating the list of legal
    * answer offsets. Legal are all answer offsets that can be used to insert
    * new clues at @p clueCellCoord with @p orientation. Illegal are answer
    * offsets that would produce some type of error, ie. @ref canInsertClue
    * would return a value other than ErrorType::NoErrorType. If, for example
    * a clue cell is at the coordinates (clue coordinates + answer offset),
    * then that answer offset would be illegal. */
    QList< AnswerOffset > legalAnswerOffsets(
        Coord clueCellCoord, Qt::Orientation orientation,
        int answerLength,
        ClueCell *excludedClue = NULL) const;
    bool canTakeClueCell(const Coord &coord,
                         const Offset &firstLetterOffset = Offset(0, 0),
                         bool allowDoubleClueCells = true,
                         ClueCell *excludedClue = NULL) const;

    /** Inserts a new clue into the crossword at @p coord. This method won't
    * overwrite existing cells, except for those of type EmptyCell. Existing cells
    * of type LetterCell are merged with the new clue (each LetterCell can
    * have a horizontal and a vertical clue). It is checked that the correct
    * letters of existing letter cells match the ones of newly inserted ones.
    * @note This method first checks if the new clue with it's answer letter
    * cells will fit into the current crossword by calling @ref canInsertClue().
    * @param coord The coordinates of the new clue cell.
    * @param orientation The orientation of the clue, may be either
    * Qt::Horizontal or Qt::Vertical.
    * @param answerOffset The position of the first letter of the answer
    * relative to the position of the clue.
    * @param clue The clue string.
    * @param answer The answer to the clue. For each letter of the answer a
    * LetterCell object will be created.
    * @param cellType The cell type of newly created letter cells. This is used
    * to create a solution clue with letter cells of type SolutionLetterCell.
    * @param errorTypesToIgnore Error types to ignore.
    * @param allowDoubleClueCells Whether or not to allow the creation of
    * double clue cells if needed, ie. if there's already a clue cell at @p coord.
    * @param insertedClue A pointer to the newly created clue cell will
    * be put into @c *insertedClue, if @p insertedClue isn't NULL.
    * If another value than ErrorType::NoError is returned, @c *insertedClue
    * won't be changed.
    * @returns ErrorType::NoError, if the clue and answer letter cells could
    * be inserted successfully.
    * @returns Other values of ErrorType, if there was an error when inserting
    * the clue and it's answers letter cells.
    * @see canInsertClue() */
    ErrorType insertClue(const KGrid2D::Coord &coord,
                         Qt::Orientation orientation,
                         AnswerOffset answerOffset,
                         const QString &clue, const QString &answer,
                         CellType cellType = LetterCellType,
                         ErrorTypes errorTypesToIgnore = DontIgnoreErrors,
                         bool allowDoubleClueCells = true,
                         ClueCell **insertedClue = NULL);

    /** Returns true, if a clue cell with it's answer letters fits into the
    * current crossword at @p coord.
    * @param coord The coordinates of the clue cell.
    * @param orientation The orientation of the clue, may be either
    * Qt::Horizontal or Qt::Vertical.
    * @param answerOffset The position of the first letter of the answer
    * relative to the position of the clue.
    * @param answer The answer to the clue.
    * @param errorTypesToIgnore Error types to ignore.
    * @param allowDoubleClueCells Whether or not to allow the creation of
    * double clue cells if needed, ie. if there's already a clue cell at @p coord.
    * @param excludedClue You can specify a clue cell which shouldn't be taken
    * into account when checking if the new clue can be inserted. */
    ErrorType canInsertClue(const KGrid2D::Coord &coord,
                            Qt::Orientation orientation,
                            const Offset &answerOffset,
                            const QString &answer,
                            ErrorTypes errorTypesToIgnore =
                                DontIgnoreErrors,
                            bool allowDoubleClueCells = true,
                            ClueCell *excludedClue = NULL);

    ErrorType changeClueProperties(ClueCell *clue,
                                   Qt::Orientation newOrientation,
                                   AnswerOffset newAnswerOffset,
                                   const QString &newCorrectAnswer,
                                   ErrorTypes errorTypesToIgnore = DontIgnoreErrors,
                                   bool allowDoubleClueCells = true);

    ErrorType canChangeClueProperties(ClueCell *clue,
                                      Qt::Orientation newOrientation,
                                      Offset newAnswerOffset,
                                      const QString &newCorrectAnswer,
                                      ErrorTypes errorTypesToIgnore =
                                          DontIgnoreErrors,
                                      bool allowDoubleClueCells = true,
                                      SolutionLetterCellList *removedSolutionLetterCells = NULL);

    /** Returns true, if an image cell with it's horizontal and vertical cell
    * span fits into the current crossword at @p coord.
    * @param coord The coordinates of the clue cell.
    * @param horizontalCellSpan The number of horizontal cells over which the
    * image should be spanned.
    * @param verticalCellSpan The number of vertical cells over which the
    * image should be spanned.
    * @param errorTypesToIgnore Error types to ignore.
    * @param excludedClue You can specify a clue cell which shouldn't be taken
    * into account when checking if the new clue can be inserted. */
    ErrorType canInsertImage(const KGrid2D::Coord &coord,
                             int horizontalCellSpan, int verticalCellSpan,
                             ErrorTypes errorTypesToIgnore = DontIgnoreErrors,
                             ImageCell *excludedImage = NULL);

    /** Removes the given clue from the crossword. It also removes answer
    * letters of the clue, if the letters have no second clue. Otherwise
    * it just detaches the clue from the letters.
    * @param clue The clue to be removed. */
    inline QList<Coord> removeClue(ClueCell* clue) {
        return removeClue(clue, RemoveFromGridAndDelete);
    }

    /** Removes the given image from the crossword.
    * @param clue The image to be removed. */
    void removeImage(ImageCell* image);

    /** Gets the maximal space for a new answer starting at @p coord in
    * direction @p orientation, but excluding @p excludedClue from the
    * calculation unless it's NULL.
    * @return The number of empty cells or letter cells which doesn't already
    * have a clue in @p orientation. These cells can be used for the answer
    * letter cells of a clue cell. */
    int maxAnswerLengthAt(const Coord &coord, Qt::Orientation orientation,
                          ClueCell *excludedClue = NULL);

    bool correctLettersAt(const Coord &coord, Qt::Orientation orientation,
                          int letterCount, QString *correctLetters,
                          ClueCell *excludeClue = NULL);

    void setupSameLetterSynchronization();
    void removeSameLetterSynchronization();
    void removeSynchronization(
        SyncMethods syncMethods = SyncAll,
        SyncCategories syncCategories =
            AllSyncCategories);

    /** Returns a list of all cells of the crossword.
    * @see KrossWordCell::cellType() */
    KrossWordCellList cells(CellTypes cellTypes = AllCellTypes) const;
    /** Returns a list of cells beginning with the cell at @p coord and going
    * in direction @p orientation.
    * @param coord The coordinates of the first cell in the returned list.
    * @param orientation The direction to go to collect the cells.
    * @param count The maximal number of cells or -1 to get all cells in the
    * given direction. */
    KrossWordCellList cells(const Coord &coord,
                            Qt::Orientation orientation,
                            int count = -1) const;
    /** Returns a list of empty cells of the crossword.
    * @see KrossWordCell::cellType() */
    EmptyCellList emptyCells() const;
    /** Returns a list of all clues of the crossword. */
    ClueCellList clues() const {
        return m_clues;
    }
    /** Gets all horizontal and vertical clues, sorted by clue number (if the
    * clues have a clue number). */
    void clues(ClueCellList *horizontalClues,
               ClueCellList *verticalClues) const;
    /** Returns a list of all letters of the crossword. */
    LetterCellList letters() const;
    /** Returns a list of all empty letters of the crossword. */
    LetterCellList emptyLetters() const;
    /** Returns a list of all letters that form the solution word of the crossword. */
    SolutionLetterCellList solutionWordLetters() const {
        return m_solutionLetters;
    }
    /** Returns a list of all images of the crossword. */
    ImageCellList images() const;

    /** Solve the crossword automatically by filling all correct letters into
    * the letter cells of this crossword. */
    void solve();
    /** Check if the crossword is correctly filled with letters.
    * @returns true If all letters are filled and correct.
    * @returns false Otherwise. */
    bool check() const;
    /** Clears all letter cells. */
    void clear();
    /** Resizes the crossword grid. The grid is then filled with empty cells.
    * @param width The new width.
    * @param height The new height.
    * @param simulate If true, no resizing is actually done, but the cells that
    * would have been are returned. If false, the crossword grid is resized.
    * @returns A list of cells that were removed. */
    KrossWordCellList resizeGrid(uint width, uint height,
                                 ResizeAnchor anchor = AnchorCenter, bool simulate = false);
    KrossWordCellList moveCells(int dx, int dy, bool simulate = false);
    /** Removes all cells, replacing them with empty cells.
    * @note To really delete all cells, you can resize the crossword to 0x0.
    * @see resizeGrid() */
    void removeAllCells();
    /** Returns the size of one crossword cell. */
    QSizeF getCellSize() const {
        return m_cellSize;
    }
    /** Checks if this crossword has a solution word. It checks if there are any
    * solution letter cells. */
    bool hasSolutionWord() const {
        return !solutionWordLetters().isEmpty();
    }
    /** Gets the correct solution word.
    * @see currentSolutionWord */
    QString solutionWord(const QChar &charNonLinkedLetters = ' ') const;
    /** Gets the solution word as it is currently solved. Not solved (empty)
    * letter cells generate a '-' in the returned string.
    * @see solutionWord */
    QString currentSolutionWord() const;
    /** Gets the currently highlighted clue.
    * @see setHighlightedClue */
    ClueCell *highlightedClue() const {
        return m_highlightedClue;
    }
    ClueCell *previousHighlightedClue() const {
        return m_previousHighlightedClue;
    }
    /** Sets the highlighted clue. All letter cells and the clue cell
    * are then highlighted. A previously highlighted clue will no longer be
    * highlighted.
    * @param clue The clue to highlight or NULL to remove all highlights.
    * @see highlightedClue */
    void setHighlightedClue(ClueCell *clue);

    void applyLetterContentToClueNumberMapping(const QString &codedPuzzleMapping);
    void setLetterContentToClueNumberMapping(const QString &codedPuzzleMapping, bool apply = true);
    QString letterContentToClueNumberMapping() const {
        return m_codedPuzzleMapping;
    }
    static const QString defaultCodedPuzzleMapping() {
        return "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    }

    QString contentString() const;
    QString synchronizationString() const;

    /** Gets the crossword cell at specified coordinates.
    * @param coord The coordinates of the cell to get. Must be bigger than or
    * equal to (0, 0) and smaller than (@ref width(), @ref height()). You can
    * check if coordinates are inside the crossword grid with @ref inside().
    * @see Coord
    * @see inside */
    inline KrossWordCell *at(Coord coord) const {
        if (!inside(coord)) {
            qDebug() << coord << "isn't inside the grid! Returning NULL.";
            return NULL;
        }
        return m_krossWordGrid->at(coord);
    }

    /** Gets the width of the crossword grid. */
    inline uint width() const {
        return m_krossWordGrid->width();
    }
    /** Gets the height of the crossword grid. */
    inline uint height() const {
        return m_krossWordGrid->height();
    }
    /** Returns true, if the given coordinates are inside the crossword grid. */
    inline bool inside(Coord coord) const {
        return m_krossWordGrid->inside(coord);
    }

    inline KrossWordCell *const&operator[](const Coord &coord) const {
        return m_krossWordGrid->operator[](coord);
    }
    inline KrossWordCell *&operator[](const Coord &coord) {
        return m_krossWordGrid->operator[](coord);
    }

    void removeSolutionSynchronizationTo(KrossWord *solutionKrossWord);
    QPixmap toPixmap(const QSize &size = QSize(64, 64));
    void assignClueNumbers();

    /** Gets an error message from a error type value. You can use this error
    * message to explain to the user why some operation couldn't be processed
    * (eg. why a clue couldn't be inserted).
    * @see canInsertClue()
    * @see canInsertImage() */
    static QString errorMessageFromErrorType(ErrorType errorType);

    //static AnswerOffset answerOffsetFromString(const QString &s); // CHECK: to remove
    //static QString answerOffsetToString(AnswerOffset answerOffset); // CHECK: to remove

    /** Checks if the crossword is empty, ie. contains no clues. */
    bool isEmpty() const;

    /** Returns whether this crossword is editable. When editable, letter cells
    * show their correct letter instead of their current letter. Editing letter
    * cells when the crossword is editable also changes their correct letters.
    * Empty cells are drawn and can be highlighted in edit mode.
    * @see setEditable.
    * @see isInteractive. */
    bool isEditable() const {
        return m_editable;
    }
    /** Enables / disables the edit mode for the crossword.
    * @see isEditable.
    * @see setInteractive. */
    void setEditable(bool editable = true);

    bool isInteractive() const {
        return m_interactive;
    }
    void setInteractive(bool interactive = true);

    bool isDrawingForPrinting() const {
        return m_drawForPrinting;
    }
    void setDrawForPrinting(bool drawForPrinting = true);

    QColor emptyCellColorForPrinting() const {
        return m_emptyCellColorForPrinting;
    }
    void setEmptyCellColorForPrinting(const QColor &color) {
        m_emptyCellColorForPrinting = color;
    }

    /** Returns the minimal size of the crossword to include all current cells,
    * ie. the "bounding rect" of all non-empty cells. */
    QSize minimalSize() const;

    KrossWordCell *currentCell() const {
        return m_currentCell;
    }
    KrossWordCell *previousCell() const {
        return m_previousCell;
    }

    /** Gets the edit mode of letter cells. */
    EditMode letterEditMode() const {
        return m_letterEditMode;
    }
    /** Changes the edit mode of all letter cells. */
    void setLetterEditMode(EditMode editMode) {
        m_letterEditMode = editMode;
    }

    KeyboardNavigation keyboardNavigation() const {
        return m_keyboardNavigation;
    }
    void setKeyboardNavigation(KeyboardNavigation keyboardNavigation =
                                   DefaultKeyboardNavigation) {
        m_keyboardNavigation = keyboardNavigation;
    }

//     void enableSignalAnswerChanged( bool enable ) {
//  m_signalAnswerChanged = enable; };
//     bool isSignalAnswerChangedEnabled() const { return m_signalAnswerChanged; };

    virtual QRectF boundingRect() const;

    const KrosswordTheme *theme() const {
        return m_theme;
    }
    void setTheme(const KrosswordTheme *theme);
    
protected:
    enum RemoveMode {
        DontRemove,
        RemoveFromGrid,
        RemoveFromGridAndDelete
    };

    void insertCluePostProcessing(ClueCell *clue);
    QList<Coord> removeClue(ClueCell* clue,
                            RemoveMode clueCellRemoveMode,
                            RemoveMode letterCellsRemoveMode = RemoveFromGridAndDelete);

    /** Returns a non-const list of all solution letter cells. */
//     SolutionLetterCellList &solutionWordLettersNonConst() { return m_solutionLetters; };

    /** Deletes all cells. The crossword grid will only contain NULL pointers
    * afterwards so use with care. This method is used by @ref resizeGrid().
    * @see removeAllCells() */
    void deleteAllCells();
    /** Removes the cell @p cell and inserts the cell @p newCell at the same
    * coordinates.
    * @param cell The cell to replace.
    * @param newCell The cell to insert at the coordinates of the old cell after
    * removing it. This is needed to assure consistency. Should not be NULL.
    * @see removeCell */
    void replaceCell(KrossWordCell *cell, KrossWordCell *newCell);
    /** Removes the cell at the given coordinates and inserts the new @p cell.
    * @param coord The coordinates of the cell to replace.
    * @param cell The cell to insert at the given coordinates after removing
    * the old one. This is needed to assure consistency. Should not be NULL.
    * @see removeCell */
    inline void replaceCell(const Coord &coord, KrossWordCell *newCell) {
        replaceCell(coord, newCell, true);
    };
    /** Removes the cell @p cell and inserts an empty cell instead.
    * @param cell The cell to remove.
    * @see replaceCell*/
    void removeCell(KrossWordCell *cell);
    /** Removes the cell at the given coordinates and inserts an empty cell instead.
    * @param coord The coordinates of the cell to remove.
    * @see replaceCell */
    void removeCell(const Coord &coord);
//     void switchCellPositions( KrossWordCell *cell1, KrossWordCell *cell2 ); TODO

    // Maybe this should also be done with signals/slots..? I think this way it's
    // easier, because the cells don't need all to be connected.
    void emitCustomContextMenuRequested(const QPointF &pos, KrossWordCell *cell) {
        emit customContextMenuRequested(pos, cell);
    };
    void emitLetterEditRequest(LetterCell *letter, const QChar &currentLetter,
                               const QChar &newLetter) {
        emit letterEditRequest(letter, currentLetter, newLetter);
    };
    void emitMousePressed(const QPointF &pos, Qt::MouseButton button,
                          KrossWordCell *cell) {
        emit mousePressed(pos, button, cell);
    };

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                       QWidget* widget = 0);

signals:
    /** This signal is emitted when new clues are added. When reading crosswords
    * it will only be emitted once with a list of all new clues. A call to @ref insertClue
    * emits this signal with only the inserted clue in the list. */
    void cluesAdded(ClueCellList clues);

    /** This signal is emitted when existing clues will get removed. When clearing
    * crosswords it will only be emitted once with a list of all clues to be removed.
    * A call to @ref removeClue emits this signal with only the removed clue in the
    * list. */
    void cluesAboutToBeRemoved(ClueCellList clues);

    void solutionWordLetterAdded(SolutionLetterCell *solutionLetter);
    void solutionWordLetterRemoved(SolutionLetterCell *solutionLetter);

    /** Right mouse button was pressed on a cell. */
    void customContextMenuRequested(const QPointF &pos, KrossWordCell *cell);

    void mousePressed(const QPointF &pos, Qt::MouseButton button, KrossWordCell *cell);
    void mouseEnteredWhilePressed(const QPointF &pos, Qt::MouseButton button, KrossWordCell *cell);

    /** The currently selected cell has been changed. */
    void currentCellChanged(KrossWordCell *currentCell, KrossWordCell *previousCell);
    /** Another clue was selected. */
    void currentClueChanged(ClueCell *clue);
    /** The currently selected answer has been changed. */
    void answerChanged(ClueCell *clue, const QString &currentAnswer);
    /** A solution letter has been changed. */
    void solutionLetterChanged(SolutionLetterCell *solutionLetter,
                               const QString &currentSolutionWord, int changedLetterIndex);

    /** Requests changing the current letter of @p letter from @p currentLetter
    * to @p newLetter.
    * To be used with QUndoCommand to have control over what's changed. */
    void letterEditRequest(LetterCell *letter, const QChar &currentLetter,
                           const QChar &newLetter);
    void addLettersToClueRequest(ClueCell *clue, int lettersToAdd);

    void editModeChanged(bool editable);

    void gridResized(KrossWord *krossWord, int columns, int rows);

public slots:
    void setCurrentCell(KrossWordCell *cell);
    void clearCache();

protected slots:
    void addLettersToClueRequestSlot(ClueCell *clue, int lettersToAdd) {
        emit addLettersToClueRequest(clue, lettersToAdd);
    };
    /** An answer of the crossword has been changed. */
    void answerChangedSlot(ClueCell *clue, const QString &currentAnswer);
    /** A letter of the solution word has been changed. */
    void solutionLetterChanged(LetterCell *letter, const QChar &newLetter);
    /** A crossword cell has received focus. */
    void focusCellChanged(KrossWordCell *currentCell);

    void currentCellDestroyed(QObject*);

private:
    void replaceCell(const Coord& coord, KrossWordCell *newCell,
                     bool deleteOldCell);
    void removeCell(const Coord &coord, bool deleteOldCell);
    KrossWordCellList invalidateCellRegion(const Coord &coordTopLeft,
                                           const Coord &coordBottomRight,
                                           bool simulate = false);
    KrossWordCellList invalidateCell(const Coord &coord,
                                     bool simulate = false);

    void init(uint width = 0, uint height = 0);
    void fillWithEmptyCells();
    void fillWithEmptyCells(const Coord &coordTopLeft,
                            const Coord &coordBottomRight);
    bool canTakeClueLetterCell(const Coord &coord, Qt::Orientation orientation,
                               ClueCell *excludedClue = NULL);
    bool isCellEmptyIfClueIsExcluded(const Coord &coord,
                                     ClueCell *excludedClue) const;
    bool isCellEmptyIfSpannedCellIsExcluded(const Coord &coord,
                                            SpannedCell* excludedSpannedCell) const;

    //void setTopLeftCellOffset(const QPointF &topLeftCellOffset);
    void updateHeaderItem();

    Animator *m_animator;

    KrosswordGrid *m_krossWordGrid; // Stores all cells in the crossword
    QSizeF m_cellSize; // The size of one crossword cell
    ClueCellList m_clues; // A list of all clues
    SolutionLetterCellList m_solutionLetters; // A list of all solution letters
    QHash< ClueCell*, ClueExpanderItem* > m_clueExpanderItems; // A list of all clue expanders
    KrossWordCell *m_currentCell, *m_previousCell;
    ClueCell *m_highlightedClue, *m_previousHighlightedClue;
    EditMode m_letterEditMode;
    KeyboardNavigation m_keyboardNavigation;
    QColor m_emptyCellColorForPrinting; // The color of empty/blank cells when printing the crossword
    int m_maxClueNumber; // The maximum assigned clue number

    QString m_title; // Title of the crossword
    QString m_authors; // Authors of the crossword
    QString m_copyright; // Copyright information
    QString m_notes; // Other notes for the crossword

//     bool m_signalAnswerChanged;
    bool m_interactive; // Stores whether or not the crossword is currently interactive
    bool m_editable; // Stores whether or not the crossword is currently editable
    bool m_drawForPrinting; // Stores whether or not the crossword should be drawn for printing

    CrosswordTypeInfo m_crosswordTypeInfo; // Stores information about the crossword type

    // Only used if crosswordType is CodedPuzzle:
    // Stores how characters are mapped to (number) clues. Each character in the
    // string is mapped to the clue with the number n = i + 1, where i is the index
    // of that character in the string. Note: 1 <= n <= 26.
    QString m_codedPuzzleMapping;

    bool m_animationEnabled;

    FocusItem *m_focusItem;
    QGraphicsTextItem *m_headerItem;

    const KrosswordTheme *m_theme;
}; // class KrossWord



inline QDebug &operator <<(QDebug debug, KrossWord *krossWord)
{
    return debug << krossWord->contentString();
};

inline QDebug &operator <<(QDebug debug,
                           KrossWord::ConversionCommand conversionCommand)
{
    switch (conversionCommand) {
    case KrossWord::NoCommand:
        return debug << "No command";
    case KrossWord::SetupSameLetterSynchronization:
        return debug << "SetupSameLetterSynchronization";
    case KrossWord::SetDefaultCodedPuzzleMapping:
        return debug << "SetDefaultCodedPuzzleMapping";
    default:
        return debug << "ConversionCommand unknown" << conversionCommand;
    }
};

}; // namespace Crossword

Q_DECLARE_METATYPE(Crossword::CrosswordTypeInfo)


inline QDebug &operator <<(QDebug debug, Qt::Orientation orientation)
{
    return debug << (orientation == Qt::Horizontal ? "Horizontal" : "Vertical");
};

inline QDebug &operator <<(QDebug debug, Crossword::ErrorType errorType)
{
    return debug << Crossword::KrossWord::errorMessageFromErrorType(errorType);
};

inline QDebug &operator <<(QDebug debug, Crossword::KrossWord::ResizeAnchor anchor)
{
    switch (anchor) {
    case Crossword::KrossWord::AnchorCenter:
        return debug << "AnchorCenter";
    case Crossword::KrossWord::AnchorLeft:
        return debug << "AnchorLeft";
    case Crossword::KrossWord::AnchorRight:
        return debug << "AnchorRight";
    case Crossword::KrossWord::AnchorTopLeft:
        return debug << "AnchorTopLeft";
    case Crossword::KrossWord::AnchorTop:
        return debug << "AnchorTop";
    case Crossword::KrossWord::AnchorTopRight:
        return debug << "AnchorTopRight";
    case Crossword::KrossWord::AnchorBottomLeft:
        return debug << "AnchorBottomLeft";
    case Crossword::KrossWord::AnchorBottom:
        return debug << "AnchorBottom";
    case Crossword::KrossWord::AnchorBottomRight:
        return debug << "AnchorBottomRight";
    default:
        return debug << "ResizeAnchor unknown" << anchor;
    }
};

#endif // KROSSWORD_H
