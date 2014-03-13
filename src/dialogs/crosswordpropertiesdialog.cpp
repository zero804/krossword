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

#include "crosswordpropertiesdialog.h"

#include "convertcrossworddialog.h"
#include "krossword.h"
#include "cells/krosswordcell.h"

//#include <KGameDifficulty>
#include <KgDifficulty>
#include <KColorScheme>


const QList< QChar > CrosswordPropertiesDialog::ArrowChars = QList<QChar>()
        << QChar( 0x2196 ) << QChar( 0x2191 ) << QChar( 0x2197 )
        << QChar( 0x2190 ) << QChar( 0x2022 ) << QChar( 0x2192 )
        << QChar( 0x2199 ) << QChar( 0x2193 ) << QChar( 0x2198 );

CrosswordPropertiesDialog::CrosswordPropertiesDialog(
    KrossWord* krossWord, QWidget* parent, Qt::WFlags flags )
        : KDialog( parent, flags ), m_krossWord( krossWord )
{
    setWindowTitle( i18n( "Properties" ) );
    QWidget *propertiesDlg = new QWidget;
    ui_properties.setupUi( propertiesDlg );
    setModal( true );

    ui_properties.btnReset->setIcon( KIcon( "edit-undo" ) );
    ui_properties.btnReset->setEnabled( false );

    ui_properties.toolBox->setItemText( ui_properties.toolBox->indexOf(
                                            ui_properties.pageCrosswordType ), i18n( "Crossword Type: %1",
                                                    krossWord->crosswordTypeInfo().name ) );
    ui_properties.typeInfoWidget->setElements(
        CrosswordTypeWidget::ElementDetails | CrosswordTypeWidget::ElementIcon );
    ui_properties.typeInfoWidget->setEditMode( CrosswordTypeWidget::EditAlwaysReadOnly );
    ui_properties.typeInfoWidget->addUserButtonElement( i18n( "&Convert..." ),
            this, SLOT( convertClicked() ) );
    ui_properties.typeInfoWidget->setTypeInfo( krossWord->crosswordTypeInfo() );

    QStringList languages = KGlobal::locale()->allLanguagesList();
    languages.sort();
    foreach( const QString &language, languages )
    ui_properties.language->insertLanguage( language );

    //QMap<QByteArray, QString> difficulties = KGameDifficulty::localizedLevelStrings();
    //ui_properties.difficulty->addItems( difficulties.values() );

    ui_properties.title->setText( krossWord->title() );
    ui_properties.author->setText( krossWord->authors() );
    ui_properties.copyright->setText( krossWord->copyright() );
    ui_properties.notes->setText( krossWord->notes() );

    ui_properties.anchorCenter->setChecked( true );
#if KDE_IS_VERSION(4,3,0)
    m_anchorIdToAnchor.insert( ui_properties.buttonGroupAnchor->id(
                                   ui_properties.anchorTopLeft ), KrossWord::AnchorTopLeft );
    m_anchorIdToAnchor.insert( ui_properties.buttonGroupAnchor->id(
                                   ui_properties.anchorTop ), KrossWord::AnchorTop );
    m_anchorIdToAnchor.insert( ui_properties.buttonGroupAnchor->id(
                                   ui_properties.anchorTopRight ), KrossWord::AnchorTopRight );
    m_anchorIdToAnchor.insert( ui_properties.buttonGroupAnchor->id(
                                   ui_properties.anchorLeft ), KrossWord::AnchorLeft );
    m_anchorIdToAnchor.insert( ui_properties.buttonGroupAnchor->id(
                                   ui_properties.anchorCenter ), KrossWord::AnchorCenter );
    m_anchorIdToAnchor.insert( ui_properties.buttonGroupAnchor->id(
                                   ui_properties.anchorRight ), KrossWord::AnchorRight );
    m_anchorIdToAnchor.insert( ui_properties.buttonGroupAnchor->id(
                                   ui_properties.anchorBottomLeft ), KrossWord::AnchorBottomLeft );
    m_anchorIdToAnchor.insert( ui_properties.buttonGroupAnchor->id(
                                   ui_properties.anchorBottom ), KrossWord::AnchorBottom );
    m_anchorIdToAnchor.insert( ui_properties.buttonGroupAnchor->id(
                                   ui_properties.anchorBottomRight ), KrossWord::AnchorBottomRight );
#else
    m_anchorIdToAnchor.insert( 0, KrossWord::AnchorTopLeft );
    m_anchorIdToAnchor.insert( 1, KrossWord::AnchorTop );
    m_anchorIdToAnchor.insert( 2, KrossWord::AnchorTopRight );
    m_anchorIdToAnchor.insert( 3, KrossWord::AnchorLeft );
    m_anchorIdToAnchor.insert( 4, KrossWord::AnchorCenter );
    m_anchorIdToAnchor.insert( 5, KrossWord::AnchorRight );
    m_anchorIdToAnchor.insert( 6, KrossWord::AnchorBottomLeft );
    m_anchorIdToAnchor.insert( 7, KrossWord::AnchorBottom );
    m_anchorIdToAnchor.insert( 8, KrossWord::AnchorBottomRight );
#endif

    connect( ui_properties.buttonGroupAnchor, SIGNAL( changed( int ) ),
             this, SLOT( resizeAnchorChanged( int ) ) );
    connect( ui_properties.btnReset, SIGNAL( clicked() ),
             this, SLOT( resetSizeClicked() ) );

    ui_properties.columns->setValue( krossWord->width() );
    ui_properties.rows->setValue( krossWord->height() );

    connect( ui_properties.columns, SIGNAL( valueChanged( int ) ),
             this, SLOT( columnsChanged( int ) ) );
    connect( ui_properties.rows, SIGNAL( valueChanged( int ) ),
             this, SLOT( rowsChanged( int ) ) );
    // To update the resize info label
    sizeChanged( krossWord->width(), krossWord->height() );

    setMainWidget( propertiesDlg );
}

