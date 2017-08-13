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

#include "dictionarydialog.h"
#include "extendedsqltablemodel.h"
#include "htmldelegate.h"
#include "dictionary.h"

#include <QPropertyAnimation>

#include <QTimer>
#include <KStandardDirs>
#include <KFileDialog>


DictionaryDialog::DictionaryDialog(KrosswordDictionary* dictionary, QWidget* parent)
    : QDialog(parent), m_dictionary(dictionary), m_infoMessage(0)
{
    setWindowTitle(i18n("Dictionary"));
    ui_dictionaries.setupUi(this);;
    setWindowIcon(QIcon::fromTheme(QStringLiteral("crossword-dictionary")));
    setModal(true);

    ui_dictionaries.extractFromLibrary->setIcon(QIcon::fromTheme(QStringLiteral("extract-from-library")));
    ui_dictionaries.extractFromCrosswords->setIcon(QIcon::fromTheme(QStringLiteral("extract-from-crosswords")));
    ui_dictionaries.addWordsFromDictionary->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    ui_dictionaries.addEntry->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    ui_dictionaries.removeEntries->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    ui_dictionaries.clear->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear")));
    ui_dictionaries.importFromCSV->setIcon(QIcon::fromTheme(QStringLiteral("document-import")));
    ui_dictionaries.exportToCSV->setIcon(QIcon::fromTheme(QStringLiteral("document-export")));

    m_dbTable = m_dictionary->createModel();
    m_dbTable->select();

    ui_dictionaries.tableDictionary->setModel(m_dbTable);
    ui_dictionaries.tableDictionary->hideColumn(0);   // Hide id
    ui_dictionaries.tableDictionary->hideColumn(3);   // Hide score
    ui_dictionaries.tableDictionary->hideColumn(4);   // Hide language
    ui_dictionaries.tableDictionary->setItemDelegateForColumn(1, new CrosswordAnswerDelegate);  // Set delegate for column 'answer'
    ui_dictionaries.removeEntries->setDisabled(true);

    connect(ui_dictionaries.addEntry, SIGNAL(clicked()),
            this, SLOT(addEntryClicked()));
    connect(ui_dictionaries.extractFromCrosswords, SIGNAL(clicked()),
            this, SLOT(addDictionaryFromCrosswordsClicked()));
    connect(ui_dictionaries.extractFromLibrary, SIGNAL(clicked()),
            this, SLOT(addDictionaryFromLibraryClicked()));
    connect(ui_dictionaries.addWordsFromDictionary, SIGNAL(clicked()),
            this, SLOT(getWordsFromDictionaryClicked()));
    connect(ui_dictionaries.removeEntries, SIGNAL(clicked()),
            this, SLOT(removeEntriesClicked()));
    connect(ui_dictionaries.clear, SIGNAL(clicked()),
            this, SLOT(clearClicked()));
    connect(ui_dictionaries.importFromCSV, SIGNAL(clicked()),
            this, SLOT(importFromCsvClicked()));
    connect(ui_dictionaries.exportToCSV, SIGNAL(clicked()),
            this, SLOT(exportToCsvClicked()));
    connect(ui_dictionaries.filter, SIGNAL(textChanged(QString)),
            this, SLOT(filterChanged(QString)));
    connect(ui_dictionaries.tableDictionary->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(tableSelectionChanged(QItemSelection, QItemSelection)));
}

void DictionaryDialog::tableSelectionChanged(const QItemSelection& selected,
        const QItemSelection& deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);
    ui_dictionaries.removeEntries->setEnabled(
        !ui_dictionaries.tableDictionary->selectionModel()->selectedIndexes().isEmpty());
}

void DictionaryDialog::addEntryClicked()
{
    int row = m_dbTable->rowCount();
    if (m_dbTable->insertRecord(row, m_dbTable->record())) {
        QModelIndex index = m_dbTable->index(row, 0);
        ui_dictionaries.tableDictionary->setCurrentIndex(index);
        ui_dictionaries.tableDictionary->scrollToBottom();
    }
}

void DictionaryDialog::showInfoMessage(const QString& infoMessage)
{
    if (m_infoMessage)
        m_infoMessage->close();

    m_infoMessage = new QLabel(infoMessage, this);
    m_infoMessage->setAttribute(Qt::WA_DeleteOnClose);
    m_infoMessage->setWindowFlags(Qt::ToolTip);
    m_infoMessage->setFrameShape(QFrame::Box);
    m_infoMessage->setAutoFillBackground(true);
    m_infoMessage->setBackgroundRole(QPalette::ToolTipBase);
    m_infoMessage->setForegroundRole(QPalette::ToolTipText);
    m_infoMessage->setWordWrap(true);
    m_infoMessage->setMaximumWidth(ui_dictionaries.tableDictionary->width() - 10);

    QSize size = m_infoMessage->sizeHint();
    QRect rect = QRect(ui_dictionaries.tableDictionary->mapToGlobal(
                           ui_dictionaries.tableDictionary->rect().bottomLeft() + QPoint(5, -size.height() - 5)), size);

    QRect rectStart = rect.adjusted(0, rect.height(), 0, 0);
    m_infoMessage->setGeometry(rectStart);
    m_infoMessage->show();
    QPropertyAnimation *anim = new QPropertyAnimation(m_infoMessage, "geometry");
    anim->setStartValue(rectStart);
    anim->setEndValue(rect);
    anim->setDuration(250);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    QTimer::singleShot(3000, this, SLOT(hideInfoMessage()));
}

