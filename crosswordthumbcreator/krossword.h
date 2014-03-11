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
#include "kgrid2d.h"

#include "cells/krosswordcell.h"
#include "cells/cluecell.h"
#include <KLocalizedString>
#include <kdeversion.h>

#if QT_VERSION >= 0x040600
#include <QGraphicsObject>
#endif

class SpannedCell;
class ImageCell;

/** @class KrossWord
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
* @ref convertToSolutionLetter() to convert an empty cell into a solution letter.
* A solution letter is a letter of the solution word of the crossword. Each
* solution letter has a unique index specifying the position in the solution word.
* You don't need to set a solution word. To check if a crossword has a solution
* word use @ref hasSolutionWord() and @ref solutionWord() to get the solution
* word.
* To create a new KrossWord object that only contains one clue, which answer
* is the solution word use @ref createSeperateSolutionKrossWord. This solution
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
* @brief An interactive crossword to be displayed in a QGraphicsScene. */

#if QT_VERSION >= 0x040600
class KrossWord : public QGraphicsObject {
#else
class KrossWord : public QObject, public QGraphicsItem {
#endif
    friend class KrossWordCell;
    friend class ClueCell;
    friend class LetterCell;
    friend class SolutionLetterCell;
    Q_OBJECT
#if QT_VERSION >= 0x040600
    Q_INTERFACES( QGraphicsItem )
#endif

public:
    /** Types of error when trying to change the crossword. For example, the
    * @ref insertClue method returns a value of type Correctness. */
    enum ErrorType {
        NoError = 0x0000, /** No error. */
	DontIgnoreErrors = NoError,

        ErrorClueDoesntFit = 0x0001, /**< The clue doesn't fit in the grid with the given settings. */
        ErrorClueCellIsntEmpty = 0x0002, /**< The cell for a new clue isn't empty. For hidden
					    clue cells the old cell can also be a letter cell. */
	ErrorAnswerContainsIllegalCharacters = 0x0004, /**< The answer contains one or more illegal
								characters. Allowed are characters A-Z. */
        ErrorAnswerIsIllegal = 0x0008, /**< The answer produces mismatching letter cells
									at the same positions. */
	ErrorAnswerOverwritesClueInSameOrientation = 0x0010, /** At least one of the answer cells
	    overwrites an answer letter cell of another clue cell with the same orientation. */
        ErrorAnswerCrossesClueCell = 0x0020, /**< The answer's letter cells cross a clue cell. */

	ErrorImageDoesntFit = 0x0100, /**< The image doesn't fit in the grid with the given settings. */
	ErrorImageCellsArentEmpty = 0x0200 /**< The cells for a new image aren't empty. */
    };
    Q_DECLARE_FLAGS( ErrorTypes, ErrorType ); // TODO Better naming...

    enum FileFormat {
	DetermineByFileName,
	KrossWordPuzzleXmlFile,
	KrossWordPuzzleCompressedXmlFile,
	AcrossLitePuzFile
    };

    /** Creates a new KrossWord with @p width horizontal and @p height
    * vertical cells. All cells are initially set to an EmptyCell. You can
    * resize the crossword later using @ref resizeGrid().
    * @param scene The scene in which this crossword should be displayed. Can
    * be NULL, but then you need to call setScene() later.
    * @param width The width of the new crossword. That is, how many horizontal
    * cells should be created.
    * @param height The height of the new crossword. That is, how many vertical
    * cells should be created.
    * @see resizeGrid() */
    explicit KrossWord( int width = 0, int height = 0 );

    virtual ~KrossWord();

    /** Reads a crossword from a file.
    * @param url The URL to the file to read.
    * @param errorString Contains a string describing the error, if false was returned.
    * @return False, if there was an error. */
    bool read( const KUrl &url, QString *errorString = NULL,
	FileFormat fileFormat = DetermineByFileName );

    static FileFormat fileFormatFromFileName( const QString &fileName );

    QImage toImage( const QSize &size = QSize(64, 64) );
    void resizeTo( const QSizeF &size );
    void assignClueNumbers();

    /** Gets the clue cell at the coordinates @p coord with the given @p orientation.
    * If the clue cell is hidden, it gets the clue cell with @p orientation of the
    * letter cell at @p coord. If no clue cell could be found, it returns NULL. */
    ClueCell *findClueCell( const Coord &coord, Qt::Orientation orientation ) const;

    QString title() const { return m_title; };
    void setTitle( const QString &title ) { m_title = title; };
    QString authors() const { return m_authors; };
    void setAuthors( const QString &authors ) { m_authors = authors; };
    QString copyright() const { return m_copyright; };
    void setCopyright( const QString &copyright ) { m_copyright = copyright; };
    QString notes() const { return m_notes; };
    void setNotes( const QString &notes ) { m_notes = notes; };

    bool canTakeSpannedCell( const Coord &coord,
		    int horizontalCellSpan, int verticalCellSpan,
		    SpannedCell* excludedSpannedCell = NULL ) const;