void CrosswordPropertiesDialog::resetSizeClicked()
{
    ui_properties.columns->setValue( m_krossWord->width() );
    ui_properties.rows->setValue( m_krossWord->height() );
}

void CrosswordPropertiesDialog::resizeAnchorChanged( int id )
{
    KrossWord::ResizeAnchor anchor = m_anchorIdToAnchor[ id ];
    setAnchorIcons( anchor );
    updateInfoText( anchor );
}

void CrosswordPropertiesDialog::setAnchorIcons( KrossWord::ResizeAnchor anchor )
{
    ui_properties.anchorTopLeft->setText( QString() );
    ui_properties.anchorTop->setText( QString() );
    ui_properties.anchorTopRight->setText( QString() );
    ui_properties.anchorLeft->setText( QString() );
    ui_properties.anchorCenter->setText( QString() );
    ui_properties.anchorRight->setText( QString() );
    ui_properties.anchorBottomLeft->setText( QString() );
    ui_properties.anchorBottom->setText( QString() );
    ui_properties.anchorBottomRight->setText( QString() );

    bool shrinkH = ( uint )columns() < m_krossWord->width();
    bool shrinkV = ( uint )rows() < m_krossWord->height();

    switch ( anchor ) {
    case KrossWord::AnchorCenter:
        if ( shrinkV ) {
            ui_properties.anchorTopRight->setText( shrinkH ? ArrowChars[ArrowSW] : ArrowChars[ArrowS] );
            ui_properties.anchorTopLeft->setText( shrinkH ? ArrowChars[ArrowSE] : ArrowChars[ArrowS] );
            ui_properties.anchorBottomLeft->setText( shrinkH ? ArrowChars[ArrowNE] : ArrowChars[ArrowN] );
            ui_properties.anchorBottomRight->setText( shrinkH ? ArrowChars[ArrowNW] : ArrowChars[ArrowN] );
        } else {
            ui_properties.anchorTopRight->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowNE] );
            ui_properties.anchorTopLeft->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowNW] );
            ui_properties.anchorBottomLeft->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowSW] );
            ui_properties.anchorBottomRight->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowSE] );
        }
        ui_properties.anchorTop->setText( shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN] );
        ui_properties.anchorLeft->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW] );
        ui_properties.anchorCenter->setText( ArrowChars[ArrowNone] );
        ui_properties.anchorRight->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE] );
        ui_properties.anchorBottom->setText( shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS] );
        break;
    case KrossWord::AnchorTopLeft:
        ui_properties.anchorTopLeft->setText( ArrowChars[ArrowNone] );
        ui_properties.anchorTop->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE] );
        ui_properties.anchorLeft->setText( shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS] );
        if ( shrinkV )
            ui_properties.anchorCenter->setText( shrinkH ? ArrowChars[ArrowNW] : ArrowChars[ArrowN] );
        else
            ui_properties.anchorCenter->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowSE] );
        break;
    case KrossWord::AnchorTop:
        ui_properties.anchorTopLeft->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW] );
        ui_properties.anchorTop->setText( ArrowChars[ArrowNone] );
        ui_properties.anchorTopRight->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE] );
        ui_properties.anchorCenter->setText( shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS] );
        if ( shrinkV ) {
            ui_properties.anchorLeft->setText( shrinkH ? ArrowChars[ArrowNE] : ArrowChars[ArrowN] );
            ui_properties.anchorRight->setText( shrinkH ? ArrowChars[ArrowNW] : ArrowChars[ArrowN] );
        } else {
            ui_properties.anchorLeft->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowSW] );
            ui_properties.anchorRight->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowSE] );
        }
        break;
    case KrossWord::AnchorTopRight:
        ui_properties.anchorTop->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW] );
        ui_properties.anchorTopRight->setText( ArrowChars[ArrowNone] );
        if ( shrinkV )
            ui_properties.anchorCenter->setText( shrinkH ? ArrowChars[ArrowNE] : ArrowChars[ArrowN] );
        else
            ui_properties.anchorCenter->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowSW] );
        ui_properties.anchorRight->setText( shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS] );
        break;
    case KrossWord::AnchorLeft:
        ui_properties.anchorTopLeft->setText( shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN] );
        ui_properties.anchorLeft->setText( ArrowChars[ArrowNone] );
        ui_properties.anchorCenter->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE] );
        ui_properties.anchorBottomLeft->setText( shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS] );
        if ( shrinkV ) {
            ui_properties.anchorTop->setText( shrinkH ? ArrowChars[ArrowSW] : ArrowChars[ArrowS] );
            ui_properties.anchorBottom->setText( shrinkH ? ArrowChars[ArrowNW] : ArrowChars[ArrowN] );
        } else {
            ui_properties.anchorTop->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowNE] );
            ui_properties.anchorBottom->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowSE] );
        }
        break;
    case KrossWord::AnchorRight:
        ui_properties.anchorTopRight->setText( shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN] );
        ui_properties.anchorCenter->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW] );
        ui_properties.anchorRight->setText( ArrowChars[ArrowNone] );
        ui_properties.anchorBottomRight->setText( shrinkV ? ArrowChars[ArrowN] : ArrowChars[ArrowS] );
        if ( shrinkV ) {
            ui_properties.anchorBottom->setText( shrinkH ? ArrowChars[ArrowNE] : ArrowChars[ArrowN] );
            ui_properties.anchorTop->setText( shrinkH ? ArrowChars[ArrowSE] : ArrowChars[ArrowS] );
        } else {
            ui_properties.anchorBottom->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowSW] );
            ui_properties.anchorTop->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowNW] );
        }
        break;
    case KrossWord::AnchorBottomLeft:
        ui_properties.anchorLeft->setText( shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN] );
        if ( shrinkV )
            ui_properties.anchorCenter->setText( shrinkH ? ArrowChars[ArrowSW] : ArrowChars[ArrowS] );
        else
            ui_properties.anchorCenter->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowNE] );
        ui_properties.anchorBottomLeft->setText( ArrowChars[ArrowNone] );
        ui_properties.anchorBottom->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE] );
        break;
    case KrossWord::AnchorBottom:
        ui_properties.anchorCenter->setText( shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN] );
        ui_properties.anchorBottomLeft->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW] );
        ui_properties.anchorBottom->setText( ArrowChars[ArrowNone] );
        ui_properties.anchorBottomRight->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowE] );
        if ( shrinkV ) {
            ui_properties.anchorLeft->setText( shrinkH ? ArrowChars[ArrowSE] : ArrowChars[ArrowS] );
            ui_properties.anchorRight->setText( shrinkH ? ArrowChars[ArrowSW] : ArrowChars[ArrowS] );
        } else {
            ui_properties.anchorLeft->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowNW] );
            ui_properties.anchorRight->setText( shrinkH ? ArrowChars[ArrowW] : ArrowChars[ArrowNE] );
        }
        break;
    case KrossWord::AnchorBottomRight:
        if ( shrinkV )
            ui_properties.anchorCenter->setText( shrinkH ? ArrowChars[ArrowSE] : ArrowChars[ArrowS] );
        else
            ui_properties.anchorCenter->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowNW] );
        ui_properties.anchorRight->setText( shrinkV ? ArrowChars[ArrowS] : ArrowChars[ArrowN] );
        ui_properties.anchorBottom->setText( shrinkH ? ArrowChars[ArrowE] : ArrowChars[ArrowW] );
        ui_properties.anchorBottomRight->setText( ArrowChars[ArrowNone] );
        break;
    }
}

