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

#include "dictionarymanager.h"
#include "dictionarymodel.h"

#include "krossword.h"
#include "cells/cluecell.h"
#include "htmldelegate.h"

#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QSqlDatabase>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QLabel>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include <QDebug>
#include <KUrl>
#include <KStandardGuiItem>
#include <KMessageBox>
#include <QDialog>

using namespace Crossword;

const QString DictionaryManager::CONNECTION_NAME = "krosswordpuzzle";

DictionaryManager::DictionaryManager(QObject* parent)
    : QObject(parent)
{
    m_hasConnection = createConnection();

    if (m_hasConnection) {
        qDebug() << "Database ready";
        createModel();
    } else {
        qDebug() << "Database has not been set up yet";
    }
}

DictionaryManager::~DictionaryManager()
{
    qDebug() << "Closing and removing database connection...";
    closeDatabase();
    qDebug() << "Done";
}

bool DictionaryManager::createConnection()
{
    if (!QSqlDatabase::contains(CONNECTION_NAME)) {
        m_database = QSqlDatabase::addDatabase("QMYSQL", CONNECTION_NAME);
    } else {
        m_database = QSqlDatabase::database(CONNECTION_NAME);
    }

    m_database.setHostName("localhost");
    m_database.setUserName("krosswordpuzzle");
    m_database.setDatabaseName("krosswordpuzzle");
    m_database.setPassword("krosswordpuzzle");

    return m_database.open();
}

void DictionaryManager::createModel()
{
    m_model = new DictionaryModel(this, m_database);
    m_model->setTable("dictionary");
    m_model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    m_model->setHeaderData(1, Qt::Horizontal,
                           i18nc("The header title for answers in the dictionary database", "Answer"), Qt::DisplayRole);
    m_model->setHeaderData(2, Qt::Horizontal,
                           i18nc("The header title for clues associated with answer words in the dictionary database", "Clue"), Qt::DisplayRole);
    m_model->setHeaderData(3, Qt::Horizontal,
                           i18nc("The header title for scores/difficulties of clue/answer pairs in the dictionary database", "Score"), Qt::DisplayRole);
    m_model->setHeaderData(4, Qt::Horizontal,
                           i18nc("The header title for languages of clue/answer pairs in the dictionary database", "Language"), Qt::DisplayRole);
}

DictionaryModel *DictionaryManager::getModel()
{
    m_model->sort(1, Qt::AscendingOrder);   // Sort by word, it does a select() too
    return m_model;
}

bool DictionaryManager::isReady() const {
    return m_hasConnection;
}

bool DictionaryManager::setupDatabase(QWidget *dlgParent)
{
    bool success = true;

    if (KMessageBox::warningContinueCancel(dlgParent, i18n("No Connection to the database! Please make sure the "
                    "MySQL server is running. Afterwards click \"Continue\" to retry connecting to the database."),
                    i18n("No Database Connection")) == KMessageBox::Cancel) {
        success = false;
    } else {
        QPointer<QDialog> dialog = new QDialog(dlgParent);
        dialog->setWindowTitle(i18n("Connect Database"));
        ui_database_connection.setupUi(dialog);
        dialog->setModal(true);

        if (dialog->exec() == QDialog::Accepted) {
            QSqlDatabase dbRoot = QSqlDatabase::addDatabase("QMYSQL", "root_connection");
            dbRoot.setHostName(ui_database_connection.host->text());
            dbRoot.setUserName(ui_database_connection.user->text());
            dbRoot.setPassword(ui_database_connection.password->text());

            if (dbRoot.open()) {
                QSqlQuery rootQuery(dbRoot);

                success = createUser(rootQuery);
                success = createDatabase(rootQuery);

            } else {
                success = false;
                qDebug() << "Error opening root connection";
            }

            dbRoot.close();
        } else { // QDialog::Rejected
            success = false;
        }

        delete dialog;
    }

    return success;
}

bool DictionaryManager::createUser(QSqlQuery &query)
{
    bool success = true;

    // This workaround should solve a bug in sql when creating user (error 1396)
    //rootQuery.exec("DROP USER krosswordpuzzle@localhost;");
    //rootQuery.exec("FLUSH PRIVILEGES;");

    if (!query.exec("CREATE USER krosswordpuzzle@localhost IDENTIFIED BY 'krosswordpuzzle'")) {
        qDebug() << "Error creating the db user" << query.lastError();
        success = false;
    } else if (!query.exec("GRANT ALL ON krosswordpuzzle.* TO 'krosswordpuzzle'@'localhost';")) {
        qDebug() << "Error granting privileges to the database krosswordpuzzle" << query.lastError();
        success = false;
    }

    return success;
}

