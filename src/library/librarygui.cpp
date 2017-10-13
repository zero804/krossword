/*
*   Copyright 2010 Friedrich Pülz <fpuelz@gmx.de>
*   Copyright 2014 Andrea Barazzetti <andreadevsrv@gmail.com>
*   Copyright 2014 Giacomo Barazzetti <giacomosrv@gmail.com>
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

#include "librarygui.h"

#include "krosswordpuzzle.h"
#include "krossworddocument.h"
#include "io/krosswordxmlreader.h"
#include "htmldelegate.h"
#include "settings.h"
#include "dialogs/createnewcrossworddialog.h"

#include <QDebug>

#include <KMessageBox>
//#include <KIO/CopyJob>
#include <KActionCollection>
#include <KIO/PreviewJob>
#include <KJobWidgets>

#include <QStandardPaths>
#include <QMenuBar>
#include <QFileDialog>


LibraryGui::LibraryGui(KrossWordPuzzle* parent) : KXmlGuiWindow(parent, Qt::WindowFlags()),
      m_mainWindow(parent),
      m_libraryTree(new QTreeView()),
      m_libraryDelegate(nullptr),
      m_libraryModel(new LibraryManager(this)),
      m_downloadPreviewJob(nullptr),
      m_downloadCrosswordsDlg(nullptr)
{
    QString libraryDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + "library";
    QDir dir;
    dir.mkpath(libraryDir);
    m_libraryModel->setRootPath(libraryDir);

    if (!m_libraryDelegate) {
        m_libraryDelegate = new HtmlDelegate(this);
        m_libraryTree->setItemDelegate(m_libraryDelegate);
    }

    m_libraryTree->setModel(m_libraryModel);
    m_libraryTree->setRootIndex(m_libraryModel->index(libraryDir));

    m_libraryTree->setSortingEnabled(false);
    m_libraryTree->sortByColumn(0, Qt::AscendingOrder);

    m_libraryTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_libraryTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_libraryTree->setDragDropMode(QAbstractItemView::InternalMove);
    m_libraryTree->setAlternatingRowColors(true);
    m_libraryTree->setIconSize(QSize(64, 64));
    m_libraryTree->setAnimated(true);
    m_libraryTree->setAutoScroll(true);
    m_libraryTree->setAllColumnsShowFocus(true);

    m_libraryTree->header()->hideSection(1); //hide size column
    m_libraryTree->header()->hideSection(2); //hide type
    m_libraryTree->header()->setSectionResizeMode(QHeaderView::Stretch);

    setObjectName("library");
    setAutoSaveSettings(QLatin1String("LibraryWindow"), false);

    setCentralWidget(m_libraryTree);

    setupActions();
    setupGUI(ToolBar | /*Keys | */Save | Create, "krossword_library_ui.rc");
    menuBar()->hide(); // because it will be exposed as needed in the mainwindow

    connect(m_libraryTree, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(libraryItemDoubleClicked(QModelIndex)));
    connect(m_libraryTree->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(libraryCurrentChanged(QModelIndex, QModelIndex))); // to keep updated the available actions

    // select last opened crossword
    QString lastCrossword = Settings::lastCrossword();
    m_libraryModel->setOnDirectoryLoadedFunction([this, lastCrossword]() {
        this->m_libraryTree->setSortingEnabled(true);
        this->m_libraryTree->setCurrentIndex(this->m_libraryModel->index(lastCrossword));

        m_libraryModel->clearOnDirectoryLoadedFunction();
    });
}

QTreeView* LibraryGui::libraryTree() const
{
    return m_libraryTree;
}

const char* LibraryGui::actionName(LibraryGui::Action actionEnum) const
{
    switch (actionEnum) {
    case Library_Add:
        return "library_add";
    case Library_Export:
        return "library_export";
    case Library_Download:
        return "library_download";
    case Library_Delete:
        return "library_delete";
    case Library_NewFolder:
        return "library_new_folder";
    case Library_CreateCrossword:
        return "library_create_crossword";

    default:
        qWarning() << "Action enumerable not handled in switch" << actionEnum;
        return "";
    }
}

