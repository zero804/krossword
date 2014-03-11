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

#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "ui_database_connection.h"

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QThread>
#include <QSqlDatabase>

class QProgressBar;
class KDialog;
class ExtendedSqlTableModel;

class KrosswordDictionary : public QObject {
  Q_OBJECT

  public:
	KrosswordDictionary( QObject* parent = 0 );
	virtual ~KrosswordDictionary();

	static const int MAX_WORD_LENGTH = 256;
	bool openDatabase( QWidget *dlgParent );
	void closeDatabase();
	bool createTables();

	bool isDatabaseOk() const { return m_databaseOk; };

	bool isEmpty();
	int entryCount();

	bool exportToCsv( const QString &fileName );
	int importFromCsv( const QString &fileName, QWidget *parent );

	ExtendedSqlTableModel *createModel();

	int addEntriesFromCrosswords( const QStringList &fileNames, QWidget *parent );
	int addEntriesFromDictionary( const QString &fileName, QWidget *parent );

	bool clearDatabase();

    public slots:
	void cancelCurrentActionClicked();

    signals:
	void extractedEntriesFromCrossword( const QString &fileName, int entryCount );
	void errorExtractedEntriesFromCrossword( const QString &fileName, const QString &errorString );

    private:
	KDialog *createProgressDialog( QWidget *parent, const QString &text, QProgressBar *progressBar );
	bool checkDatabase();

	Ui::database_connection ui_database_connection;
	bool m_cancel;
	bool m_databaseOk;
};

#endif // DICTIONARY_H
