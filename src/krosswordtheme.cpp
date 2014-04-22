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

#include "krosswordtheme.h"

#include <QDebug>

KrosswordTheme::KrosswordTheme() : KgTheme("krosswordpuzzle")
{
}

bool KrosswordTheme::readFromDesktopFile(const QString& file)
{
    qDebug() << "Read theme:" << file;

    if (!KgTheme::readFromDesktopFile(file) && !KgTheme::readFromDesktopFile("themes/" + file.toLower() + ".desktop")) {
        return false;
    }

    // Load theme .desktop file custom settings
    QStringList letterCellMargins = (customData("LetterCellMargins", "2,2,2,2")).split(',', QString::SkipEmptyParts);
    QStringList clueCellMargins = (customData("ClueCellMargins", "2,2,2,2")).split(',', QString::SkipEmptyParts);
    if (letterCellMargins.count() != 4)
        letterCellMargins = QStringList() << "2" << "2" << "2" << "2";
    if (clueCellMargins.count() != 4)
        clueCellMargins = QStringList() << "2" << "2" << "2" << "2";
    m_marginsLetterCell = QMargins(letterCellMargins.at(0).toInt(), letterCellMargins.at(1).toInt(), letterCellMargins.at(2).toInt(), letterCellMargins.at(3).toInt());
    m_marginsClueCell = QMargins(clueCellMargins.at(0).toInt(), clueCellMargins.at(1).toInt(), clueCellMargins.at(2).toInt(), clueCellMargins.at(3).toInt());


    m_hasDarkBackground = (customData("HasDarkBackground", "false") == "false") ? false : true;
    m_glowColor = QColor(customData("GlowColor", "64, 64, 255"));
    m_glowFocusColor = QColor(customData("FocusGlowColor", "255, 64, 64"));
    m_selectionColor = QColor(customData("SelectionColor", "255, 100, 100, 128"));
    m_emptyCellColor = QColor(customData("EmptyCellColor", "100, 100, 100, 128"));

    // (original) TODO only use "free" positions as default values
    m_clueNumberPos = positionFromString(customData("ClueNumberPos", ""), BottomRight);
    m_numberPuzzleCluePos = positionFromString(customData("NumberPuzzleCluePos", ""), TopRight);
    m_solutionLetterIndexPos = positionFromString(customData("SolutionLetterIndexPos", ""), BottomLeft);

    return true;
}

KrosswordTheme* KrosswordTheme::defaultValues()
{
    KrosswordTheme *theme = new KrosswordTheme;

    theme->m_marginsLetterCell = QMargins();
    theme->m_marginsClueCell = QMargins();

    theme->m_hasDarkBackground = false;
    theme->m_glowColor = QColor(64, 64, 255);
    theme->m_glowFocusColor = QColor(255, 64, 64);
    theme->m_selectionColor = QColor(255, 100, 100, 128);
    theme->m_emptyCellColor = QColor(100, 100, 100, 128);

    theme->m_clueNumberPos = BottomRight;
    theme->m_numberPuzzleCluePos = TopRight;
    theme->m_solutionLetterIndexPos = BottomLeft;

    return theme;
}

ItemPosition KrosswordTheme::positionFromString(const QString& s, ItemPosition defaultPos) const
{
    if (s.compare("TopLeft", Qt::CaseInsensitive) == 0)
        return TopLeft;
    else if (s.compare("TopRight", Qt::CaseInsensitive) == 0)
        return TopRight;
    else if (s.compare("BottomLeft", Qt::CaseInsensitive) == 0)
        return BottomLeft;
    else if (s.compare("BottomRight", Qt::CaseInsensitive) == 0)
        return BottomRight;
    else
        return defaultPos;
}

QRect KrosswordTheme::rectAtPos(const QRect& bounds, const QRect& itemRect, ItemPosition position)
{
    switch (position) {
    case TopLeft:
        return QRect(bounds.left(), bounds.top(), itemRect.width(), itemRect.height());
    case TopRight:
        return QRect(bounds.right() - itemRect.width(), bounds.top(), itemRect.width(), itemRect.height());
    case BottomLeft:
        return QRect(bounds.left(), bounds.bottom() - itemRect.height(), itemRect.width(), itemRect.height());
    default:
    case BottomRight:
        return QRect(bounds.right() - itemRect.width(), bounds.bottom() - itemRect.height(), itemRect.width(), itemRect.height());
    }

    return QRect(); // To make the buildService happy for openSuse 11.1 TODO: Test.
}

QRect KrosswordTheme::trimmedRect(const QRect& source, const QMargins& margins)
{
    return source.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
}

QMargins KrosswordTheme::marginsLetterCell() const {
    return m_marginsLetterCell;
}

QMargins KrosswordTheme::marginsLetterCell(qreal levelOfDetail) const {
    QMargins ret = m_marginsLetterCell;
    ret.setLeft(ret.left() * levelOfDetail);
    ret.setTop(ret.top() * levelOfDetail);
    ret.setRight(ret.right() * levelOfDetail);
    ret.setBottom(ret.bottom() * levelOfDetail);
    return ret;
}

QMargins KrosswordTheme::marginsClueCell() const {
    return m_marginsClueCell;
}

QMargins KrosswordTheme::marginsClueCell(qreal levelOfDetail) const {
    QMargins ret = m_marginsClueCell;
    ret.setLeft(ret.left() * levelOfDetail);
    ret.setTop(ret.top() * levelOfDetail);
    ret.setRight(ret.right() * levelOfDetail);
    ret.setBottom(ret.bottom() * levelOfDetail);
    return ret;
}

bool KrosswordTheme::hasDarkBackground() const {
    return m_hasDarkBackground;
}

QColor KrosswordTheme::glowColor() const {
    return m_glowColor;
}

QColor KrosswordTheme::glowFocusColor() const {
    return m_glowFocusColor;
}

QColor KrosswordTheme::selectionColor() const {
    return m_selectionColor;
}

QColor KrosswordTheme::emptyCellColor() const {
    return m_emptyCellColor;
}

ItemPosition KrosswordTheme::numberPuzzleCluePos() const {
    return m_numberPuzzleCluePos;
}

ItemPosition KrosswordTheme::clueNumberPos() const {
    return m_clueNumberPos;
}

ItemPosition KrosswordTheme::solutionLetterIndexPos() const {
    return m_solutionLetterIndexPos;
}