bool DictionaryManager::createDatabase(QSqlQuery &query)
{
    bool success = true;

    if (!query.exec(QLatin1String("USE krosswordpuzzle"))) {
        qDebug() << "No database named 'krosswordpuzzle', creating it now";

        if (query.exec(QLatin1String("CREATE DATABASE krosswordpuzzle"))) {
            if (query.exec(QLatin1String("USE krosswordpuzzle"))) {
                qDebug() << "Database created, now creating the tables";

                success = createTable();
            } else {
                qDebug() << "Database created but \"USE [DATABASE]\" failed" << query.lastError();
                success = false;
            }
        } else {
            qDebug() << "Couldn't create database krosswordpuzzle" << query.lastError();
            success = false;
        }
    }

    return success;
}

bool DictionaryManager::createTable()
{
    bool ok = true;
    if (!m_database.isOpen()) {
        qDebug() << "Database isn't opened";
        return false;
    }

    QSqlQuery query(m_database);
    ok = query.exec("CREATE TABLE dictionary ( " \
                    "id INTEGER NOT NULL AUTO_INCREMENT, " \
                    "word VARCHAR (255) NOT NULL, " \
                    "clue VARCHAR (255), " \
                    "score INTEGER DEFAULT 0, " \
                    "language VARCHAR (3) DEFAULT 'en', " \
                    "PRIMARY KEY(id), " \
                    "UNIQUE (word) );");
    if (!ok)
        qDebug() << "Couldn't create table" << query.lastError();

    return ok;
}



void DictionaryManager::closeDatabase()
{
    if (m_database.isValid()) {
        m_database.close();
    }
}

bool DictionaryManager::exportToCsv(const QString& fileName)
{
    if (!m_database.isValid())
        return false;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    QTextStream stream(&file);
    QSqlQuery query = m_database.exec("SELECT * FROM dictionary");
    QSqlRecord rec = query.record();

    // Write field titles
    QStringList fieldNames;
    for (int field = 0; field < rec.count(); ++field)
        fieldNames << rec.fieldName(field);
    fieldNames.removeFirst();
    stream << fieldNames.join(";").append('\n');

    // Write rows
    while (query.next()) {
        QStringList fields;
        for (int field = 1; field < rec.count(); ++field)
            fields << QString("\"%1\"").arg(query.value(field).toString().replace('\"', "\"\""));
        stream << fields.join(";").append('\n');
    }

    file.close();
    return true;
}

QDialog* DictionaryManager::createProgressDialog(QWidget *parent, const QString& text, QProgressBar *progressBar)
{
    QDialog *dlgProgress = new QDialog(parent);
    dlgProgress->setAttribute(Qt::WA_DeleteOnClose);
    dlgProgress->setWindowFlags(dlgProgress->windowFlags() ^ (Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint));
    dlgProgress->setWindowTitle(i18n("Importing Dictionary"));
    dlgProgress->setModal(true);

    QVBoxLayout *layout = new QVBoxLayout;
    QLabel *label = new QLabel(text);
    label->setWordWrap(true);

    //QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    //connect(buttonBox, SIGNAL(rejected()), dlgProgress, SLOT(reject()));

    layout->addWidget(label);
    layout->addWidget(progressBar);
    //layout->addWidget(buttonBox);

    dlgProgress->setLayout(layout);

    return dlgProgress;
}

