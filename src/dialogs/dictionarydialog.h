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

#ifndef DICTIONARYDIALOG_H
#define DICTIONARYDIALOG_H

#include "ui_dictionaries.h"
#include <KDialog>

class ExtendedSqlTableModel;
class KrosswordDictionary;

class DictionaryDialog : public KDialog
{
    Q_OBJECT

public:
    explicit DictionaryDialog( KrosswordDictionary *dictionary, QWidget* parent = 0 );

    ExtendedSqlTableModel *databaseTable() const {
        return m_dbTable;
    };

protected slots:
    void tableSelectionChanged( const QItemSelection &selected,
                                const QItemSelection &deselected );
    void addDictionaryFromCrosswordsClicked();
    void addDictionaryFromLibraryClicked();
    void getWordsFromDictionaryClicked();
    void addEntryClicked();
    void removeEntriesClicked();
    void clearClicked();
    void importFromCsvClicked();
    void exportToCsvClicked();
    void filterChanged( const QString &filter );
    void hideInfoMessage();

private:
    void showInfoMessage( const QString &infoMessage );

    Ui::dictionaries ui_dictionaries;
    KrosswordDictionary *m_dictionary;
    ExtendedSqlTableModel *m_dbTable;
    QLabel *m_infoMessage;
};

#endif // DICTIONARYDIALOG_H