void CrosswordPropertiesDialog::rowsChanged( int rows )
{
    sizeChanged( ui_properties.columns->value(), rows );
}

void CrosswordPropertiesDialog::columnsChanged( int columns )
{
    sizeChanged( columns, ui_properties.rows->value() );
}

void CrosswordPropertiesDialog::sizeChanged( int columns, int rows )
{
    setAnchorIcons( anchor() );
    updateInfoText( columns, rows );

    ui_properties.btnReset->setEnabled(
        ui_properties.columns->value() != ( int )m_krossWord->width()
        || ui_properties.rows->value() != ( int )m_krossWord->height() );
}

void CrosswordPropertiesDialog::updateInfoText( KrossWord::ResizeAnchor anchor,
        int columns, int rows )
{
    KrossWordCellList removedCells = m_krossWord->resizeGrid( columns, rows,
                                     anchor, true );
    int clueCount = 0, imageCount = 0;
    foreach( KrossWordCell *cell, removedCells ) {
        if ( cell->isType( ClueCellType ) )
            ++clueCount;
        else if ( cell->isType( ImageCellType ) )
            ++imageCount;
    }
    if ( ui_properties.columns->value() == ( int )m_krossWord->width()
            && ui_properties.rows->value() == ( int )m_krossWord->height() ) {
        ui_properties.lblResizeInfo->setText(
            i18nc( "No changes to the crossword grid size.", "No change" ) );
        ui_properties.lblResizeInfo->setEnabled( false );
    } else {
        ui_properties.lblResizeInfo->setText( i18ncp( "How many clues are removed "
                                              "when resizing the crossword. %2 is replaced by a plural indicating how "
                                              "many images will be removed when resizing.",
                                              "Resizing from %3x%4 will remove %1 clue %2",
                                              "Resizing from %3x%4 will remove %1 clues %2",
                                              clueCount,
                                              i18ncp( "How many images are removed when resizing the crossword. This "
                                                      "string gets combined with a plural string indicating how many "
                                                      "clue cells will be removed when resizing.",
                                                      "and %1 image", "and %1 images", imageCount ),
                                              m_krossWord->width(), m_krossWord->height() ) );
        ui_properties.lblResizeInfo->setEnabled( true );
    }

    bool hasRemovedCells = clueCount > 0 || imageCount > 0;
    KColorScheme colorScheme( QPalette::Normal );
    QPalette palette = ui_properties.lblResizeInfo->palette();
    palette.setColor( QPalette::Normal, QPalette::WindowText,
                      colorScheme.foreground( hasRemovedCells
                                              ? KColorScheme::NegativeText : KColorScheme::PositiveText ).color() );
    ui_properties.lblResizeInfo->setPalette( palette );
}