LibraryManager* LibraryGui::libraryManager() const
{
    return m_libraryModel;
}

//======================================================

void LibraryGui::libraryAddCrossword(const QUrl &url, const QString &folder)
{
    QString crosswordUrl;
    LibraryManager::E_ERROR_TYPE errorCode = m_libraryModel->addCrossword(url, crosswordUrl, folder);

    if (errorCode == LibraryManager::E_ERROR_TYPE::Succeeded) {
        // Select new crossword in the tree view
        QModelIndex index = m_libraryModel->index(crosswordUrl);
        if (index.isValid()) {
            m_libraryTree->setCurrentIndex(index);
            m_libraryTree->scrollTo(index);
        }
    } else {
        if (errorCode == LibraryManager::E_ERROR_TYPE::ReadError)
            KMessageBox::error(this, i18n("The crossword couldn't be imported to the library."));
        else if (errorCode == LibraryManager::E_ERROR_TYPE::WriteError)
            KMessageBox::error(this, i18n("The crossword couldn't be written to the library folder."));
    }
}

//======================================================

void LibraryGui::downloadProviderChanged(int index)
{
    QListWidgetItem *item;

    ui_download.crosswords->clear();

    DownloadProvider provider = static_cast<DownloadProvider>(ui_download.providers->itemData(index).toInt());
    switch (provider) {

    case JonesinCrosswords:
        getDownloadCrosswordItems("http://herbach.dnsalias.com/Jonesin/jz%1.puz", QDate(2008, 6, 5), QDate::currentDate(), 7);
        break;

    case WallStreetJournal:
        getDownloadCrosswordItems("http://mazerlm.home.comcast.net/wsj%1.puz", QDate(2009, 1, 2), QDate(2012, 12, 28), 7);
        getDownloadCrosswordItems("http://herbach.dnsalias.com/wsj/wsj%1.puz", QDate(2013, 1, 4), QDate::currentDate(), 7);
        break;

    case ChronicleHigherEducation:
        getDownloadCrosswordItems("http://chronicle.com/items/biz/puzzles/20%1.puz", QDate(2009, 10, 9), QDate::currentDate(), 7);
        break;

    case CrossNerd:
        item = new QListWidgetItem(QString(i18n("The Cross Nerd current crossword")));
        item->setData(Qt::UserRole, "http://crossexamination.info/CN_current.puz");
        ui_download.crosswords->addItem(item);
        break;

    case SwearCrossword:
        getDownloadCrosswordItems("http://wij.theworld.com/puzzles/dailyrecord/DR%1.puz", QDate(2011, 1, 7), QDate(2013, 12, 27), 7);
        break;

    case ChrisWords:
        for (int i = 1; i <= 5; ++i) {
            item = new QListWidgetItem(QString(i18n("Crossword %1")).arg(i));
            item->setData(Qt::UserRole, QString("https://dl.dropboxusercontent.com/u/91385912/PUZ00%1.puz").arg(i));
            ui_download.crosswords->addItem(item);
        }
        for (int i = 6; i <= 18; ++i) {
            item = new QListWidgetItem(QString(i18n("Crossword %1")).arg(i));
            item->setData(Qt::UserRole, QString("https://dl.dropboxusercontent.com/u/91385912/PUZ0%1/PUZ0%1.puz").arg(i, 2, 10, QChar('0')));
            ui_download.crosswords->addItem(item);
        }
        break;

    case Motscroisesch:
        for (int i = 1; i <= 18; ++i) {
            item = new QListWidgetItem(QString(i18n("Crossword %1")).arg(i));
            item->setData(Qt::UserRole, QString("http://www.mots-croises.ch/Grilles/HC/HC-00%1.puz").arg(i, 2, 10, QChar('0')));
            ui_download.crosswords->addItem(item);
        }
        break;
    }
}

