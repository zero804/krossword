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

#ifndef DICTIONARYMANAGER_H
#define DICTIONARYMANAGER_H

#include "ui_database_connection.h"

#include <QObject>
#include <QStringList>
#include <QSqlQuery>

class QProgressBar;
class QDialog;

class DictionaryModel;

class DictionaryManager : public QObject
{
    Q_OBJECT

public:
    DictionaryManager(QObject* parent = nullptr);
    virtual ~DictionaryManager();

    void closeDatabase();

    bool isReady() const;
    bool setupDatabase(QWidget *dlgParent);

    bool isEmpty(); // just itemsCount()?
    int itemsCount();

    int importFromCrosswords(const QStringList &fileNames, QWidget *parent);
    int importFromDictionary(const QString &fileName, QWidget *parent);
    int importFromCsv(const QString &fileName, QWidget *parent);
    bool exportToCsv(const QString &fileName);

    bool clearDatabase();

    DictionaryModel *getModel();

signals:
    void extractedEntriesFromCrossword(const QString &fileName, int itemsCount);
    void errorExtractedEntriesFromCrossword(const QString &fileName, const QString &errorString);

private:
    QDialog *createProgressDialog(QWidget *parent, const QString &text, QProgressBar *progressBar);

    bool m_hasConnection;
    QSqlDatabase m_database;
    DictionaryModel *m_model;

    bool createConnection();

    bool createUser(QSqlQuery &query);
    bool createDatabase(QSqlQuery &query);
    bool createTable();

    void createModel();

private:
    Ui::database_connection ui_database_connection;

    static const int MAX_WORD_LENGTH = 256;
    static const QString CONNECTION_NAME;
};

#endif // DICTIONARYMANAGER_H