void CrosswordPropertiesDialog::convertClicked()
{
    ConvertCrosswordDialog *dialog = new ConvertCrosswordDialog( m_krossWord, this );
//   dialog->set()
    // TODO:
//     KDialog *dialog = new KDialog( m_propertiesDialog );
//     dialog->setWindowTitle( i18n("Convert Crossword Type To") );
//     QWidget *convertDlg = new QWidget;
//     ui_convert_crossword_type.setupUi( convertDlg );
//
// //     ui_convert_crossword_type.arrow->setText( QString() );
// //     ui_convert_crossword_type.arrow->setPixmap( KIcon("arrow-down").pixmap(32, 32, QIcon::Normal) );
//
//     ui_convert_crossword_type.toolBox->setItemText( 0, i18n("Convert From: %1",
//  krossWord()->crosswordTypeInfo().name) );
//     ui_convert_crossword_type.typeInfoFromWidget->setElements(
//  CrosswordTypeWidget::DefaultElements | CrosswordTypeWidget::ElementIcon );
//     ui_convert_crossword_type.typeInfoToWidget->setElements();
//     ui_convert_crossword_type.typeInfoFromWidget->setTypeInfo(
//  krossWord()->crosswordTypeInfo() );
// //     ui_convert_crossword_type.lblInfoOld->setText( krossWord()->crosswordTypeInfo().description );
//
//     // Setup crossword type view
//     QList<CrosswordTypeInfo> additionalInfos;
//     if ( krossWord()->crosswordTypeInfo().crosswordType
//   == UserDefinedCrossword )
//  additionalInfos << krossWord()->crosswordTypeInfo();
//     QStandardItemModel *model = KrossWord::createCrosswordTypeModel( additionalInfos );
//     ui_convert_crossword_type.crosswordType->setModel( model );
//     ui_convert_crossword_type.crosswordType->setModelColumn( 0 );
//
//     m_convertTypeInfo = CrosswordTypeInfo::american();
//     m_convertChangedUserDefinedSettings = false;
//
//     m_previousConvertToTypeIndex = -1;
//     ui_convert_crossword_type.crosswordType->setCurrentIndex( 0 );
//     createNewCurrentCrosswordTypeChanged( 0 );
    /*
        connect( ui_convert_crossword_type.crosswordType,
          SIGNAL(currentIndexChanged(int)),
          this, SLOT(createNewCurrentCrosswordTypeChanged(int)) );
        connect( ui_convert_crossword_type.typeInfoToWidget,
          SIGNAL(crosswordTypeInfoChanged(CrosswordTypeInfo)),
          this, SLOT(createNewCrosswordTypeInfoChanged(CrosswordTypeInfo)) );
    //     connect( ui_convert_crossword_type.crosswordType->selectionModel(),
    //       SIGNAL(currentChanged(QModelIndex,QModelIndex)),
    //       this, SLOT(createNewCurrentCrosswordTypeChanged(QModelIndex,QModelIndex)) );
    //     connect( ui_convert_crossword_type.configureDetailsFrom, SIGNAL(clicked()),
    //       this, SLOT(createNewConfigureRulesFrom()) );
    //     connect( ui_convert_crossword_type.configureDetailsTo, SIGNAL(clicked()),
    //       this, SLOT(createNewConfigureRulesTo()) );

        dialog->setModal( true );
        dialog->setMainWidget( convertDlg );*/
    if ( dialog->exec() == KDialog::Accepted ) {
        CrosswordTypeInfo typeInfo = dialog->crosswordTypeInfo();
        emit conversionRequested( typeInfo );
//       QString errorMessage;
//       if ( !m_undoStack->tryPush(
//  new ConvertCrosswordCommand(m_krossWord, m_krossWord->crosswordTypeInfo()), &errorMessage) ) {
//  statusBar()->showMessage(
//    i18nc("%1 contains the reason why the crossword couldn't be converted",
//    "Can't convert crossword. %1", errorMessage) );
//       } else {
//  stateChanged( "clue_cell_highlighted" );

        ui_properties.toolBox->setItemText(
            ui_properties.toolBox->indexOf( ui_properties.pageCrosswordType ),
            i18n( "Crossword Type: %1", typeInfo.name ) );
        ui_properties.typeInfoWidget->setTypeInfo( typeInfo );
//  ui_properties.lblInfo->setText( krossWord()->crosswordTypeInfo().description );

//  enableEditActions();
//  adjustGuiToCrosswordType();
//  setModificationType( ModifiedCrossword );
//       }
    }

    delete dialog;
}

int CrosswordPropertiesDialog::columns() const
{
    return ui_properties.columns->value();
}

int CrosswordPropertiesDialog::rows() const
{
    return ui_properties.rows->value();
}