void LibraryGui::downloadCurrentCrosswordChanged(QListWidgetItem* current, QListWidgetItem* previous)
{
    Q_UNUSED(previous);

    if (!current)
        return;

    ui_download.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    ui_download.preview->setPixmap(QPixmap());
    ui_download.preview->setEnabled(false);
    ui_download.preview->setText(i18n("Loading..."));

    KIO::StoredTransferJob *downloadJob = KIO::storedGet(current->data(Qt::UserRole).toUrl());
    KJobWidgets::setWindow(downloadJob, m_downloadCrosswordsDlg);
    connect(downloadJob, SIGNAL(result(KJob*)), this, SLOT(downloadCrosswordResult(KJob*)));
    downloadJob->start();
}

void LibraryGui::downloadCrosswordResult(KJob *job)
{
    if (job->error()) {
        ui_download.preview->setText(i18n("Download error: ") + job->errorString());
    } else {
        KIO::StoredTransferJob* downJob = static_cast<KIO::StoredTransferJob*>(job);

        m_downloadedCrossword.reset(new QTemporaryFile); //m_downloadedCrossword = QScopedPointer<QTemporaryFile>(new QTemporaryFile);
        if (m_downloadedCrossword->open()) {
            m_downloadedCrossword->write(downJob->data());
            m_downloadedCrossword->close();
        }

        KFileItem crossword(QUrl::fromLocalFile(m_downloadedCrossword->fileName()), "application/x-acrosslite-puz");
        QStringList plugin("crosswordthumbnail");
        m_downloadPreviewJob = new KIO::PreviewJob(KFileItemList() << crossword, QSize(256, 256), &plugin);
        connect(m_downloadPreviewJob, SIGNAL(gotPreview(KFileItem, QPixmap)), this, SLOT(downloadPreviewJobGotPreview(KFileItem, QPixmap)));
        connect(m_downloadPreviewJob, SIGNAL(failed(KFileItem)), this, SLOT(downloadPreviewJobFailed(KFileItem)));
        m_downloadPreviewJob->start();
    }
}

void LibraryGui::downloadPreviewJobGotPreview(const KFileItem& fi, const QPixmap& pix)
{
    Q_UNUSED(fi);

    if (m_downloadCrosswordsDlg && ui_download.preview) {
        ui_download.preview->setPixmap(pix);
        ui_download.preview->setEnabled(true);

        ui_download.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void LibraryGui::downloadPreviewJobFailed(const KFileItem& fi)
{
    qDebug() << "Preview job failed for file" << fi.url().url(QUrl::PreferLocalFile);

    if (m_downloadCrosswordsDlg && ui_download.preview) {
        ui_download.preview->setText(i18n("Crossword not valid"));
    }
}

void LibraryGui::libraryItemDoubleClicked(const QModelIndex &index)
{
    libraryOpenItem(index);
}

void LibraryGui::libraryOpenItem(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    if (!m_libraryModel->isDir(index))
        m_mainWindow->loadFile(QUrl::fromLocalFile(index.data(QFileSystemModel::FilePathRole).toString()));
}

void LibraryGui::libraryCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);

    bool isCrosswordSelected = !(m_libraryModel->isDir(current));
    action(actionName(Library_Export))->setEnabled(isCrosswordSelected);

    action(actionName(Library_Delete))->setEnabled(current.isValid());
}

void LibraryGui::libraryAddSlot()
{
    QUrl url = QFileDialog::getOpenFileUrl(this,
                                           QString(),
                                           QUrl(),
                                           "Crosswords (*.kwp *.kwpz *.puz)");

    if (!url.isEmpty())
        libraryAddCrossword(url);
}

