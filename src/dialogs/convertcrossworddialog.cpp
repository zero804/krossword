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

#include "convertcrossworddialog.h"
#include <KMessageBox>
#include <QStandardItemModel>
#include <KColorScheme>


ConvertCrosswordDialog::ConvertCrosswordDialog(
	KrossWord* krossWord, QWidget* parent, Qt::WFlags flags )
	: KDialog( parent, flags ), m_krossWord( krossWord ) {
  setWindowTitle( i18n("Convert Crossword Type To") );
  QWidget *convertDlg = new QWidget;
  ui_convert_crossword_type.setupUi( convertDlg );
  setModal( true );

  ui_convert_crossword_type.toolBox->setItemText( 0, i18n("Convert From: %1",
      krossWord->crosswordTypeInfo().name) );
  ui_convert_crossword_type.typeInfoFromWidget->setElements(
      CrosswordTypeWidget::DefaultElements | CrosswordTypeWidget::ElementIcon );
  ui_convert_crossword_type.typeInfoToWidget->setElements();
  ui_convert_crossword_type.typeInfoFromWidget->setTypeInfo(
      krossWord->crosswordTypeInfo() );

  // Setup crossword type view
  QList<CrosswordTypeInfo> additionalInfos;
  if ( krossWord->crosswordTypeInfo().crosswordType == UserDefinedCrossword )
      additionalInfos << krossWord->crosswordTypeInfo();
  QStandardItemModel *model = KrossWord::createCrosswordTypeModel( additionalInfos );
  ui_convert_crossword_type.crosswordType->setModel( model );
  ui_convert_crossword_type.crosswordType->setModelColumn( 0 );

  m_convertTypeInfo = CrosswordTypeInfo::american();
  m_changedUserDefinedSettings = false;

  m_previousConvertToTypeIndex = -1;
  ui_convert_crossword_type.crosswordType->setCurrentIndex( 0 );
  crosswordTypeChanged( 0 );

  connect( ui_convert_crossword_type.crosswordType,
	   SIGNAL(currentIndexChanged(int)),
	   this, SLOT(crosswordTypeChanged(int)) );
  connect( ui_convert_crossword_type.typeInfoToWidget,
	   SIGNAL(crosswordTypeInfoChanged(CrosswordTypeInfo)),
	   this, SLOT(crosswordTypeInfoChanged(CrosswordTypeInfo)) );

  setMainWidget( convertDlg );
}

void ConvertCrosswordDialog::crosswordTypeChanged( int index ) {
  QModelIndex current = ui_convert_crossword_type.crosswordType->model()->index(
      index, ui_convert_crossword_type.crosswordType->modelColumn() );
  QModelIndex previous;
  if ( m_previousConvertToTypeIndex != -1 )
    previous = ui_convert_crossword_type.crosswordType->model()->index(
	m_previousConvertToTypeIndex, ui_convert_crossword_type.crosswordType->modelColumn() );
  m_previousConvertToTypeIndex = index;
  if ( !current.isValid() )
    return;

  // Warn when trashing user defined crossword type settings
  if ( m_changedUserDefinedSettings ) {
    if ( static_cast<CrosswordType>(previous.data(
      Qt::UserRole + 1).toInt()) != UserDefinedCrossword ) {
      return;
    } else if ( KMessageBox::warningContinueCancel(this, i18n("Changing the "
		"crossword type will discard user defined crossword type settings."),
		QString(), KStandardGuiItem::discard()) == KMessageBox::Cancel ) {
      ui_convert_crossword_type.crosswordType->setCurrentIndex( previous.row() );
      return;
    }
  }

  // In (Qt::UserRole + 2) the crossword type info object is stored for each item.
  if ( current.data(Qt::UserRole + 2).isValid() ) {
    m_convertTypeInfo = current.data( Qt::UserRole + 2 ).value< CrosswordTypeInfo >();
  } else {
    CrosswordType crosswordType =
	static_cast<CrosswordType>( current.data(Qt::UserRole + 1).toInt() );
    if ( crosswordType == UserDefinedCrossword ) {
      CrosswordTypeInfo userDefinedTypeInfo =
      CrosswordTypeInfo::infoFromType( crosswordType );
      m_convertTypeInfo.crosswordType = UserDefinedCrossword;
      m_convertTypeInfo.name = userDefinedTypeInfo.name;
      m_convertTypeInfo.description = userDefinedTypeInfo.description;
      m_convertTypeInfo.longDescription = userDefinedTypeInfo.longDescription;
      m_convertTypeInfo.iconName = userDefinedTypeInfo.iconName;
    } else
      m_convertTypeInfo = CrosswordTypeInfo::infoFromType( crosswordType );
  }
  m_changedUserDefinedSettings = false;

  ui_convert_crossword_type.toolBox->setItemText( 1,
      i18n("Convert To: %1", m_convertTypeInfo.name) );
  ui_convert_crossword_type.typeInfoToWidget->setTypeInfo( m_convertTypeInfo );
//   ui_convert_crossword_type.lblInfo->setText( m_convertTypeInfo.description );

  // Update conversion info
  KrossWord::ConversionInfo conversionInfo =
      m_krossWord->convertToType( m_convertTypeInfo, true );
  ui_convert_crossword_type.conversionInfo->setText(
      m_krossWord->conversionInfoToString(conversionInfo) );

  bool removedCells = !conversionInfo.cellsToRemove.isEmpty()
		      || !conversionInfo.cluesToRemove.isEmpty();
  KColorScheme colorScheme( QPalette::Normal );
  QPalette palette = ui_convert_crossword_type.conversionInfo->palette();
  palette.setColor( QPalette::Normal, QPalette::WindowText,
		    colorScheme.foreground(removedCells
		    ? KColorScheme::NegativeText : KColorScheme::PositiveText).color() );
  ui_convert_crossword_type.conversionInfo->setPalette( palette );
}

void ConvertCrosswordDialog::crosswordTypeInfoChanged(
	const CrosswordTypeInfo &typeInfo ) {
  m_convertTypeInfo = typeInfo;

  // Update conversion info
  KrossWord::ConversionInfo conversionInfo =
      m_krossWord->convertToType( typeInfo, true );
  ui_convert_crossword_type.conversionInfo->setText(
      m_krossWord->conversionInfoToString(conversionInfo) );

  m_changedUserDefinedSettings = true;
}




