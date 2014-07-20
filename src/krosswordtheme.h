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
#include <KgTheme>
#include <QMargins>

enum ItemPosition {TopLeft, TopRight, BottomLeft, BottomRight};

class KrosswordTheme : public KgTheme
{
    Q_OBJECT
public:
    KrosswordTheme();

    //Q_INVOKABLE KrosswordTheme(const QByteArray &identifier, QObject *parent=0);

    virtual bool readFromDesktopFile(const QString& file);

    static KrosswordTheme *defaultValues();

    /** Margins of letter cell contents. */
    QMargins marginsLetterCell() const;
    QMargins marginsLetterCell(qreal levelOfDetail) const;

    /** Margins of clue cell contents. */
    QMargins marginsClueCell() const;
    QMargins marginsClueCell(qreal levelOfDetail) const;

    /** Wheather or not the background is dark (affects shadows). */
    bool hasDarkBackground() const;

    /** The glow color of highlighted cells. */
    QColor glowColor() const;

    /** The glow color of focused cells. */
    QColor glowFocusColor() const;

    /** The color for selected cells in edit mode. */
    QColor selectionColor() const;

    /** The color for empty cells in edit mode. */
    QColor emptyCellColor() const;

    /** The position of number clues inside letter cells for number puzzles. */
    //ItemPosition numberPuzzleCluePos() const;

    /** The position of clue numbers for american crosswords. */
    //ItemPosition clueNumberPos() const;

    /** The position of the index for solution letters. */
    //ItemPosition solutionLetterIndexPos() const;

    /** Returns the given @p itemRect aligned at @p position inside @p bounds. */
    static QRect rectAtPos(const QRect &bounds, const QRect &itemRect, ItemPosition position);

    /** Trims the given @p source rect with the given @p margins. */
    static QRect trimmedRect(const QRect &source, const QMargins &margins);

private:
    //ItemPosition positionFromString(const QString &s, ItemPosition defaultPos) const;

    QMargins m_marginsLetterCell, m_marginsClueCell;
    QColor m_glowColor, m_glowFocusColor, m_selectionColor, m_emptyCellColor;
    bool m_hasDarkBackground;
    //ItemPosition m_numberPuzzleCluePos, m_clueNumberPos, m_solutionLetterIndexPos;
};

#endif // KROSSWORDTHEME_H
