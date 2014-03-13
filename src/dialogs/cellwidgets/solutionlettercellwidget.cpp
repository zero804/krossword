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

#include "solutionlettercellwidget.h"

#include "../../cells/lettercell.h"
#include "../../krossword.h"

SolutionLetterCellWidget::SolutionLetterCellWidget(
    SolutionLetterCell* solutionLetterCell, QWidget* parent)
    : QWidget(parent)
{
    ui_solution_letter_properties.setupUi(this);
    setSolutionLetterCell(solutionLetterCell);

    connect(ui_solution_letter_properties.solutionWordPosition, SIGNAL(valueChanged(int)),
            this, SLOT(solutionLetterPropertiesPositionChanged(int)));
    connect(ui_solution_letter_properties.apply, SIGNAL(clicked()),
            this, SLOT(applyClicked()));
    connect(ui_solution_letter_properties.convertToLetterCell, SIGNAL(clicked()),
            this, SLOT(convertToLetterCellClicked()));
}

void SolutionLetterCellWidget::setSolutionLetterCell(SolutionLetterCell* solutionLetterCell)
{
    Q_ASSERT(solutionLetterCell);

    if (m_solutionLetterCell == solutionLetterCell)
        return;

    m_solutionLetterCell = solutionLetterCell;
    ui_solution_letter_properties.solutionWordPosition->setValue(
        solutionLetterCell->solutionWordIndex() + 1);

    updateSolutionWord();
}

void SolutionLetterCellWidget::updateSolutionWord()
{
    QString solutionWord = m_solutionLetterCell->krossWord()->solutionWord().replace(' ', '-');
    if (solutionWord.isEmpty())
        ui_solution_letter_properties.currentSolutionWord->setText("<empty>");
    else
        ui_solution_letter_properties.currentSolutionWord->setText(solutionWord);
}

void SolutionLetterCellWidget::applyClicked()
{
//   Coord coord = m_solutionLetterCell->coord();

    emit setSolutionWordIndexRequest(m_solutionLetterCell,
                                     ui_solution_letter_properties.solutionWordPosition->value() - 1);
    updateSolutionWord();
}

void SolutionLetterCellWidget::convertToLetterCellClicked()
{
    emit convertToLetterCellRequest(m_solutionLetterCell);
}

void SolutionLetterCellWidget::solutionLetterPropertiesPositionChanged(int position)
{
    bool positionAlreadyUsed = false;
    SolutionLetterCellList solutionLetters = m_solutionLetterCell->krossWord()->solutionWordLetters();
    foreach(SolutionLetterCell * solutionLetter, solutionLetters) {
        if (solutionLetter->solutionWordIndex() == position - 1) {
            if (m_solutionLetterCell != solutionLetter)
                positionAlreadyUsed = true;
            else
                ui_solution_letter_properties.apply->setEnabled(false);   // No change
            break;
        }
    }

    ui_solution_letter_properties.apply->setEnabled(!positionAlreadyUsed);
    if (positionAlreadyUsed) {
        ui_solution_letter_properties.apply->setToolTip(i18n(
                    "The solution letter position %1 is already assigned.\n"
                    "Please change the solution word position of that letter first.",
                    m_solutionLetterCell->solutionWordIndex()));
    } else
        ui_solution_letter_properties.apply->setToolTip(QString());
}

