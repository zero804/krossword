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

#include <KStandardDirs>
#include <KDebug>
#include <kconfiggroup.h>
#include <kconfig.h>
#include "krosswordrenderer.h"
#include <QFile>


KrosswordTheme::KrosswordTheme() : KgTheme("krosswordpuzzle") {
}


bool KrosswordTheme::readFromDesktopFile( const QString& file ) {
  if ( !KgTheme::readFromDesktopFile(file) )
    return false;

  // Load theme .desktop file
  KConfig themeConfig( file, KConfig::SimpleConfig );
  KConfigGroup configGroup( &themeConfig, "KGameTheme" );

  QList<int> letterCellMargins = configGroup.readEntry( "LetterCellMargins", QList<int>() );
  QList<int> clueCellMargins = configGroup.readEntry( "ClueCellMargins", QList<int>() );
  if ( letterCellMargins.count() != 4 )
    letterCellMargins = QList<int>() << 2 << 2 << 2 << 2;
  if ( clueCellMargins.count() != 4 )
    clueCellMargins = QList<int>() << 2 << 2 << 2 << 2;

  m_marginsLetterCell = QMargins( letterCellMargins[0], letterCellMargins[1], letterCellMargins[2], letterCellMargins[3] );
  m_marginsClueCell = QMargins( clueCellMargins[0], clueCellMargins[1], clueCellMargins[2], clueCellMargins[3] );

  m_hasDarkBackground = configGroup.readEntry( "HasDarkBackground", false );
  m_glowColor = configGroup.readEntry( "GlowColor", QColor(64, 64, 255) );
  m_glowFocusColor = configGroup.readEntry( "FocusGlowColor", QColor(255, 64, 64) );
  m_selectionColor = configGroup.readEntry( "SelectionColor", QColor(255, 100, 100, 128) );
  m_emptyCellColor = configGroup.readEntry( "EmptyCellColor", QColor(100, 100, 100, 128) );

  // TODO only use "free" positions as default values
  m_clueNumberPos = positionFromString( configGroup.readEntry("ClueNumberPos", ""), BottomRight );
  m_numberPuzzleCluePos = positionFromString( configGroup.readEntry("NumberPuzzleCluePos", ""), TopRight );
  m_solutionLetterIndexPos = positionFromString( configGroup.readEntry("SolutionLetterIndexPos", ""), BottomLeft );

  if ( !KrosswordRenderer::self()->setTheme(graphicsPath()) ) {
    kDebug() << "Couldn't load theme SVG file" << graphicsPath();
    return false;
  }

  return true;
}

KrosswordTheme* KrosswordTheme::defaultValues() {
  KrosswordTheme *theme = new KrosswordTheme;

  theme->m_marginsLetterCell = QMargins();
  theme->m_marginsClueCell = QMargins();

  theme->m_hasDarkBackground = false;
  theme->m_glowColor = QColor( 64, 64, 255 );
  theme->m_glowFocusColor = QColor( 255, 64, 64 );
  theme->m_selectionColor = QColor( 255, 100, 100, 128 );
  theme->m_emptyCellColor = QColor( 100, 100, 100, 128 );

  theme->m_clueNumberPos = BottomRight;
  theme->m_numberPuzzleCluePos = TopRight;
  theme->m_solutionLetterIndexPos = BottomLeft;

  return theme;
}

KrosswordTheme::ItemPosition KrosswordTheme::positionFromString( const QString& s, ItemPosition defaultPos ) const {
  if ( s.compare("TopLeft", Qt::CaseInsensitive) == 0 )
    return TopLeft;
  else if ( s.compare("TopRight", Qt::CaseInsensitive) == 0 )
    return TopRight;
  else if ( s.compare("BottomLeft", Qt::CaseInsensitive) == 0 )
    return BottomLeft;
  else if ( s.compare("BottomRight", Qt::CaseInsensitive) == 0 )
    return BottomRight;
  else
    return defaultPos;
}

QRect KrosswordTheme::rectAtPos( const QRect& bounds, const QRect& itemRect, KrosswordTheme::ItemPosition position ) {
  switch ( position ) {
    case TopLeft:
      return QRect( bounds.left(), bounds.top(), itemRect.width(), itemRect.height() );
    case TopRight:
      return QRect( bounds.right() - itemRect.width(), bounds.top(), itemRect.width(), itemRect.height() );
    case BottomLeft:
      return QRect( bounds.left(), bounds.bottom() - itemRect.height(), itemRect.width(), itemRect.height() );
    default:
    case BottomRight:
      return QRect( bounds.right() - itemRect.width(), bounds.bottom() - itemRect.height(), itemRect.width(), itemRect.height() );
  }

  return QRect(); // To make the buildService happy for openSuse 11.1 TODO: Test.
}

QRect KrosswordTheme::trimmedRect( const QRect& source, const QMargins& margins ) {
  return source.adjusted( margins.left(), margins.top(), -margins.right(), -margins.bottom() );
}