    ErrorType insertImage( const KGrid2D::Coord &coord,
	    int horizontalCellSpan, int verticalCellSpan, KUrl url,
	    ErrorTypes correctnessesToIgnore = DontIgnoreErrors,
	    ImageCell **insertedImage = NULL );

    bool canTakeClueCell( const Coord &coord,
		    const Offset &firstLetterOffset = Offset(0, 0),
		    bool allowDoubleClueCells = true,
		    ClueCell *excludedClue = NULL ) const;

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
    * @param correctnessesToIgnore Correctness values to ignore.
    * @param allowDoubleClueCells Whether or not to allow the creation of
    * double clue cells if needed, ie. if there's already a clue cell at @p coord.
    * @param insertedClue A pointer to the newly created clue cell will
    * be put into @c *insertedClue, if @p insertedClue isn't NULL.
    * If another value than Correctness::Correct is returned, @c *insertedClue
    * won't be changed.
    * @returns Correctness::Correct, if the clue and answer letter cells could
    * be inserted successfully.
    * @returns Other values of Correctness, if there was an error when inserting
    * the new clue and it's answers letter cells.
    * @see canInsertClue() */
    ErrorType insertClue( const KGrid2D::Coord &coord,
	    Qt::Orientation orientation, ClueCell::AnswerOffset answerOffset,
	    const QString &clue, const QString &answer,
	    KrossWordCell::CellType cellType = KrossWordCell::LetterCellType,
	    ErrorTypes correctnessesToIgnore = DontIgnoreErrors,
	    bool allowDoubleClueCells = true, ClueCell **insertedClue = NULL );

    /** Returns true, if a clue cell with it's answer letters fits into the
    * current crossword at @p coord.
    * @param coord The coordinates of the clue cell.
    * @param orientation The orientation of the clue, may be either
    * Qt::Horizontal or Qt::Vertical.
    * @param answerOffset The position of the first letter of the answer
    * relative to the position of the clue.
    * @param answer The answer to the clue.
    * @param correctnessesToIgnore Correctness values to ignore.
    * @param allowDoubleClueCells Whether or not to allow the creation of
    * double clue cells if needed, ie. if there's already a clue cell at @p coord.
    * @param excludedClue You can specify a clue cell which shouldn't be taken
    * into account when checking if the new clue can be inserted. */
    ErrorType canInsertClue( const KGrid2D::Coord &coord, Qt::Orientation orientation,
	    const Offset &answerOffset, const QString &answer,
	    ErrorTypes correctnessesToIgnore = DontIgnoreErrors,
	    bool allowDoubleClueCells = true, ClueCell *excludedClue = NULL );

    /** Returns true, if an image cell with it's horizontal and vertical cell
    * span fits into the current crossword at @p coord.
    * @param coord The coordinates of the clue cell.
    * @param horizontalCellSpan The number of horizontal cells over which the
    * image should be spanned.
    * @param verticalCellSpan The number of vertical cells over which the
    * image should be spanned.
    * @param correctnessesToIgnore Correctness values to ignore.
    * @param excludedClue You can specify a clue cell which shouldn't be taken
    * into account when checking if the new clue can be inserted. */
    ErrorType canInsertImage( const KGrid2D::Coord &coord,
	    int horizontalCellSpan, int verticalCellSpan,
	    ErrorTypes correctnessesToIgnore = DontIgnoreErrors,
	    ImageCell *excludedImage = NULL );

    /** Convert a LetterCell to a SolutionLetterCell.
    * @note There must be a letter cell at the given coordinates. */
    bool convertToSolutionLetter( const KGrid2D::Coord &coord, int solutionLetterIndex );

    /** Returns a list of all cells of the crossword.
    * @see KrossWordCell::cellType() */
    KrossWordCellList cells( KrossWordCell::CellTypes cellTypes = KrossWordCell::AllCellTypes ) const;
    /** Returns a list of cells beginning with the cell at @p coord and going
    * in direction @p orientation.
    * @param coord The coordinates of the first cell in the returned list.
    * @param orientation The direction to go to collect the cells.
    * @param count The maximal number of cells or -1 to get all cells in the
    * given direction. */
    KrossWordCellList cells( const Coord &coord, Qt::Orientation orientation, int count = -1 ) const;
    /** Returns a list of empty cells of the crossword.
    * @see KrossWordCell::cellType() */
    EmptyCellList emptyCells() const;
    /** Returns the first clue of the crossword or NULL if there are no clues. */
    ClueCell *firstClue() const;
    /** Returns a list of all clues of the crossword. */
    ClueCellList clues() const;
    /** Gets all horizontal and vertical clues, sorted by clue number (if the
    * clues have a clue number). */
    void clues( ClueCellList *horizontalClues, ClueCellList *verticalClues ) const;
    /** Returns a list of all letters of the crossword. */
    LetterCellList letters() const;
    /** Returns a list of all empty letters of the crossword. */
    LetterCellList emptyLetters() const;
    /** Returns a list of all letters that form the solution word of the crossword. */
    SolutionLetterCellList solutionWordLetters() const { return m_solutionLetters; };
    /** Returns a list of all images of the crossword. */
    ImageCellList images() const;