// MISSING kwp, kwpz (and puz?) EXPORT
void LibraryGui::libraryExportSlot()
{
    QModelIndex index = m_libraryTree->currentIndex();

    if (!index.isValid())
        return;

    // should not be the case because of libraryCurrentChanged filtering actions
    if (m_libraryModel->isDir(index))
        KMessageBox::information(this, i18n("Can't export whole directories. Please select a crossword."));

    Crossword::KrossWord krossWord;
    QString errorString;
    QString filePath = index.data(QFileSystemModel::FilePathRole).toString();
    if (!krossWord.read(QUrl::fromLocalFile(filePath), &errorString)) {
        KMessageBox::error(this, i18n("There was an error opening the crossword.\n%1", errorString));
    } else {
        QString fileName = QFileDialog::getSaveFileName(
                    this,
                    i18n("Export"),
                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                    "Pdf (*.pdf);;Postscript (*.ps);;Images (*.png *.jpg *.jpeg)");

        if (fileName.isEmpty())
            return;

        QString fileSuffix = QFileInfo(fileName).suffix();
        if (fileSuffix.compare("pdf", Qt::CaseInsensitive) == 0 || fileSuffix.compare("ps", Qt::CaseInsensitive) == 0) {
            QPrinter printer;
            printer.setCreator("Krossword");
            printer.setDocName(fileName);   // TODO: set krossword title if available

            printer.setOutputFileName(fileName);
            PdfDocument document(&krossWord, &printer);
            document.print();
        } else if (fileSuffix.compare("png", Qt::CaseInsensitive) == 0
                   || fileSuffix.compare("jpeg", Qt::CaseInsensitive) == 0
                   || fileSuffix.compare("jpg", Qt::CaseInsensitive) == 0) {
            QPointer<QDialog> exportToImageDlg = new QDialog(this);
            exportToImageDlg->setWindowTitle(i18n("Export Settings"));
            exportToImageDlg->setModal(true);

            ui_export_to_image.setupUi(exportToImageDlg);

            if (exportToImageDlg->exec() == QDialog::Rejected) {
                delete exportToImageDlg;
                return;
            }

            QPixmap pix = krossWord.toPixmap(QSize(ui_export_to_image.width->value(), ui_export_to_image.height->value()));
            int quality = ui_export_to_image.quality->value();
            delete exportToImageDlg;

            if (!pix.save(fileName, 0, quality)) {
                qDebug() << "Export failed:" << fileName;
                KMessageBox::error(this, i18n("Couldn't export the image to the specified file."));
            }
        }
    }
}

void LibraryGui::libraryDownloadSlot()
{
    m_downloadCrosswordsDlg = new QDialog(m_mainWindow);
    m_downloadCrosswordsDlg->setWindowTitle(i18n("Download Crossword To Library"));
    ui_download.setupUi(m_downloadCrosswordsDlg);
    ui_download.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    foreach(const DownloadProvider & provider, allDownloadProviders()) {
        switch (provider) {

        case JonesinCrosswords:
            ui_download.providers->addItem(i18n("Matt Jones (thursdays)"), static_cast<int>(provider));
            break;

        case WallStreetJournal:
            ui_download.providers->addItem(i18n("Wall Street Journal by Mike Shenk (fridays)"), static_cast<int>(provider));
            break;

        case ChronicleHigherEducation:
            ui_download.providers->addItem(i18n("The Chronicle of Higher Education (fridays)"), static_cast<int>(provider));
            break;

        case CrossNerd:
            ui_download.providers->addItem(i18n("The Cross Nerd (tuesdays)"), static_cast<int>(provider));
            break;

        case SwearCrossword:
            ui_download.providers->addItem(i18n("I Swear Crossword by Victor Fleming (fridays)"), static_cast<int>(provider));
            break;

        case ChrisWords:
            ui_download.providers->addItem(i18n("ChrisWords.com"), static_cast<int>(provider));
            break;

        case Motscroisesch:
            ui_download.providers->addItem(i18n("Mots-croisés.ch free French crosswords"), static_cast<int>(provider));
            break;
        }
    }

    ui_download.providers->setCurrentIndex(0);

    connect(ui_download.providers, SIGNAL(currentIndexChanged(int)), this, SLOT(downloadProviderChanged(int)));
    connect(ui_download.crosswords,
            SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
            this, SLOT(downloadCurrentCrosswordChanged(QListWidgetItem*, QListWidgetItem*)));

    downloadProviderChanged(0);

    // search for all the directories and add them (the library root too) as available download dir
    QFileInfoList directories = m_libraryModel->getFoldersPath();
    QHash<int, QString> directoriesIndex;

    int i = 0;
    foreach(QFileInfo fi, directories) {
        directoriesIndex.insert(i, fi.filePath());
        ++i;
        if(fi.filePath() == m_libraryModel->rootPath()) {
            ui_download.targetDirectory->addItem(QIcon::fromTheme(QStringLiteral("folder")), i18n("Library"));
        } else {
            ui_download.targetDirectory->addItem(QIcon::fromTheme(QStringLiteral("folder")), fi.fileName());
        }
    }

    // maybe restore here the directory previously selected by the user

    if (m_downloadCrosswordsDlg->exec() == QDialog::Accepted) {
        QString url = m_downloadedCrossword->fileName();
        if (!url.isEmpty()) {
            QString downloadDir = directoriesIndex.value(ui_download.targetDirectory->currentIndex());
            libraryAddCrossword(QUrl::fromLocalFile(url), downloadDir);
        }
    }

    delete m_downloadCrosswordsDlg;
    m_downloadCrosswordsDlg = nullptr;
}

