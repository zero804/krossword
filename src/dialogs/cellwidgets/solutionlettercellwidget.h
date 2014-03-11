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

#ifndef SOLUTIONLETTERCELLWIDGET_HEADER
#define SOLUTIONLETTERCELLWIDGET_HEADER

#include <QtGui/QWidget>
#include "ui_solution_letter_properties.h"
#include "global.h"

using namespace Crossword;


class SolutionLetterCellWidget : public QWidget {
  Q_OBJECT
  public:
    explicit SolutionLetterCellWidget( SolutionLetterCell *solutionLetterCell, QWidget* parent = 0 );

    void setSolutionLetterCell( SolutionLetterCell *solutionLetterCell );
    SolutionLetterCell *solutionLetterCell() const { return m_solutionLetterCell; };

  signals:
    void setSolutionWordIndexRequest( SolutionLetterCell *solutionLetterCell,
				      int solutionWordIndex );
    void convertToLetterCellRequest( SolutionLetterCell *solutionLetterCell );
    
  protected slots:
    void applyClicked();
    void convertToLetterCellClicked();
    void solutionLetterPropertiesPositionChanged( int position );
    
  private:
    void updateSolutionWord();
    
    Ui::solution_letter_properties ui_solution_letter_properties;
    SolutionLetterCell *m_solutionLetterCell;
};

#endif // Multiple inclusion guard
