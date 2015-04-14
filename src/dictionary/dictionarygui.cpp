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

#include "dictionarygui.h"
#include "dictionarymodel.h"
#include "dictionarymanager.h"

#include "htmldelegate.h"

#include <QPropertyAnimation>

#include <QTimer>
#include <QSqlRecord>
#include <QSqlError>
#include <KStandardDirs>
#include <KFileDialog>


DictionaryGui::DictionaryGui(DictionaryManager* dictionary, QWidget* parent)
    : QDialog(parent),
      m_dictionaryManager(dictionary),
      m_infoMessage(0)
{
    setWindowTitle(i18n("Dictionary"));
    ui_dictionarygui.setupUi(this);;
    setWindowIcon(KIcon("crossword-dictionary"));
    setModal(true);

    ui_dictionarygui.extractFromLibrary->setIcon(KIcon("extract-from-library"));
    ui_dictionarygui.extractFromCrosswords->setIcon(KIcon("extract-from-crosswords"));
    ui_dictionarygui.addWordsFromDictionary->setIcon(KIcon("list-add"));
    ui_dictionarygui.addEntry->setIcon(KIcon("list-add"));
    ui_dictionarygui.removeEntries->setIcon(KIcon("list-remove"));
    ui_dictionarygui.clear->setIcon(KIcon("edit-clear"));
    ui_dictionarygui.importFromCSV->setIcon(KIcon("document-import"));
    ui_dictionarygui.exportToCSV->setIcon(KIcon("document-export"));

    m_dictionaryModel = m_dictionaryManager->getModel();

    ui_dictionarygui.tableDictionary->setModel(m_dictionaryModel);
    ui_dictionarygui.tableDictionary->hideColumn(0);   // Hide id
    ui_dictionarygui.tableDictionary->hideColumn(3);   // Hide score
    ui_dictionarygui.tableDictionary->hideColumn(4);   // Hide language
    ui_dictionarygui.tableDictionary->setItemDelegateForColumn(1, new CrosswordAnswerDelegate);  // Set delegate for column 'answer'
    ui_dictionarygui.removeEntries->setDisabled(true);

    connect(ui_dictionarygui.addEntry, SIGNAL(clicked()),
            this, SLOT(addEntryClicked()));
    connect(ui_dictionarygui.extractFromCrosswords, SIGNAL(clicked()),
            this, SLOT(addDictionaryFromCrosswordsClicked()));
    connect(ui_dictionarygui.extractFromLibrary, SIGNAL(clicked()),
            this, SLOT(addDictionaryFromLibraryClicked()));
    connect(ui_dictionarygui.addWordsFromDictionary, SIGNAL(clicked()),
            this, SLOT(getWordsFromDictionaryClicked()));
    connect(ui_dictionarygui.removeEntries, SIGNAL(clicked()),
            this, SLOT(removeEntriesClicked()));
    connect(ui_dictionarygui.clear, SIGNAL(clicked()),
            this, SLOT(clearClicked()));
    connect(ui_dictionarygui.importFromCSV, SIGNAL(clicked()),
            this, SLOT(importFromCsvClicked()));
    connect(ui_dictionarygui.exportToCSV, SIGNAL(clicked()),
            this, SLOT(exportToCsvClicked()));
    connect(ui_dictionarygui.filter, SIGNAL(textChanged(QString)),
            this, SLOT(filterChanged(QString)));
    connect(ui_dictionarygui.tableDictionary->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this, SLOT(tableSelectionChanged(QItemSelection, QItemSelection)));
}

void DictionaryGui::tableSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);
    ui_dictionarygui.removeEntries->setEnabled(
        !ui_dictionarygui.tableDictionary->selectionModel()->selectedIndexes().isEmpty());
}

void DictionaryGui::addEntryClicked()
{
    int row = m_dictionaryModel->rowCount();
    if (m_dictionaryModel->insertRecord(row, m_dictionaryModel->record())) {
        QModelIndex index = m_dictionaryModel->index(row, 0);
        ui_dictionarygui.tableDictionary->setCurrentIndex(index);
        ui_dictionarygui.tableDictionary->scrollToBottom();
    }
}

void DictionaryGui::showInfoMessage(const QString& infoMessage)
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
    m_infoMessage->setMaximumWidth(ui_dictionarygui.tableDictionary->width() - 10);

    QSize size = m_infoMessage->sizeHint();
    QRect rect = QRect(ui_dictionarygui.tableDictionary->mapToGlobal(
                           ui_dictionarygui.tableDictionary->rect().bottomLeft() + QPoint(5, -size.height() - 5)), size);

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

