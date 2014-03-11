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

#ifndef KROSSWORDLETTERCELL_H
#define KROSSWORDLETTERCELL_H

#include "krosswordcell.h"

class LetterCell : public KrossWordCell {
    Q_OBJECT
    friend class KrossWord;
    friend class ClueCell;
    friend class SolutionLetterCell;

    public:
	/** Describes the confidence of the correctness of a letter. */
	enum Confidence {
	    Solved, /**< The letter is definetly correct, because it was solved. */
	    Confident, /**< Confident that the letter is correct. */
	    Unsure, /**< Unsure if the letter is correct. */
	    Unknown /**< Confidence is unknown, eg. when loaded from a file format
				    not supporting confidence. */
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

	LetterCell( KrossWord *krossWord, const Coord &coord,
		    ClueCell *clueHorizontal, ClueCell *clueVertical/*,
		    QGraphicsScene *scene*/ );
	LetterCell( KrossWord *krossWord, const Coord &coord, ClueCell *clue/*,
		    QGraphicsScene *scene*/ );

	static const qreal BAR_WIDTH = 0.08; // in percent of the total cell width/height
	
	virtual bool isLetterCell() const { return true; };

	/** For qgraphicsitem_cast. */
	enum { Type = UserType + 5 };
	virtual int type() const { return Type; };

	ClueCell *clueHorizontal() const { return m_clueHorizontal; };
	ClueCell *clueVertical() const { return m_clueVertical; };
	ClueCell *clue( Qt::Orientation orientation ) const {
	    return orientation == Qt::Horizontal ? m_clueHorizontal : m_clueVertical;
	};
	ClueCellList clues() const {
	  ClueCellList clueList;
	  if ( m_clueHorizontal )
	    clueList << m_clueHorizontal;
	  if ( m_clueVertical )
	    clueList << m_clueVertical;
	  return clueList;
	};
	bool hasClueInDirection( Qt::Orientation orientation ) const {
	    return clue(orientation);
	};
	bool isCrossed() const {
	    return m_clueHorizontal && m_clueVertical;
	};
	bool isAttachedToClue( ClueCell *clue ) {
	    return m_clueHorizontal == clue || m_clueVertical == clue;
	};
	bool isAttachedToClueExclusivly( ClueCell *clue ) {
	    return isAttachedToClue( clue ) && !isCrossed();
	};

	QString allowedLetters() const { return ALLOWED_CHARACTERS; };
	static bool isLetterAllowed( const QChar &letter ) {
	    return ALLOWED_CHARACTERS.contains( letter.toUpper() ); };

	/** Returns the correct letter of this letter cell. */
	QChar correctLetter() const;
	/** Returns the current letter of this letter cell. */
	QChar currentLetter() const { return m_currentLetter; };
	void setCurrentLetter( const QChar &currentLetter, Confidence confidence = Confident );
	void setCorrectLetter( const QChar &correctLetter );
	bool isCorrect() const { return m_currentLetter == correctLetter(); };
	bool isEmpty() const { return m_currentLetter == ' '; };
	Confidence confidence() const { return m_confidence; };
	void setConfidence( Confidence confidence );

	bool needsEndBar( Qt::Orientation orientation ) const;
	
	LetterCell *letterCellOnRight() const;
	LetterCell *letterCellOnLeft() const;
	LetterCell *letterCellOnTop() const;
	LetterCell *letterCellOnBottom() const;

	static Confidence stringToConfidence( const QString &string );
	static QString confidenceToString( Confidence confidence );

    public slots:
	/** Sets the current letter to @p newLetter. */
	void setCurrentLetterSlot( LetterCell *letter, const QChar &newLetter );

    protected:
	LetterCell( KrossWord *krossWord, const Coord &coord,
		    ClueCell *clueHorizontal, ClueCell *clueVertical,
		    /*QGraphicsScene *scene,*/ CellType cellType );
	LetterCell( KrossWord *krossWord, const Coord &coord, ClueCell *clue,
		    /*QGraphicsScene *scene,*/ CellType cellType );

	/** Gets the clue of this letter cell that is orthogonal to the given
	* @p clue, or NULL if this letter cell has no other clue.
	* @note It is assumed that @p clue is one @ref horizontalClue()
	* and @ref verticalClue(). */
	ClueCell *getOrthogonalClueTo( ClueCell *clue ) const;

	void setClueHorizontal(ClueCell *clue) { m_clueHorizontal = clue; };
	void setClueVertical(ClueCell *clue) { m_clueVertical = clue; };
	void setClue(ClueCell *clue);
	void detachClues();
	bool detachClue( ClueCell *cell );

	virtual void drawClueArrows( QPainter *p, const QStyleOptionGraphicsItem *option );
	virtual void drawEndBarIfNeeded( QPainter *p, const QStyleOptionGraphicsItem *option );

	virtual void drawBackgroundForPrinting( QPainter *p, const QStyleOptionGraphicsItem *option );
	virtual void drawForegroundForPrinting( QPainter *p, const QStyleOptionGraphicsItem *option );

    private:
	void init( ClueCell *clueHorizontal, ClueCell *clueVertical );
	void init( ClueCell *clue );
	LetterCell *letterCellAtOffset( Offset offset ) const;

	ClueCell *m_clueHorizontal, *m_clueVertical;
	QChar m_currentLetter;
	Confidence m_confidence;
};

class SolutionLetterCell : public LetterCell {
    friend class KrossWord;
    friend class LetterCell;
    Q_OBJECT

    public:
	SolutionLetterCell( KrossWord *krossWord, const Coord &coord,
		    ClueCell *clueHorizontal, ClueCell *clueVertical,
		    int solutionWordIndex/*, QGraphicsScene *scene*/ );
	SolutionLetterCell( KrossWord *krossWord, const Coord &coord,
		    ClueCell *clue, int solutionWordIndex/*, QGraphicsScene *scene*/ );

	/** For qgraphicsitem_cast. */
	enum { Type = UserType + 6 };
	virtual int type() const { return Type; };

	int solutionWordIndex() const { return m_solutionWordIndex; };

	static SolutionLetterCell *fromLetterCell( LetterCell *&letter,
						   int solutionWordIndex, bool deleteLetter = true );

    protected:
	virtual void drawForeground( QPainter* p, const QStyleOptionGraphicsItem* option );

    private:
	// Use fromLetterCell()
	SolutionLetterCell( LetterCell *letter, int solutionWordIndex );
	void init( int solutionWordIndex );

	int m_solutionWordIndex;
};

#endif // KROSSWORDLETTERCELL_H