void DictionaryDialog::hideInfoMessage()
{
    if (!m_infoMessage)
        return;

    QRect rectEnd = m_infoMessage->geometry().adjusted(0, m_infoMessage->height(), 0, 0);
    QPropertyAnimation *anim = new QPropertyAnimation(m_infoMessage, "geometry");
    anim->setStartValue(m_infoMessage->geometry());
    anim->setEndValue(rectEnd);
    anim->setDuration(250);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
    QTimer::singleShot(250, m_infoMessage, SLOT(close()));

    m_infoMessage = NULL;
}

void DictionaryDialog::getWordsFromDictionaryClicked()
{
    QUrl startDir;
    if (QDir("/usr/share/dict").exists())
        startDir = QUrl::fromLocalFile("/usr/share/dict");
    else if (QDir("/usr/dict").exists())
        startDir = QUrl::fromLocalFile("/usr/dict");
    else
        startDir = QUrl::fromLocalFile("kfiledialog:///addDictionary");
    QString fileName = KFileDialog::getOpenFileName(startDir, QString(), this);
    if (fileName.isEmpty())
        return;

    int i = m_dictionary->addEntriesFromDictionary(fileName, this);
    showInfoMessage(i18n("%1 entries added from this dictionary: '%2'", i, fileName));

    m_dbTable->select();
}

void DictionaryDialog::addDictionaryFromLibraryClicked()
{
    QStringList libraryFiles = KGlobal::dirs()->findAllResources("appdata", "library/*.kwp?",
                               KStandardDirs::NoDuplicates | KStandardDirs::Recursive);
    int i = m_dictionary->addEntriesFromCrosswords(libraryFiles, this);
    showInfoMessage(i18n("%1 entries added from %2 crosswords", i, libraryFiles.count()));

    m_dbTable->select();
}

void DictionaryDialog::addDictionaryFromCrosswordsClicked()
{
    QStringList fileNames = KFileDialog::getOpenFileNames(QUrl::fromLocalFile(),
                            "application/x-krosswordpuzzle "
                            "application/x-krosswordpuzzle-compressed "
                            "application/x-acrosslite-puz", this);
    if (fileNames.isEmpty())
        return;

    int i = m_dictionary->addEntriesFromCrosswords(fileNames, this);
    showInfoMessage(i18n("%1 entries added from %2 crosswords", i, fileNames.count()));

    m_dbTable->select();
}

void DictionaryDialog::removeEntriesClicked()
{
    QItemSelectionModel *selModel = ui_dictionaries.tableDictionary->selectionModel();
    QModelIndexList selectedRows = selModel->selectedRows();
    if (selectedRows.isEmpty())
        return;

    foreach(const QModelIndex & index, selectedRows)
    m_dbTable->removeRow(index.row());

    if (!m_dbTable->submitAll())
        showInfoMessage(i18n("Couldn't submit removed entries to the database: %1",
                             m_dbTable->lastError().text()));
    else
        showInfoMessage(i18np("%1 entry removed", "%1 entries removed",
                              selectedRows.count()));
}

void DictionaryDialog::clearClicked()
{
    m_dictionary->clearDatabase();
    m_dbTable->select();

    showInfoMessage(i18n("Dictionary cleared"));
}

void DictionaryDialog::exportToCsvClicked()
{
    QString fileName;
    QPointer<KFileDialog> fileDlg = new KFileDialog(QUrl::fromLocalFile(), QString(), this);
    fileDlg->setWindowTitle(i18n("Export To CSV"));
    fileDlg->setMode(KFile::File);
    fileDlg->setOperationMode(KFileDialog::Saving);
    fileDlg->setConfirmOverwrite(true);
    if (fileDlg->exec() == KFileDialog::Accepted)
        fileName = fileDlg->selectedFile();
    delete fileDlg;

    if (fileName.isEmpty())
        return;

    bool result = m_dictionary->exportToCsv(fileName);
    if (!result)
        showInfoMessage(i18n("There was an error while exporting to '%1'", fileName));
    else
        showInfoMessage(i18n("Export to '%1' was successful", fileName));
}

void DictionaryDialog::importFromCsvClicked()
{
    QString fileName = KFileDialog::getOpenFileName(QUrl::fromLocalFile(), QString(),
                       this, i18n("Import From CSV"));
    if (fileName.isEmpty())
        return;

    int i = m_dictionary->importFromCsv(fileName, this);
    if (i == -1)
        showInfoMessage(i18n("There was an error while importing from '%1'", fileName));
    else {
        m_dbTable->select();
        showInfoMessage(i18np("%1 entry imported from '%2'",
                              "%1 entries imported from '%2'", i, fileName));
    }
}

void DictionaryDialog::filterChanged(const QString& filter)
{
    m_dbTable->submitAll(); // submit possible changes
    m_dbTable->setFilter(QString("word LIKE '%%1%'").arg(filter));
}