void DictionaryGui::hideInfoMessage()
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

void DictionaryGui::getWordsFromDictionaryClicked()
{
    KUrl startDir;
    if (QDir("/usr/share/dict").exists())
        startDir = KUrl("/usr/share/dict");
    else if (QDir("/usr/dict").exists())
        startDir = KUrl("/usr/dict");
    else
        startDir = KUrl("kfiledialog:///addDictionary");
    QString fileName = KFileDialog::getOpenFileName(startDir, QString(), this);
    if (fileName.isEmpty()) {
        return;
    }

    int i = m_dictionaryManager->importFromDictionary(fileName, this);
    showInfoMessage(i18n("%1 entries added from this dictionary: '%2'", i, fileName));
}

void DictionaryGui::addDictionaryFromLibraryClicked()
{
    QStringList libraryFiles = KGlobal::dirs()->findAllResources("appdata", "library/*.kwp?",
                               KStandardDirs::NoDuplicates | KStandardDirs::Recursive);
    int i = m_dictionaryManager->importFromCrosswords(libraryFiles, this);
    showInfoMessage(i18n("%1 entries added from %2 crosswords", i, libraryFiles.count()));
}

void DictionaryGui::addDictionaryFromCrosswordsClicked()
{
    QStringList fileNames = KFileDialog::getOpenFileNames(KUrl(),
                            "application/x-krosswordpuzzle "
                            "application/x-krosswordpuzzle-compressed "
                            "application/x-acrosslite-puz", this);
    if (fileNames.isEmpty())
        return;

    int i = m_dictionaryManager->importFromCrosswords(fileNames, this);
    showInfoMessage(i18n("%1 entries added from %2 crosswords", i, fileNames.count()));
}

void DictionaryGui::removeEntriesClicked()
{
    QItemSelectionModel *selModel = ui_dictionarygui.tableDictionary->selectionModel();
    QModelIndexList selectedRows = selModel->selectedRows();
    if (selectedRows.isEmpty()) {
        return;
    }

    foreach(const QModelIndex & index, selectedRows) {
        m_dictionaryModel->removeRow(index.row());
    }

    if (!m_dictionaryModel->submitAll()) {
        showInfoMessage(i18n("Couldn't submit removed entries to the database: %1", m_dictionaryModel->lastError().text()));
    } else {
        showInfoMessage(i18np("%1 entry removed", "%1 entries removed", selectedRows.count()));
    }
}

void DictionaryGui::clearClicked()
{
    m_dictionaryManager->clearDatabase();

    showInfoMessage(i18n("Dictionary cleared"));
}

void DictionaryGui::exportToCsvClicked()
{
    QString fileName;
    QPointer<KFileDialog> fileDlg = new KFileDialog(KUrl(), QString(), this);
    fileDlg->setWindowTitle(i18n("Export To CSV"));
    fileDlg->setMode(KFile::File);
    fileDlg->setOperationMode(KFileDialog::Saving);
    fileDlg->setConfirmOverwrite(true);
    if (fileDlg->exec() == KFileDialog::Accepted)
        fileName = fileDlg->selectedFile();
    delete fileDlg;

    if (fileName.isEmpty())
        return;

    bool result = m_dictionaryManager->exportToCsv(fileName);
    if (!result)
        showInfoMessage(i18n("There was an error while exporting to '%1'", fileName));
    else
        showInfoMessage(i18n("Export to '%1' was successful", fileName));
}

void DictionaryGui::importFromCsvClicked()
{
    QString fileName = KFileDialog::getOpenFileName(KUrl(), QString(),
                       this, i18n("Import From CSV"));
    if (fileName.isEmpty())
        return;

    int i = m_dictionaryManager->importFromCsv(fileName, this);
    if (i == -1)
        showInfoMessage(i18n("There was an error while importing from '%1'", fileName));
    else {
        m_dictionaryModel->select();
        showInfoMessage(i18np("%1 entry imported from '%2'",
                              "%1 entries imported from '%2'", i, fileName));
    }
}

void DictionaryGui::filterChanged(const QString& filter)
{
    m_dictionaryModel->submitAll(); // submit possible changes
    m_dictionaryModel->setFilter(QString("word LIKE '%%1%'").arg(filter));
}
