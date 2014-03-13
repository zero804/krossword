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

#include "movecellsdialog.h"
#include <cells/krosswordcell.h>
#include <krossword.h>
#include <KColorScheme>


MoveCellsDialog::MoveCellsDialog( KrossWord* krossWord, QWidget* parent )
        : KDialog( parent ), m_krossWord( krossWord )
{
    setWindowTitle( i18n( "Move Cells" ) );
    QWidget *mainWidget = new QWidget;
    ui_move_cells.setupUi( mainWidget );
    setMainWidget( mainWidget );
    setModal( true );

    ui_move_cells.dx->setRange( -krossWord->width(), krossWord->width() );
    ui_move_cells.dx->setValue( 0 );

    ui_move_cells.dy->setRange( -krossWord->height(), krossWord->height() );
    ui_move_cells.dy->setValue( 0 );

    updateInfoText();

    connect( ui_move_cells.dx, SIGNAL( valueChanged( int ) ),
             this, SLOT( updateInfoText() ) );
    connect( ui_move_cells.dy, SIGNAL( valueChanged( int ) ),
             this, SLOT( updateInfoText() ) );
}

void MoveCellsDialog::updateInfoText()
{
    KrossWordCellList removedCells = m_krossWord->moveCells(
                                         ui_move_cells.dx->value(), ui_move_cells.dy->value(), true );
    int clueCount = 0, imageCount = 0;
    foreach( KrossWordCell *cell, removedCells ) {
        if ( cell->isType( ClueCellType ) )
            ++clueCount;
        else if ( cell->isType( ImageCellType ) )
            ++imageCount;
    }
    ui_move_cells.lblMoveInfo->setText( i18ncp( "How many clues are removed "
                                        "when moving all cells of the crossword. %2 is replaced by a plural "
                                        "indicating how many images will be removed when moving the cells.",
                                        "Moving all cells will remove %1 clue %2",
                                        "Moving all cells will remove %1 clues %2",
                                        clueCount,
                                        i18ncp( "How many images are removed when moving all cells of the crossword. "
                                                "This string gets combined with a plural string indicating how many "
                                                "clue cells will be removed when moving the cells.",
                                                "and %1 image", "and %1 images", imageCount ) ) );

    bool hasRemovedCells = clueCount > 0 || imageCount > 0;
    KColorScheme colorScheme( QPalette::Normal );
    QPalette palette = ui_move_cells.lblMoveInfo->palette();
    palette.setColor( QPalette::Normal, QPalette::WindowText,
                      colorScheme.foreground( hasRemovedCells
                                              ? KColorScheme::NegativeText : KColorScheme::PositiveText ).color() );
    ui_move_cells.lblMoveInfo->setPalette( palette );
}