int DictionaryManager::importFromCsv(const QString& fileName, QWidget *parent)
{
    if (!m_database.isValid()) {
        qDebug() << "Database not open";
        return -1;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Coulnd't open the file in read only mode";
        return -1;
    }

    // Setup a dialog to indicate progress
    QProgressBar *progressBar = new QProgressBar;
    QDialog *dlgProgress = createProgressDialog(parent,
                           i18n("Importing word clue pairs from '%1' to the database.\nPlease wait.",
                                fileName), progressBar);
    dlgProgress->show();

    // Get entry count before inserting to calculate how many entries
    // have been added at the end
    int entryCountBefore = itemsCount();

    QTextStream stream(&file);

    // Read field order
    QStringList fieldNames = stream.readLine().split(';');
    // Remove surrounding double quotes
    for (int i = 0; i < fieldNames.count(); ++i) {
        if (fieldNames[i].startsWith('\"') && fieldNames[i].endsWith('\"'))
            fieldNames[ i ] = fieldNames[ i ].mid(1, fieldNames[i].length() - 2);
    }
    if (!fieldNames.contains("word")) {
        qDebug() << "Field name 'word' not included:" << fieldNames;
        dlgProgress->close();
        return -1;
    }
    // Only allow alphanumerical characters as field names (and prevent SQL injection).
    foreach(const QString & fieldName, fieldNames) {
        if (fieldName.contains(QRegExp("[^A-Z0-9]", Qt::CaseInsensitive))) {
            qDebug() << "Field name contains disallowed characters:" << fieldName;
            dlgProgress->close();
            return -1;
        }
    }

    QString beginSqlQuery = QString("INSERT INTO dictionary (%1) VALUES ").arg(fieldNames.join(","));
    QStringList sqlInserts;
    int counter = 0;
    while (!stream.atEnd()) {
        QStringList fields; // = stream.readLine().split( ";" );

        QString line = stream.readLine();
        QString origLine = line;
        int pos;
        while ((pos = line.indexOf(';')) != -1) {
            QString field;
            // First double quote that is the end of an odd number of double quotes (also 1 double quote)
            QRegExp rx("([^\"](\"\")*\"$|[^\"](\"\")*\"[^\"])");
            int posNextOddQuotes = line.indexOf(rx, 1);
            posNextOddQuotes += rx.matchedLength() - 2;
            bool startsWithOddDoubleQuotes = line.startsWith('\"') && line.indexOf(QRegExp("^(\"\")*\"[^\"]"), 1) == -1;
            qDebug() << line << posNextOddQuotes << startsWithOddDoubleQuotes << rx.matchedLength();
            if (startsWithOddDoubleQuotes && posNextOddQuotes > pos) {
                field = line.mid(1, posNextOddQuotes - 1);
                line.remove(0, posNextOddQuotes + 1);
            } else if (startsWithOddDoubleQuotes && posNextOddQuotes == pos - 1) {
                field = line.mid(1, posNextOddQuotes - 1);
                line.remove(0, pos + 1);
            } else {
                field = line.left(pos);
                line.remove(0, pos + 1);
            }
            fields << field.replace("\"\"", "\"");
        }

        QString field;
        QRegExp rx("([^\"](\"\")*\"$|[^\"](\"\")*\"[^\"])");
        int posNextOddQuotes = line.indexOf(rx, 1);
        posNextOddQuotes += rx.matchedLength() - 2;
//  int posQuotes = line.indexOf( QRegExp("[^\"](?:\"\")*(\")"), 1 ) + 1;
        bool startsWithOddDoubleQuotes = line.startsWith('\"') && line.indexOf(QRegExp("^(\"\")*\"[^\"]"), 1) == -1;
        if (startsWithOddDoubleQuotes && (posNextOddQuotes > pos || posNextOddQuotes == pos - 1))
            field = line.mid(1, posNextOddQuotes - 1);
        else
            field = line;
        fields << field.replace("\"\"", "\"");

        if (fields.count() != fieldNames.count()) {
            qDebug() << "Line contains a number of fields that is different from the field count in the first line"
                     << origLine << ", field count is" << fields.count() << "but should be" << fieldNames.count();
            continue; // Skip lines with a field count other than the given field names in the first line
        }

        for (int i = 0; i < fields.count(); ++i)
            fields[ i ] = QString("'%1'").arg(fields[i].replace('\'', "\\'"));
        sqlInserts << QString("(%1)").arg(fields.join(","));

        // Insert chunks of 1000 entries into the database
        ++counter;
        if (counter % 100 == 0)
            progressBar->setValue(100 * file.pos() / file.size());
        if (counter > 1000) {
            counter = 0;

            QSqlQuery query = m_database.exec(beginSqlQuery + sqlInserts.join(", "));
            sqlInserts.clear();

            //QApplication::processEvents();
        }
    }
    file.close();

    // Insert the remaining entries (< 1000)
    if (!sqlInserts.isEmpty()) {
        QSqlQuery query = m_database.exec(beginSqlQuery + sqlInserts.join(", "));
        if (query.lastError().isValid())
            qDebug() << query.lastError();
    }

    dlgProgress->close();

    return itemsCount() - entryCountBefore;
}

