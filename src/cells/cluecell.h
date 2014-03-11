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

#ifndef KROSSWORDCLUECELL_H
#define KROSSWORDCLUECELL_H

#include "global.h"
#include "krosswordcell.h"
#include "lettercell.h"
#include <QTextLayout>

namespace Crossword {

class KrossWord;
/** A crossword cell that contains the clue text in it. */
class ClueCell : public KrossWordCell {
    Q_OBJECT
    friend class DoubleClueCell;
    friend class LetterCell;
    friend class SolutionLetterCell; // SolutionLetterCell::toLetter() needs to call findLetters()
    friend class KrossWord;
    
    Q_PROPERTY( qreal transitionHeightFactor READ transitionHeightFactor
					     WRITE setTransitionHeightFactor )

    public:
	static const char EmptyCorrectCharacter = ' ';

	static QList< AnswerOffset > allAnswerOffsets() {
	  return QList< AnswerOffset >()
	      << OffsetTopLeft << OffsetTop
	      << OffsetTopRight << OffsetLeft
	      << OnClueCell << OffsetRight
	      << OffsetBottomLeft << OffsetBottom
	      << OffsetBottomRight;
	};

	ClueCell( KrossWord *krossWord, Coord coord,
		  Qt::Orientation orientation, AnswerOffset answerOffset,
		  QString clue, QString answer );

	/** For qgraphicsitem_cast. */
	enum { Type = UserType + 3 };
	virtual int type() const { return Type; };

	virtual QRectF boundingRect() const;
	QRectF boundingRectIncludingAnswerCells() const;

	/** The orientation in which the clue's answer letter cells are arranged. */
	Qt::Orientation orientation() const { return m_orientation; };
	/** Whether or not this is a horizontal clue. @see orientation() */
	bool isHorizontal() const { return m_orientation == Qt::Horizontal; }
	/** Whether or not this is a vertical clue. @see orientation() */
	bool isVertical() const { return m_orientation == Qt::Vertical; };

	static QList<Coord> answerCoordList( const Coord &clueCoord,
					     AnswerOffset answerOffset,
					     Qt::Orientation orientation,
					     int answerLength );

	ErrorType setProperties( Qt::Orientation newOrientation,
				AnswerOffset newAnswerOffset,
				const QString &newCorrectAnswer );
	/** Hides the clue cell, by moving it's coordinates to the coordinates of the
	* first answer answerLengthChangedcell and setting answer offset to @ref OnClueCell. */
	void setHidden();
	bool setUnhidden( AnswerOffset newAnswerOffset );
	/** Tries to find an empty cell sibling to the current position of this clue
	* cell and moves this clue cell to the found empty cell. It also adjusts the
	* answer offset appropriately. If the answer offset wasn't @ref OnClueCell
	* nothing is done and true is returned.
	* @returns true If the clue cell could be made visible.
	*	false If no appropriate empty cell for the clue cell could be found. */
	AnswerOffset tryToMakeVisible( bool simulate = false,
				      QList<Coord> disallowedCoords = QList<Coord>() );

	/** Gets the clue string of the clue cell. */
	QString clue() const { return m_clue; };
	/** Sets the clue string of the clue cell. */
	void setClue( const QString &clue );
	QString clueWithoutHyphens() const;
	QString clueWithNumber( QString format = "%1. %2" ) const;
	/** Gets the correct answer. */
	QString correctAnswer() const { return m_correctAnswer; };
	/** Gets the current answer by concatenating the current letter
	* (@ref LetterCell::currentLetter()) of the letter cells of this clue
	* (@ref letters()).
	* @param pad This character is inserted for each empty letter
	* (@ref LetterCell::isEmpty()). */
	QString currentAnswer( const QChar &pad = '-' ) const;
	void setCurrentAnswer( const QString &answer,
		    Confidence confidence = Confident );
	/** Returns true if no letter cell of this clue is empty.
	* @see LetterCell::isEmpty() */
	bool isAnswerComplete() const;
	/** Returns true if all letter cells of this clue are correct.
	* @see LetterCell::isCorrect() */
	bool isAnswerCorrect() const;

	inline bool isInDoubleClueCell() const {
	    return qgraphicsitem_cast< DoubleClueCell* >( parentItem() ); };

	/** Solves all letter cells of this clue cell by setting it's current
	* answer to the correct answer. */
	void solve();
	/** Clears all letter cell of this clue cell by clearing it's current
	* answer. */
	void clear( ClearMode clearMode =
			ClearCurrentLetter );
	bool isEmpty() const;
	bool isCorrectAnswerEmpty() const;

	virtual void setHighlight( bool hightlight = true );