void LibraryGui::libraryDeleteSlot()
{
    QModelIndex index = m_libraryTree->currentIndex();
    if (index.isValid()) {
        if (m_libraryModel->isDir(index)) {
            int result = KMessageBox::warningContinueCancel(this,
                         i18n("This will permanently delete the selected directory and all contained "
                              "crosswords from the library. The operation can't be undone!"), QString(),
                         KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "dont_show_delete_library_directory_confirmation");
            if (result == KMessageBox::Cancel)
                return;

            bool error = m_libraryModel->remove(index);
            if(error == false)
                KMessageBox::error(this, i18n("Couldn't remove the directory from the library.\nYou need root privileges to remove directories which belong to the global library."));

        } else {
            int result = KMessageBox::warningContinueCancel(this,
                         i18n("This will permanently delete the selected crossword from the library.\nThe operation can't be undone!"), QString(),
                         KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "dont_show_delete_library_crossword_confirmation");
            if (result == KMessageBox::Cancel)
                return;

            bool error = m_libraryModel->remove(index);
            if(error == false)
                KMessageBox::error(this, i18n("Couldn't remove the crossword from the library."));
        }
    }
}

void LibraryGui::libraryNewFolderSlot()
{
    QPointer<QDialog> dlg = new QDialog(this);
    QLineEdit *newFolderName = new QLineEdit(dlg);
    newFolderName->setClearButtonEnabled(true);
    newFolderName->setPlaceholderText("Insert the name of the new library folder");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, Qt::Horizontal, dlg);
    connect(buttonBox, SIGNAL(accepted()), dlg, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), dlg, SLOT(reject()));

    QVBoxLayout *layout = new QVBoxLayout(dlg);
    layout->addWidget(newFolderName);
    layout->addWidget(buttonBox);
    dlg->setLayout(layout);

    dlg->setWindowTitle(i18n("New Library Folder"));

    newFolderName->setFocus();

    if (dlg->exec() == QDialog::Accepted) {
        QString folderName = newFolderName->text();
        if (!folderName.isEmpty()) {
            if (!m_libraryModel->newFolder(folderName)) {
                KMessageBox::information(this, i18n("The folder already exists in the library."));
            }
        }
    }

    delete dlg;
}

void LibraryGui::libraryCreateCrosswordSlot()
{
    QPointer<CreateNewCrosswordDialog> dialog = new CreateNewCrosswordDialog(this);

    if (dialog->exec() == QDialog::Accepted) {
        if (dialog->useTemplate()) {
            m_mainWindow->createNewCrossWordFromTemplate(dialog->templateFilePath(),
                    dialog->title(),
                    dialog->author(),
                    dialog->copyright(),
                    dialog->notes());
        } else {
            m_mainWindow->createNewCrossWord(dialog->crosswordTypeInfo(),
                                             dialog->crosswordSize(),
                                             dialog->title(), dialog->author(),
                                             dialog->copyright(), dialog->notes());
        }
    }

    delete dialog;
}

