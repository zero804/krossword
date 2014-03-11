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

#ifndef KROSSWORDTHEME_H
#define KROSSWORDTHEME_H

#include <QString>
#include <QColor>
#include <QRect>

#include <KGameTheme>

#if QT_VERSION >= 0x040600 // HACK TODO FIXME 0x040600
#include <QMargins>
#else
// Clone of Qt 4.6's QMargins class (without operator!=, operator==) for
// compilation with Qt < 4.6.
class QMargins {
  public:
    QMargins() { m_left = m_top = m_right = m_bottom = 0; };
    QMargins( int left, int top, int right, int bottom ) {
      m_left = left; m_top = top; m_right = right; m_bottom = bottom; };
    bool isNull () const { return m_left == 0 && m_top == 0 && m_right == 0 && m_bottom == 0; };
    int left () const { return m_left; };
    int top () const { return m_top; };
    int right () const { return m_right; };
    int bottom () const { return m_bottom; };
    void setLeft ( int left ) { m_left = left; };
    void setTop ( int top ) { m_top = top; };
    void setRight ( int right ) { m_right = right; };
    void setBottom ( int bottom ) { m_bottom = bottom; };
  private:
    int m_left, m_top, m_right, m_bottom;
};
#endif

class KrosswordTheme : public KGameTheme {
  public:
    enum ItemPosition {
      TopLeft, TopRight, BottomLeft, BottomRight
    };
    
    KrosswordTheme();
    
    virtual bool load( const QString& file );

    /** Margins of letter cell contents. */
    QMargins marginsLetterCell() const { return m_marginsLetterCell; };

    QMargins marginsLetterCell( qreal levelOfDetail ) const {
	QMargins ret = m_marginsLetterCell;
	ret.setLeft( ret.left() * levelOfDetail );
	ret.setTop( ret.top() * levelOfDetail );
	ret.setRight( ret.right() * levelOfDetail );
	ret.setBottom( ret.bottom() * levelOfDetail );
	return ret; };
    
    /** Margins of clue cell contents. */
    QMargins marginsClueCell() const { return m_marginsClueCell; };
	
    QMargins marginsClueCell( qreal levelOfDetail ) const {
	QMargins ret = m_marginsClueCell;
	ret.setLeft( ret.left() * levelOfDetail );
	ret.setTop( ret.top() * levelOfDetail );
	ret.setRight( ret.right() * levelOfDetail );
	ret.setBottom( ret.bottom() * levelOfDetail );
	return ret; };

    /** Wheather or not the background is dark (affects shadows). */
    bool hasDarkBackground() const { return m_hasDarkBackground; };

    /** The glow color of highlighted cells. */
    QColor glowColor() const { return m_glowColor; };
    
    /** The glow color of focused cells. */
    QColor glowFocusColor() const { return m_glowFocusColor; };

    /** The color for selected cells in edit mode. */
    QColor selectionColor() const { return m_selectionColor; };

    /** The color for empty cells in edit mode. */
    QColor emptyCellColor() const { return m_emptyCellColor; };

    /** The position of number clues inside letter cells for number puzzles. */
    ItemPosition numberPuzzleCluePos() const { return m_numberPuzzleCluePos; };

    /** The position of clue numbers for american crosswords. */
    ItemPosition clueNumberPos() const { return m_clueNumberPos; };

    /** The position of the index for solution letters. */
    ItemPosition solutionLetterIndexPos() const { return m_solutionLetterIndexPos; };

    /** Returns the given @p itemRect aligned at @p position inside @p bounds. */
    static QRect rectAtPos( const QRect &bounds, const QRect &itemRect,
			    ItemPosition position );
    /** Trims the given @p source rect with the given @p margins. */
    static QRect trimmedRect( const QRect &source, const QMargins &margins );

    static KrosswordTheme *defaultValues();
  
  private:
    ItemPosition positionFromString( const QString &s, ItemPosition defaultPos ) const;
    
    QMargins m_marginsLetterCell, m_marginsClueCell;
    QColor m_glowColor, m_glowFocusColor, m_selectionColor, m_emptyCellColor;
    bool m_hasDarkBackground;
    ItemPosition m_numberPuzzleCluePos, m_clueNumberPos, m_solutionLetterIndexPos;
};

#endif // KROSSWORDTHEME_H