	/** Gets the offset of the first answer letter cell. */
	AnswerOffset answerOffset() const { return m_answerOffset; };
// 	KrossWord::ErrorType setAnswerOffset( AnswerOffset answerOffset );
	/** The offset of the first letter of the answer in the crossword grid.
	* Can be (0 or +-1, 0 or +-1). It is (0, 0) if the clue is hidden. The
	* clue cell is hidden, if @ref answerOffset() is
	* @ref ClueCell::ClueHidden. */
	inline Offset firstLetterOffset( AnswerOffset answerOffset = OffsetInvalid ) const {
	    return answerOffset == OffsetInvalid
		? answerOffsetToOffset(m_answerOffset)
		: answerOffsetToOffset(answerOffset); };
	Coord firstLetterCoords() const;

	static Offset answerOffsetToOffset( AnswerOffset answerOffset );
	static AnswerOffset offsetToAnswerOffset( Offset offset );

	static Coord firstLetterCoords( Coord clueCoords,
						   AnswerOffset answerOffset );
	bool isHidden() const { return m_answerOffset == OnClueCell; };

	/** Returns a list of all letter cells that represent the answer to this clue. */
	LetterCellList letters() const { return m_letters; };
	/** Gets the first letter cell of the answer to this clue cell.
	* @see lastLetter()
	* @see letterAt() */
	LetterCell *firstLetter() const {
	    return m_letters.isEmpty() ? NULL : m_letters.first(); };
	/** Gets the last letter cell of the answer to this clue cell.
	* @see firstLetter()
	* @see letterAt() */
	LetterCell *lastLetter() const {
	    return m_letters.isEmpty() ? NULL : m_letters.last(); };
	/** The the letter cell at @p letterIndex from the answer letter cells.
	* @see firstLetter()
	* @see lastLetter()
	* @see letters() */
	LetterCell *letterAt( int letterIndex ) const {
	    return m_letters[ letterIndex ]; };
	int posOfLetter( LetterCell *letterCell ) const {
	    return m_letters.indexOf( letterCell ); };
	bool answerContainsLetter( LetterCell *letterCell ) const {
	    return m_letters.contains( letterCell ); /*posOfLetter(letterCell) != -1;*/ };

	/** Returns the maximal number of letters that can actually be added /
	* removed, but maximally @p count. */
	int canAddLetters( int count ) const;
	int addLetters( int count );

	int minAnswerLength() const;
	int maxAnswerLength() const;
	int setAnswerLength( int newLength );
	int answerLength() const { return m_correctAnswer.length(); };

	int clueNumber() const { return m_clueNumber; };
	void setClueNumber( int clueNumber );

	/** Sets the correct answer to this clue cell. */
	void setCorrectAnswer( const QString &correctAnswer );

	qreal transitionHeightFactor() const { return m_transitionHeightFactor; };
	void setTransitionHeightFactor( qreal transitionHeightFactor ) {
	    prepareGeometryChange();
	    m_transitionHeightFactor = transitionHeightFactor;
#if QT_VERSION >= 0x040600
	    clearCache( Animator::Instant ); };
#else
	    clearCache(); };
#endif

    signals:
	/** Emitted, when the current answer changes. */
	void currentAnswerChanged( ClueCell *clueCell, const QString &currentAnswer );
	void correctAnswerChanged( ClueCell *clueCell, const QString &correctAnswer );
	void answerLengthChanged( ClueCell *clueCell, int newLength );
	void clueTextChanged( ClueCell *clueCell, const QString &clue );
	void orientationChanged( ClueCell *clueCell, Qt::Orientation orientation );
	void answerOffsetChanged( ClueCell *clueCell, AnswerOffset answerOffset );
	void lastLetterChanged( LetterCell *lastLetter );

    public slots:
	/** A letter of the answer has been changed. */
	void answerLetterChanged( LetterCell *letter, const QChar &newLetter );
	void letterAdded( LetterCell *letter );
	void letterRemoved( LetterCell *letter );

    protected:
	virtual void focusInEvent( QFocusEvent* event );
	virtual void focusOutEvent( QFocusEvent* event );
	virtual void mousePressEvent( QGraphicsSceneMouseEvent* event );
	virtual QVariant itemChange( GraphicsItemChange change, const QVariant& value );

	virtual void drawBackground( QPainter* p, const QStyleOptionGraphicsItem* option );
	virtual void drawForeground( QPainter *p, const QStyleOptionGraphicsItem* option );
	virtual void drawClueNumber( QPainter *p, const QStyleOptionGraphicsItem *option );

	virtual void drawBackgroundForPrinting( QPainter* , const QStyleOptionGraphicsItem* );
	virtual void drawForegroundForPrinting( QPainter* , const QStyleOptionGraphicsItem* );

	virtual void wrapClueText();

	void createLayout( const QRect &rect );
	void setProperties( Qt::Orientation newOrientation,
			    AnswerOffset newAnswerOffset );
	void setOrientation( Qt::Orientation newOrientation );
	void setAnswerOffset( AnswerOffset newAnswerOffset );

