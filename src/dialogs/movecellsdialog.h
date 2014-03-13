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

#ifndef MOVECELLSDIALOG_H
#define MOVECELLSDIALOG_H

#include "ui_move_cells.h"

namespace Crossword
{
class KrossWord;
}
using namespace Crossword;

/** A dialog to move all cells of a crossword. */
class MoveCellsDialog : public KDialog
{
    Q_OBJECT

public:
    explicit MoveCellsDialog(KrossWord *krossWord, QWidget* parent = 0);

    int moveHorizontal() const {
        return ui_move_cells.dx->value();
    };
    int moveVertical() const {
        return ui_move_cells.dy->value();
    };

protected slots:
    void updateInfoText();

private:
    KrossWord *m_krossWord;
    Ui::move_cells ui_move_cells;
};


#endif // MOVECELLSDIALOG_H