    /** Resizes the crossword grid. The grid is then filled with empty cells.
    * @param width The new width.
    * @param height The new height. */
    void resizeGrid( uint width, uint height );
    /** Removes all cells, replacing them with empty cells.
    * @note To really delete all cells, you can resize the crossword to 0x0.
    * @see resizeGrid() */
    void removeAllCells();
    /** Returns the size of one crossword cell. */
    QSizeF cellSize() const { return m_cellSize; };

    /** Gets the crossword cell at specified coordinates.
    * @param coord The coordinates of the cell to get. Must be bigger than or
    * equal to (0, 0) and smaller than (@ref width(), @ref height()). You can
    * check if coordinates are inside the crossword grid with @ref inside().
    * @see Coord
    * @see inside */
    KrossWordCell *at( Coord coord ) const {
      if ( !inside(coord) ) {
	kDebug() << coord << "is outside of the grid! Returning NULL.";
	return NULL;
      }
      KrossWordCell *cell = m_krossWordGrid->at( coord );
      Q_ASSERT( !cell || cell->cellType() == KrossWordCell::ImageCellType || cell->coord() == coord );
      return cell;
    };
    /** Gets the width of the crossword grid. */
    uint width() const { return m_krossWordGrid->width(); };
    /** Gets the height of the crossword grid. */
    uint height() const { return m_krossWordGrid->height(); };
    /** Returns true, if the given coordinates are inside the crossword grid. */
    bool inside( Coord coord ) const { return m_krossWordGrid->inside( coord ); };

    KrossWordCell *const&operator[]( const Coord &coord ) const {
	return m_krossWordGrid->operator[](coord); };
    KrossWordCell *&operator[]( const Coord &coord ) {
	return m_krossWordGrid->operator[](coord); };

    static QString errorMessageFromErrorType( ErrorType correctness );
    static ClueCell::AnswerOffset answerOffsetFromString( const QString &s );
    static QString answerOffsetToString( ClueCell::AnswerOffset answerOffset );

    void resizeScene();

    /** Checks if the crossword is empty, ie. contains no clues. */
    bool isEmpty() const;

    QColor emptyCellColorForPrinting() const { return m_emptyCellColorForPrinting; };
    void setEmptyCellColorForPrinting( const QColor &color ) { m_emptyCellColorForPrinting = color; };

    /** Returns the minimal size of the crossword to include all current cells,
    * ie. the "bounding rect" of all non-empty cells. */
    QSize minimalSize() const;

protected:
    /** Returns a non-const list of all solution letter cells. */
    SolutionLetterCellList &solutionWordLettersNonConst() { return m_solutionLetters; };

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
    void replaceCell( KrossWordCell *cell, KrossWordCell *newCell );
    /** Removes the cell at the given coordinates and inserts the new @p cell.
    * @param coord The coordinates of the cell to replace.
    * @param cell The cell to insert at the given coordinates after removing
    * the old one. This is needed to assure consistency. Should not be NULL.
    * @see removeCell */
    void replaceCell( const KGrid2D::Coord &coord, KrossWordCell *newCell );
    /** Removes the cell @p cell and inserts an empty cell instead.
    * @param cell The cell to remove.
    * @see replaceCell*/
    void removeCell( KrossWordCell *cell );
    /** Removes the cell at the given coordinates and inserts an empty cell instead.
    * @param coord The coordinates of the cell to remove.
    * @see replaceCell */
    void removeCell( const KGrid2D::Coord &coord );

    virtual QRectF boundingRect() const;
    virtual void paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0 );

signals:
    /** This signal is emitted when new clues are added. When reading crosswords
    * it will only be emitted once with a list of all new clues. A call to @ref insertClue
    * emits this signal with only the inserted clue in the list. */
    void cluesAdded( ClueCellList clues );
    /** This signal is emitted when existing clues will get removed. When clearing
    * crosswords it will only be emitted once with a list of all clues to be removed.
    * A call to @ref removeClue emits this signal with only the removed clue in the
    * list. */
    void cluesAboutToBeRemoved( ClueCellList clues );

private:
    void replaceCell( const KGrid2D::Coord& coord, KrossWordCell *newCell,
		      bool deleteOldCell );

    void init( uint width = 0, uint height = 0 );
    void fillWithEmptyCells();
    bool canTakeClueLetterCell( const Coord &coord, Qt::Orientation orientation,
			   ClueCell *excludedClue = NULL );
    bool isCellEmptyIfClueIsExcluded( const Coord &coord, ClueCell *excludedClue ) const;
    bool isCellEmptyIfSpannedCellIsExcluded( const Coord &coord, SpannedCell* excludedSpannedCell ) const;

    KGrid2D::Generic<KrossWordCell*> *m_krossWordGrid;
    QSizeF m_cellSize;
    SolutionLetterCellList m_solutionLetters;
    QColor m_emptyCellColorForPrinting;

    QString m_title;
    QString m_authors;
    QString m_copyright;
    QString m_notes;
};

#endif // KROSSWORD_H