//======================================================

void LibraryGui::setupActions()
{
    // Library actions
    QAction *libraryCreateCrosswordAction = new QAction(QIcon::fromTheme(QStringLiteral("document-new")), i18nc("@action:intoolbar", "&Create Crossword..."), this);
    libraryCreateCrosswordAction->setToolTip(i18n("Create a new crossword"));
    actionCollection()->addAction(actionName(Library_CreateCrossword), libraryCreateCrosswordAction);
    connect(libraryCreateCrosswordAction, SIGNAL(triggered()), this, SLOT(libraryCreateCrosswordSlot()));

    QAction *libraryNewFolderAction = new QAction(QIcon::fromTheme(QStringLiteral("folder-new")), i18nc("@action:intoolbar", "New &Folder..."), this);
    libraryNewFolderAction->setToolTip(i18n("Create a new folder in the library"));
    actionCollection()->addAction(actionName(Library_NewFolder), libraryNewFolderAction);
    connect(libraryNewFolderAction, SIGNAL(triggered()), this, SLOT(libraryNewFolderSlot()));

    QAction *libraryDownloadAction = new QAction(QIcon::fromTheme(QStringLiteral("download")), i18nc("@action:intoolbar", "&Download..."), this);
    libraryDownloadAction->setToolTip(i18n("Download a crossword and add it to the library"));
    actionCollection()->addAction(actionName(Library_Download), libraryDownloadAction);
    connect(libraryDownloadAction, SIGNAL(triggered()), this, SLOT(libraryDownloadSlot()));

    QAction *libraryAddAction = new QAction(QIcon::fromTheme(QStringLiteral("list-add")), i18nc("@action:intoolbar", "&Add..."), this);
    libraryAddAction->setToolTip(i18n("Add a crossword to the library"));
    actionCollection()->addAction(actionName(Library_Add), libraryAddAction);
    connect(libraryAddAction, SIGNAL(triggered()), this, SLOT(libraryAddSlot()));

    QAction *libraryDeleteAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@action:intoolbar", "&Delete..."), this);
    libraryDeleteAction->setToolTip(i18n("Delete the selected crossword from the library"));
    actionCollection()->addAction(actionName(Library_Delete), libraryDeleteAction);
    connect(libraryDeleteAction, SIGNAL(triggered()), this, SLOT(libraryDeleteSlot()));

    QAction *libraryExportAction = new QAction(QIcon::fromTheme(QStringLiteral("document-export")), i18nc("@action:intoolbar", "&Export..."), this);
    libraryExportAction->setToolTip(i18n("Export the selected crossword from the library"));
    actionCollection()->addAction(actionName(Library_Export), libraryExportAction);
    connect(libraryExportAction, SIGNAL(triggered()), this, SLOT(libraryExportSlot()));
}

void LibraryGui::getDownloadCrosswordItems(const QString& rawUrl, const QDate& startDate, const QDate& endDate, int dayOffset)
{
    QDate date = startDate;
    for (int year = startDate.year(); year <= endDate.year(); ++year) {
        int curMonth = date.month();
        while (date.year() == year && date < endDate) {
            QListWidgetItem *item = new QListWidgetItem(QString(i18n("%1", QLocale().toString(date)))); //KLocale::global()->formatDate(date, KLocale::FancyLongDate))));
            item->setData(Qt::UserRole, rawUrl.arg(date.toString("yyMMdd")));
            ui_download.crosswords->addItem(item);

            date = date.addDays(dayOffset);
            if (date.month() != curMonth)
                curMonth = date.month();
        }
    }
}

QList<LibraryGui::DownloadProvider> LibraryGui::allDownloadProviders()
{
    return QList<DownloadProvider>() << JonesinCrosswords << WallStreetJournal << ChronicleHigherEducation << CrossNerd << SwearCrossword << ChrisWords << Motscroisesch;
}