	void beginAddLetters();
	void endAddLetters();
	void endAddLetters( Qt::Orientation newOrientation,
			    AnswerOffset newAnswerOffset );
	void findLetters( LetterCell *newLetter = NULL );

    private:
	void findLetters( Qt::Orientation newOrientation,
			  AnswerOffset newAnswerOffset,
			  LetterCell *newLetter = NULL );

	Qt::Orientation m_orientation;
	AnswerOffset m_answerOffset;
	QString m_clue, m_wrappedClue, m_correctAnswer;
	int m_clueNumber;

	QTextLayout m_textLayout;
	QRect m_textLayoutRect;

	LetterCellList m_letters;
	bool m_dontProcessLetterAdded;

	qreal m_transitionHeightFactor;
};


class DoubleClueCell : public KrossWordCell {
    Q_OBJECT
    friend class ClueCell;

    public:
	DoubleClueCell( KrossWord* krossWord, const Coord& coord,
			ClueCell *clue1, ClueCell *clue2 );

	/** For qgraphicsitem_cast. */
	enum { Type = UserType + 4 };
	virtual int type() const { return Type; };

	ClueCell *clue1() const { return m_clue1; };
	ClueCell *clue2() const { return m_clue2; };
	ClueCellList clues() const {
	  return ClueCellList() << m_clue1 << m_clue2; };

	ClueCell *clue( AnswerOffset answerOffset,
			Qt::Orientation orientation ) const;

	void removeClueCell( ClueCell *clueCell );
	ClueCell *takeClueCell( ClueCell *clueCell );
	bool hasClue( ClueCell *clue ) const {
	    return m_clue1 == clue || m_clue2 == clue; };

    private:
// 	bool setPositionFromCoordinates( bool animate = true );

	ClueCell *m_clue1;
	ClueCell *m_clue2;
};

inline QDebug &operator <<(QDebug debug, AnswerOffset answerOffset) {
  switch ( answerOffset ) {
    case OnClueCell:
      return debug << "OnClueCell";
    case OffsetTopLeft:
      return debug << "OffsetTopLeft";
    case OffsetTop:
      return debug << "OffsetTop";
    case OffsetTopRight:
      return debug << "OffsetTopRight";
    case OffsetLeft:
      return debug << "OffsetLeft";
    case OffsetRight:
      return debug << "OffsetRight";
    case OffsetBottomLeft:
      return debug << "OffsetBottomLeft";
    case OffsetBottom:
      return debug << "OffsetBottom";
    case OffsetBottomRight:
      return debug << "OffsetBottomRight";
    default:
      return debug << "Unknown answer offset" << static_cast<int>(answerOffset);
  }
};

inline QDebug &operator <<(QDebug debug, ClueCell *cell) {
  if ( !cell )
    return debug << "NULL (ClueCell)";

  return debug << (KrossWordCell*)cell
	       << "| Answer Offset: " << cell->answerOffset()
	       << "| Orientation: " << cell->orientation()
	       << "| Clue: " << cell->clue()
	       << "| Correct: " << cell->correctAnswer()
	       << "| Current: " << cell->currentAnswer();

//   try {
//   if ( cell ) {
//     debug << "  Correct: " << cell->correctAnswer();
//     debug << "| Current: " << cell->currentAnswer() << endl;
//     debug << "  Answer Offset: "
// 	  << ClueCell::answerOffsetToOffset( cell->answerOffset() );
//     debug << "| Orientation: " << cell->orientation() << endl;
//     debug << "  Clue: " << cell->clue();
//   }
//   } catch() {
//     debug << "Error getting current Answer. ";
//
//     debug << "Correct: ";
//     debug << cell->correctAnswer();
//
//     debug << " Answer Offset: ";
//     debug << ClueCell::answerOffsetToOffset( cell->answerOffset() );
//
//     debug << " Orientation: ";
//     debug << cell->orientation();
//
//     debug << " Clue: ";
//     debug << cell->clue();
//   }

//   return debug.space();
};

// Sorting functions
bool lessThanClueNumber( const ClueCell *clueCell1, const ClueCell *clueCell2 );
bool greaterThanClueNumber( const ClueCell *clueCell1, const ClueCell *clueCell2 );

bool lessThanAnswerLength( const ClueCell *clueCell1, const ClueCell *clueCell2 );
bool greaterThanAnswerLength( const ClueCell *clueCell1, const ClueCell *clueCell2 );

/** Horizontal clues first. */
bool lessThanOrientation( const ClueCell *clueCell1, const ClueCell *clueCell2 );
/** Vertical clues first. */
bool greaterThanOrientation( const ClueCell *clueCell1, const ClueCell *clueCell2 );

}; // namespace Crossword

#endif // KROSSWORDCLUECELL_H