int DictionaryManager::importFromDictionary(const QString& fileName, QWidget *parent)
{
    if (!m_database.isValid())
        return 0;

    int entryCountBefore = itemsCount();

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return 0;

    //--- Progress bar ---
    QProgressBar *progressBar = new QProgressBar;
    QDialog *dlgProgress = createProgressDialog(parent,
                           i18n("Adding words from the dictionary '%1' to the database.\nPlease wait.",
                                fileName), progressBar);
    dlgProgress->show();
    //--------------------

    QTextStream textStream(&file);
    QList<QPair<QString, int>> entry;
    int counter = 0;

    QVariantList wordsValues;

    while (!textStream.atEnd()) {
        QString word = textStream.readLine(MAX_WORD_LENGTH);

        int score = 0;

        // Extract score if it's contained in the dictionary (eg. "word +10" scores the word with +10)
        QRegExp rx("((?:\\+|-)\\d+\\.?\\d*)$");
        int posScore = rx.indexIn(word);
        if (posScore != -1) {
            score = rx.cap().toInt();
            word = word.left(posScore);
        }

        CrosswordAnswerValidator::fix(word);
        if (word.length() > 1) {

            progressBar->setValue(100 * file.pos() / file.size());
            ++counter;

            entry << QPair<QString, int>(word, score);

            wordsValues << word;

            if (counter > 1000) {
                counter = 0;

                m_database.transaction();
                foreach (auto e, entry) {
                   QSqlQuery q(m_database);
                   q.prepare("INSERT INTO dictionary(word, score) VALUES (?, ?)");
                   q.bindValue(0, e.first);
                   q.bindValue(1, e.second);
                   q.exec();
                }
                m_database.commit();

                entry.clear();
            }
        }
        //QApplication::processEvents();
    }
    file.close();

    if (counter > 0) {
        m_database.transaction();
        foreach (auto e, entry) {
            QSqlQuery q(m_database);
            q.prepare("INSERT INTO dictionary(word, score) VALUES (?, ?)");
            q.bindValue(0, e.first);
            q.bindValue(1, e.second);
            if (!q.exec()) {
                qDebug() << q.lastError();
            }
        }
        m_database.commit();
    }

    m_model->sort(1, Qt::AscendingOrder);   // Sort by word, it does a select() too

    dlgProgress->close();

    return itemsCount() - entryCountBefore;
}

bool DictionaryManager::isEmpty()
{
    return itemsCount() == 0;
}

int DictionaryManager::itemsCount()
{
    if (!m_database.isValid())
        return 0;

    QSqlQuery query = m_database.exec("SELECT COUNT(*) FROM dictionary");
    if (query.next())
        return query.value(0).toInt();
    else
        return 0;
}

int DictionaryManager::importFromCrosswords(const QStringList& fileNames, QWidget *parent)
{
    if (!m_database.isValid())
        return 0;

    // Setup a dialog to indicate progress
    QProgressBar *progressBar = new QProgressBar;
    QDialog *dlgProgress = createProgressDialog(parent,
                           i18n("Adding words from %1 crossword files to the database.\nPlease wait.",
                                fileNames.count()), progressBar);
    dlgProgress->show();

    // Get entry count before inserting to calculate how many entries
    // have been added at the end
    int entryCountBefore = itemsCount();

    // Read each file
    QString errorString;
    QString beginSqlQuery = "INSERT INTO dictionary (word, clue) VALUES ";
    QStringList sqlInserts;
    int counter = 0, currentFileNr = 0;
    foreach(const QString & fileName, fileNames) {
        ++currentFileNr;

        KrossWord krossWord;
        qDebug() << "Reading crossword" << fileName;
        if (!krossWord.read(KUrl(fileName), &errorString)) {
            qDebug() << "Error reading" << fileName << errorString;
            emit errorExtractedEntriesFromCrossword(fileName, errorString);
        } else {
            // Read each clue
            int addedEntries = 0;
            Crossword::ClueCellList clues = krossWord.clues();
            foreach(ClueCell * clue, clues) {
                sqlInserts << QString("('%1', '%2')").arg(clue->correctAnswer())
                           .arg(clue->clue().replace('\'', "\\'"));   // Escape single quotes
                ++counter;
            }

            progressBar->setValue(100 * (float)currentFileNr / (float)fileNames.count());

            // Insert chunks of 1000 entries into the database
            if (counter > 1000) {
                counter = 0;

                m_database.transaction();
                foreach(QString s, sqlInserts) {
                    QSqlQuery query = m_database.exec(beginSqlQuery + s);
                }
                m_database.commit();

                sqlInserts.clear();

                //QApplication::processEvents();
            }

            emit extractedEntriesFromCrossword(fileName, addedEntries);
        }
    }

    // Insert the remaining entries (< 1000)
    if (!sqlInserts.isEmpty()) {
        m_database.transaction();
        foreach(QString s, sqlInserts) {
            QSqlQuery query = m_database.exec(beginSqlQuery + s);
        }
        m_database.commit();
    }

    m_model->sort(1, Qt::AscendingOrder);   // Sort by word, it does a select() too

    dlgProgress->close();

    return itemsCount() - entryCountBefore;
}

bool DictionaryManager::clearDatabase()
{
    if (!m_database.isValid())
        return false;

    // delete and recreate the table is faster then delete items
    QSqlQuery query = m_database.exec("DROP TABLE dictionary");
    createTable();

    m_model->select();

    return true;
}
