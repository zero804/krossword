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

#ifndef CROSSWORDTYPECONFIGUREDETAILSDIALOG_H
#define CROSSWORDTYPECONFIGUREDETAILSDIALOG_H

#include "krossword.h"
#include "ui_configure_details.h"

#include <KDialog>

using namespace Crossword;

/** A dialog to configure name, description, etc. and the rules of a crossword
* type. */
class CrosswordTypeConfigureDetailsDialog : public KDialog {
  Q_OBJECT

  public:
    /** Read only modes. */
    enum ReadOnlyMode {
      ReadOnly, /**< All widgets in the dialog are readonly. */
      ReadOnlyWithInfo, /**< All widgets in the dialog are readonly.
	* An info label shows information how to edit the crossword type. */
      Editable /**< All widgets in the dialog are editable. */
    };

    /** Constructs a dialog with the given @p crosswordTypeInfo. */
    CrosswordTypeConfigureDetailsDialog( QWidget* parent,
		CrosswordTypeInfo crosswordTypeInfo,
		Qt::WFlags flags = 0 );

    /** Returns the current crossword type info. */
    CrosswordTypeInfo crosswordTypeInfo() const {
	return m_typeInfo; };
    /** Returns whether or not some value has been changed. */
    bool changed() const { return m_changed; };
    /** Return whether or not the dialog is in read only mode. */
    bool isReadOnly() const { return m_readOnly; };
    /** Sets the read only mode of the dialog. */
    void setReadOnly( ReadOnlyMode readOnlyMode );

  protected slots:
    void iconChanged( const QString &iconName );
    void nameChanged( const QString &name );
    void descriptionChanged();
    void longDescriptionChanged();
    void minAnswerLengthChanged( int minAnswerLength );
    void clueCellHandlingChanged( int index );
    void clueTypeChanged( int index );
    void letterCellContentChanged( int index );
    void clueMappingChanged( int index );
    void cellTypeToggled( QListWidgetItem *item );

  private:
    void setup();
    void setupConnections( bool disconnection = false );
    void setCrosswordType( CrosswordTypeInfo crosswordTypeInfo );

    void setValuesForClueCellHandling( ClueCellHandling clueCellHandling );
    void setValuesForClueCellType( bool allowed );

    Ui::configure_details ui_configure_details;
    CrosswordTypeInfo m_typeInfo;
    bool m_changed;
    bool m_readOnly;
};

#endif // CROSSWORDTYPECONFIGUREDETAILSDIALOG_H
