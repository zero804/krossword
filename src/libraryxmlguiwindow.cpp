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

#include "libraryxmlguiwindow.h"

#include "krosswordpuzzle.h"
#include "krossworddocument.h"
#include "io/krosswordxmlreader.h"
#include "htmldelegate.h"
#include "settings.h"
#include "subfilesystemproxymodel.h"
#include "dialogs/crosswordtypeconfiguredetailsdialog.h"
#include "dialogs/createnewcrossworddialog.h"

#include <QTreeView>
#include <QFileInfo>
#include <QDir>
#include <QTreeWidget>
#include <QStandardItemModel>
#include <QListView>
#include <QStringListModel>

#include <KDebug>
#include <KAction>
#include <KMessageBox>
#include <KFileDialog>
#include <KActionMenu>
#include <KIO/CopyJob>
#include <KColorScheme>
#include <KFileItem>
#include <KMenu>
#include <KMenuBar>
#include <KStatusBar>
#include <KActionCollection>
#include <KUser>
#include <KIO/PreviewJob>
#include <KStandardDirs>
#include <KIO/NetAccess>


LibraryXmlGuiWindow::LibraryXmlGuiWindow(KrossWordPuzzle* parent) : KXmlGuiWindow(parent, Qt::WindowFlags()),
      m_libraryTree(new QTreeView()), m_mainWindow(parent),
      m_dialog(0),
      m_libraryModel(0), m_libraryDelegate(0),
      m_previewJob(0), m_downloadPreviewJob(0)
{
    m_libraryTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_libraryTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_libraryTree->setDragDropMode(QAbstractItemView::InternalMove);
    m_libraryTree->setAlternatingRowColors(true);
    m_libraryTree->setIconSize(QSize(64, 64));
    m_libraryTree->setSortingEnabled(true);
    m_libraryTree->setAnimated(true);
    m_libraryTree->setAllColumnsShowFocus(true);
    m_libraryTree->setWordWrap(true);
    m_libraryTree->header()->setMinimumSectionSize(125);
    m_libraryTree->setWhatsThis(i18n("This is the library view.<br/>You can see all crosswords that are inside the library."));
    m_libraryTree->setContextMenuPolicy(Qt::CustomContextMenu);

    statusBar()->show();
    setHelpMenuEnabled(false);
    setObjectName("library");
    setAutoSaveSettings(QLatin1String("LibraryWindow"), false);

    setCentralWidget(m_libraryTree);

    setupActions();
    setupGUI(StatusBar | ToolBar | /*Keys | */Save | Create, "krosswordpuzzle/krosswordpuzzle_library_ui.rc");
    menuBar()->hide();

    connect(m_libraryTree, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(libraryTreeContextMenuRequested(QPoint)));
    connect(m_libraryTree, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(libraryItemDoubleClicked(QModelIndex)));

    // Fill library view with files
    fillLibrary();
}

const char* LibraryXmlGuiWindow::actionName(LibraryXmlGuiWindow::Action actionEnum) const
{
    switch (actionEnum) {
    case Library_Open:
        return "library_open";
    case Library_Import:
        return "libary_import";
    case Library_Export:
        return "libary_export";
    case Library_Download:
        return "library_download";
    case Library_Delete:
        return "libary_delete";
    case Library_NewFolder:
        return "library_new_folder";
    case Library_NewCrossword:
        return "library_new_crossword";
    case Library_Update:
        return "library_update";

    default:
        kWarning() << "Action enumerable not handled in switch" << actionEnum;
        return "";
    }
}

void LibraryXmlGuiWindow::setupActions()
{
    // Library actions
    KAction *libraryOpenAction = new KAction(KIcon("document-open"),
            i18nc("&Open", "Open a crossword file"), this);
    libraryOpenAction->setStatusTip(i18n("Open the selected crossword."));
    actionCollection()->addAction(actionName(Library_Open), libraryOpenAction);
    connect(libraryOpenAction, SIGNAL(triggered()), this, SLOT(libraryOpenSlot()));

    KAction *libraryImportAction = new KAction(KIcon("document-import"),
            i18n("&Import"), this);
    libraryImportAction->setStatusTip(i18n("Import a crossword to the library."));
    actionCollection()->addAction(actionName(Library_Import), libraryImportAction);
    connect(libraryImportAction, SIGNAL(triggered()), this, SLOT(libraryImportSlot()));

    KAction *libraryExportAction = new KAction(KIcon("document-export"),
            i18n("&Export"), this);
    libraryExportAction->setStatusTip(i18n("Export the selected crossword from the library."));
    actionCollection()->addAction(actionName(Library_Export), libraryExportAction);
    connect(libraryExportAction, SIGNAL(triggered()), this, SLOT(libraryExportSlot()));

    KAction *libraryDownloadAction = new KAction(KIcon("download"),
            i18n("&Download"), this);
    libraryDownloadAction->setStatusTip(i18n("Download a crossword and add it to the library."));
    actionCollection()->addAction(actionName(Library_Download), libraryDownloadAction);
    connect(libraryDownloadAction, SIGNAL(triggered()), this, SLOT(libraryDownloadSlot()));

    KAction *libraryDeleteAction = new KAction(KIcon("edit-delete"),
            i18n("&Delete"), this);
    libraryDeleteAction->setStatusTip(i18n("Delete the selected crossword from the library."));
    actionCollection()->addAction(actionName(Library_Delete), libraryDeleteAction);
    connect(libraryDeleteAction, SIGNAL(triggered()), this, SLOT(libraryDeleteSlot()));

    KAction *libraryNewFolderAction = new KAction(KIcon("folder-new"),
            i18n("New &Folder"), this);
    libraryNewFolderAction->setStatusTip(i18n("Create a new folder in the library."));
    actionCollection()->addAction(actionName(Library_NewFolder), libraryNewFolderAction);
    connect(libraryNewFolderAction, SIGNAL(triggered()), this, SLOT(libraryNewFolderSlot()));

    KAction *libraryNewCrosswordAction = new KAction(KIcon("document-new"),
            i18n("New &Crossword"), this);
    libraryNewCrosswordAction->setStatusTip(i18n("Create a new crossword in the library."));
    actionCollection()->addAction(actionName(Library_NewCrossword), libraryNewCrosswordAction);
    connect(libraryNewCrosswordAction, SIGNAL(triggered()), this, SLOT(libraryNewCrosswordSlot()));

    KAction *libraryUpdateAction = new KAction(KIcon("view-refresh"),
            i18n("&Refresh"), this);
    libraryUpdateAction->setShortcut(QKeySequence(QKeySequence::Refresh));
    libraryUpdateAction->setStatusTip(i18n("Refreshes the library view after changes in the library folder."));
    actionCollection()->addAction(actionName(Library_Update), libraryUpdateAction);
    connect(libraryUpdateAction, SIGNAL(triggered()), this, SLOT(libraryUpdateSlot()));
}

void LibraryXmlGuiWindow::libraryOpenItem(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    QString fileName = index.sibling(index.row(), 0).data(Qt::UserRole).toString();
    if (!QFileInfo(fileName).isDir())
        m_mainWindow->loadFile(KUrl(fileName));
}

void LibraryXmlGuiWindow::libraryNewCrosswordSlot()
{
    QPointer<CreateNewCrosswordDialog> dialog = new CreateNewCrosswordDialog(this);

    if (dialog->exec() == KDialog::Accepted) {
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

void LibraryXmlGuiWindow::libraryDeleteSlot()
{
    QModelIndex index = m_libraryTree->currentIndex();
    if (index.isValid()) {
        QString fileName = index.data(Qt::UserRole).toString();
        QFileInfo fileInfo(fileName);
        if (fileInfo.isDir()) {
            int result = KMessageBox::warningContinueCancel(this,
                         i18n("This will permanently delete the selected directory and all contained "
                              "crosswords from the library. The operation can't be undone!"), QString(),
                         KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "dont_show_delete_library_directory_confirmation");
            if (result == KMessageBox::Cancel)
                return;

            QDir dir(fileName);
            QFileInfoList infoList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files);
            bool error = false;
            foreach(QFileInfo fi, infoList) {
                QFile file(fi.absoluteFilePath());
                if (!file.remove()) {
                    error = true;
                    KMessageBox::error(this, i18n("Couldn't remove a crossword from the library's directory.\n"
                                                  "You need root privileges to remove crosswords which belong to the global library."));
                    fillLibrary(); // Refill the library model
                    break;
                }
            }

            if (!error) {
                QString dirName = dir.dirName();
                dir.cdUp();
                if (dir.rmdir(dirName)) {
                    m_libraryModel->removeRow(index.row(), index.parent());
                } else
                    KMessageBox::error(this, i18n("Couldn't remove the directory from the library.\n"
                                                  "You need root privileges to remove directories which belong to the global library."));
            }
        } else {
            int result = KMessageBox::warningContinueCancel(this,
                         i18n("This will permanently delete the selected crossword from the library.\nThe operation can't be undone!"), QString(),
                         KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "dont_show_delete_library_crossword_confirmation");
            if (result == KMessageBox::Cancel)
                return;

            QFile file(fileName);
            //      QFile previewFile( fileName.replace(QRegExp("\\.kwpz?$", Qt::CaseInsensitive), ".png") ); // .png
            if (file.remove() /*&& (!previewFile.exists() || previewFile.remove())*/) {
                m_libraryModel->removeRow(index.row(), index.parent());
            } else {
                if (file.error() == QFile::PermissionsError) {
                    KMessageBox::error(this, i18n("Couldn't remove the crossword from the library.\n"
                                                  "You need root privileges to remove crosswords which belong to the global library."));
                } else {
                    KMessageBox::error(this, i18n("Couldn't remove the crossword from the library.\n%1", file.errorString()));
                }
            }
        }
    }
}

void LibraryXmlGuiWindow::libraryNewFolderSlot()
{
    QPointer<KDialog> dlg = new KDialog(this);
    KLineEdit *newFolderName = new KLineEdit(dlg);
    newFolderName->setClearButtonShown(true);
    newFolderName->setClickMessage("Insert the name of the new library folder");
    dlg->setWindowTitle(i18n("New Library Folder"));
    dlg->setMainWidget(newFolderName);
    newFolderName->setFocus();

    if (dlg->exec() == KDialog::Accepted) {
        QString folderName = newFolderName->text();
        if (!folderName.isEmpty()) {
            QString libraryDir = KGlobal::dirs()->saveLocation("appdata", "library");
            QDir dir(libraryDir);
            if (dir.exists(folderName))
                KMessageBox::information(this, i18n("The folder already exists in the library."));
            else {
                dir.mkdir(folderName);
                fillLibrary();
            }
        }
    }

    delete dlg;
}

void LibraryXmlGuiWindow::libraryExportSlot()
{
    QModelIndex index = m_libraryTree->currentIndex();
    if (!index.isValid())
        return;

    QString fileName = index.data(Qt::UserRole).toString();
    QFileInfo fileInfo(fileName);
    if (fileInfo.isDir()) {
        KMessageBox::information(this, i18n("Can't export whole directories. Please select a crossword."));
    } else {
        Crossword::KrossWord krossWord;
        QString errorString;
        if (!krossWord.read(KUrl(fileName), &errorString)) {
            KMessageBox::error(this, i18n("There was an error opening the crossword.\n%1", errorString));
        } else {
#if KDE_IS_VERSION(4,3,60)
            QString fileName = KFileDialog::getSaveFileName(
                                   KGlobalSettings::documentPath(),
                                   "application/x-krosswordpuzzle "
                                   "application/x-krosswordpuzzle-compressed "
                                   "application/x-acrosslite-puz "
                                   "application/pdf "
                                   "application/postscript "
                                   "image/png "
                                   "image/jpeg", this, i18n("Export"),
                                   KFileDialog::ConfirmOverwrite);
#else
            QString fileName;
            QPointer<KFileDialog> fileDlg = new KFileDialog(KGlobalSettings::documentPath(),
                    "application/x-krosswordpuzzle "
                    "application/x-krosswordpuzzle-compressed "
                    "application/x-acrosslite-puz "
                    "application/pdf "
                    "application/postscript "
                    "image/png "
                    "image/jpeg", this);
            fileDlg->setWindowTitle(i18n("Export"));
            fileDlg->setMode(KFile::File);
            fileDlg->setOperationMode(KFileDialog::Saving);
            fileDlg->setConfirmOverwrite(true);
            if (fileDlg->exec() == KFileDialog::Accepted)
                fileName = fileDlg->selectedFile();
            delete fileDlg;
#endif
            if (fileName.isEmpty())
                return;

            QString fileSuffix = QFileInfo(fileName).suffix();
            if (fileSuffix.compare("pdf", Qt::CaseInsensitive) == 0 || fileSuffix.compare("ps", Qt::CaseInsensitive) == 0) {
                QPrinter printer;
                printer.setCreator("KrossWordPuzzle");
                printer.setDocName(fileName);   // TODO: set krossword title if available

                printer.setOutputFileName(fileName);
                KrossWordDocument document(&krossWord, &printer);
                document.print();
            } else if (fileSuffix.compare("png", Qt::CaseInsensitive) == 0
                       || fileSuffix.compare("jpeg", Qt::CaseInsensitive) == 0
                       || fileSuffix.compare("jpg", Qt::CaseInsensitive) == 0) {
                QPointer<KDialog> dlg = new KDialog(this);
                dlg->setWindowTitle(i18n("Export Settings"));
                dlg->setModal(true);
                QWidget *exportToImageDlg = new QWidget;
                ui_export_to_image.setupUi(exportToImageDlg);
                dlg->setMainWidget(exportToImageDlg);
                if (dlg->exec() == KDialog::Rejected) {
                    delete dlg;
                    return;
                }

                QPixmap pix = krossWord.toPixmap(QSize(ui_export_to_image.width->value(), ui_export_to_image.height->value()));
                int quality = ui_export_to_image.quality->value();
                delete dlg;

                if (!pix.save(fileName, 0, quality)) {
                    kDebug() << fileName;
                    KMessageBox::error(this, i18n("Couldn't export the image to the specified file."));
                } else {
                    if (!krossWord.write(fileName, &errorString)) {
                        KMessageBox::error(this, i18n("Couldn't write the crossword to the specified file.\n%1", errorString));
                    }
                }
            }
        } // !krossWord.read() .. else
    } // fileInfo.isDir() .. else
}

void LibraryXmlGuiWindow::libraryImportSlot()
{
    KUrl url = KFileDialog::getOpenUrl(KUrl("kfiledialog:///importCrossword"),
                                       "application/x-krosswordpuzzle "
                                       "application/x-krosswordpuzzle-compressed "
                                       "application/x-acrosslite-puz", this);

    if (!url.isEmpty())
        libraryAddCrossword(url);
}

void LibraryXmlGuiWindow::libraryAddCrossword(const QList< QUrl >& urls, const QString& subFolder)
{
    if (urls.isEmpty())
        return; // No file was chosen

    QString libraryDir = subFolder.isEmpty()
                         ? KGlobal::dirs()->saveLocation("appdata", "library")
                         : KGlobal::dirs()->saveLocation("appdata", QString("library%1%2").arg(QDir::separator()).arg(subFolder));

    Crossword::KrossWord krossWord;
    QString errorString;
    QString lastCrosswordFileName;

    foreach(const KUrl & url, urls) {
        if (!krossWord.read(url, &errorString, this)) {
            KMessageBox::error(this, i18n("The crossword couldn't be imported to the library.\n%1", errorString));
        } else {
            QString name = krossWord.title().trimmed();
            if (name.isEmpty())
                name = "untitled";

            if (!libraryDir.endsWith(QDir::separator()))
                libraryDir.append(QDir::separator());

            name.prepend(libraryDir);

            QString tmpFileName = name;
            int i = 1;
            while (QFile::exists(tmpFileName + ".kwpz"))
                tmpFileName = name + '_' + QString::number(i++);

            QString saveFileName = tmpFileName + ".kwpz";

            if (!krossWord.write(saveFileName, &errorString, Crossword::KrossWord::Normal, Crossword::KrossWord::KrossWordPuzzleCompressedXmlFile)) {
                KMessageBox::error(this, i18n("The crossword couldn't be written to the library folder.\n%1", errorString));
            } else
                lastCrosswordFileName = saveFileName;
        }
    }

    if (!lastCrosswordFileName.isEmpty()) {
        fillLibrary();

        // Select new crossword in the tree view
        QModelIndexList list = m_libraryModel->match(m_libraryModel->index(0, 0), Qt::UserRole, lastCrosswordFileName, 1,
                               Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive);

        if (!list.isEmpty()) {
            m_libraryTree->setCurrentIndex(list.first());
            m_libraryTree->scrollTo(list.first());
        }
    }
}

QList<QTreeWidgetItem*> LibraryXmlGuiWindow::getDownloadCrosswordItems(const QString& rawUrl, const QDate& startDate, const QDate& endDate, int dayOffset, const KIcon &puzIcon)
{
    QList<QTreeWidgetItem*> items;

    QDate date = startDate;
    for (int year = startDate.year(); year <= endDate.year(); ++year) {
        int curMonth = date.month();
        while (date.year() == year && date < endDate) {
            QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << i18n("Weekly crossword %1", KGlobal::locale()->formatDate(date, KLocale::FancyShortDate)));
            item->setIcon(0, puzIcon);
            item->setData(0, Qt::UserRole, rawUrl.arg(date.toString("yyMMdd")));
            items << item;

            date = date.addDays(dayOffset);
            if (date.month() != curMonth)
                curMonth = date.month();
        }
    }

    return items;
}

void LibraryXmlGuiWindow::downloadProviderChanged(int index)
{
    // Get mime type icon
    QString puzIconName = KMimeType::findByPath("crossword.kwp", 0, true)->iconName();
    KIcon puzIcon;
    puzIcon.addPixmap(KIconLoader::global()->loadMimeTypeIcon(puzIconName, KIconLoader::Dialog));

    QList< QTreeWidgetItem* > items;
    QTreeWidgetItem *item;

    DownloadProvider provider = static_cast<DownloadProvider>(ui_download.providers->itemData(index).toInt());
    switch (provider) {
    case HoustonChronicle:
        item = new QTreeWidgetItem(QStringList() << i18n("Daily Crossword From Houston Chronicle"));
        item->setIcon(0, puzIcon);
        item->setData(0, Qt::UserRole, "http://www.chron.com/apps/games/xword/puzzles/today.puz");
        items << item;
        break;

    case JonesinCrosswords:
        items << getDownloadCrosswordItems(
                  "http://herbach.dnsalias.com/Jonesin/jz%1.puz",
                  QDate(2008, 6, 5), QDate::currentDate(), 7, puzIcon);
//    i18n("Weekly Crosswords From Matt Jones %1"),
//         "loadFromJonesin%1",
//         i18n("Crosswords by Matt Jones"),
        break;

    case BostonGlobe:
        items << getDownloadCrosswordItems(
                  "http://home.comcast.net/~nshack/Puzzles/bg%1.puz",
                  QDate(2009, 1, 4), QDate::currentDate(), 7, puzIcon);
//         i18n("Weekly Crosswords From Boston Globe %1"),
//        "loadFromBostonGlobe%1",
//        i18n("Crosswords from Boston Globe by Emily Cox, Henry Rathvon and Henry Hook"),
        break;

    case OnionCrosswords:
        items << getDownloadCrosswordItems(
                  "http://herbach.dnsalias.com/Tausig/av%1.puz",
                  QDate(2008, 1, 2), QDate::currentDate(), 7, puzIcon);
//        i18n("Weekly Crosswords From Onion Crosswords %1"),
//        "loadFromOnion%1",
//        i18n("Crosswords from Onion Crosswords by Ben Tausig"),
        break;

    case WallStreetJournal:
        items << getDownloadCrosswordItems(
                  "http://mazerlm.home.att.net/wsj%1.puz",
                  QDate(2008, 1, 4), QDate::currentDate(), 7, puzIcon);
//        i18n("Weekly Crosswords From Wall Street Journal %1"),
//        "loadFromWallStreetJournal%1",
//        i18n("Crosswords from Wall Street Journal by Mike Shenk"),
        break;

    case WashingtonPost:
        items << getDownloadCrosswordItems(
                  "http://crosswords.washingtonpost.com/wp-srv/style/"
                  "crosswords/util/csserve.cgi?z=sunday&f=cs%1.puz",
                  QDate(2008, 1, 6), QDate(2008, 3, 30), 7, puzIcon);
//        i18n("Weekly Crosswords From Washington Post %1"),
//        "loadFromWashingtonPost%1",
//        i18n("Crosswords from Washington Post by Fred Piscop"),
        break;
    }

    ui_download.crosswords->clear();
    ui_download.crosswords->addTopLevelItems(items);
}

void LibraryXmlGuiWindow::downloadCurrentCrosswordChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    Q_UNUSED(previous);

    m_dialog->button(KDialog::Ok)->setEnabled(current);
    if (!current)
        return;

    ui_download.preview->setPixmap(QPixmap());
    ui_download.preview->setEnabled(false);
    if (ui_download.splitter->sizes().last() == 0) {
        ui_download.preview->setText(i18n("Please reselect the crossword to show a preview..."));
        return;
    } else
        ui_download.preview->setText(i18n("Loading preview..."));

    QString url = current->data(0, Qt::UserRole).toString();
    KFileItem crossword(KUrl(url), "application/x-acrosslite-puz", KFileItem::Unknown);
    m_downloadPreviewJob = new KIO::PreviewJob(KFileItemList() << crossword, 256, 256, 0, 1, false, true, 0);
    m_downloadPreviewJob->setAutoDelete(true);

    connect(m_downloadPreviewJob, SIGNAL(gotPreview(KFileItem, QPixmap)), this, SLOT(downloadPreviewJobGotPreview(KFileItem, QPixmap)));
    connect(m_downloadPreviewJob, SIGNAL(failed(KFileItem)), this, SLOT(downloadPreviewJobFailed(KFileItem)));

    m_downloadPreviewJob->start();
}

void LibraryXmlGuiWindow::downloadPreviewJobFailed(const KFileItem& fi)
{
    kDebug() << fi.url();
}

void LibraryXmlGuiWindow::downloadPreviewJobGotPreview(const KFileItem& fi, const QPixmap& pix)
{
    Q_UNUSED(fi);
    if (!m_dialog)
        return;

    ui_download.preview->setPixmap(pix);
    ui_download.preview->setEnabled(true);
}

void LibraryXmlGuiWindow::libraryDownloadSlot()
{
    // TODO: get rid of m_loadFromInternetList?
    KDialog *dialog = m_dialog = new KDialog(mainWindow());
    dialog->setWindowTitle(i18n("Download Crossword To Library"));
    QWidget *downloadCrosswordsDlg = new QWidget;
    ui_download.setupUi(downloadCrosswordsDlg);
    dialog->setMainWidget(downloadCrosswordsDlg);
    dialog->setModal(true);

    foreach(const DownloadProvider & provider, allDownloadProviders()) {
        switch (provider) {
        case HoustonChronicle:
            ui_download.providers->addItem(
                i18n("Houston Chronicle (daily)"), static_cast<int>(provider));
            break;

        case JonesinCrosswords:
            ui_download.providers->addItem(
                i18n("Matt Jones (thursdays)"), static_cast<int>(provider));
            break;

        case BostonGlobe:
            ui_download.providers->addItem(
                i18n("Boston Globe (sundays)"), static_cast<int>(provider));
            break;

        case OnionCrosswords:
            ui_download.providers->addItem(
                i18n("Onion Crosswords by Ben Tausig (wednesdays)"), static_cast<int>(provider));
            break;

        case WallStreetJournal:
            ui_download.providers->addItem(
                i18n("Wall Street Journal by Mike Shenk (fridays)"), static_cast<int>(provider));
            break;

        case WashingtonPost:
            ui_download.providers->addItem(
                i18n("Washington Post by Fred Piscop (sundays)"), static_cast<int>(provider));
            break;
        }
    }

    ui_download.crosswordsSearchLine->searchLine()->addTreeWidget(ui_download.crosswords);
    ui_download.providers->setCurrentIndex(0);

    connect(ui_download.providers, SIGNAL(currentIndexChanged(int)), this, SLOT(downloadProviderChanged(int)));
    connect(ui_download.crosswords,
            SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
            this, SLOT(downloadCurrentCrosswordChanged(QTreeWidgetItem*, QTreeWidgetItem*)));

    downloadProviderChanged(0);

    QHash< int, QDir > cmbIndexToDir;
    QString sLibraryDir = KGlobal::dirs()->saveLocation("appdata", "library");
    QDir libraryDir(sLibraryDir);
    QFileInfoList fiSubDirs = QDir(sLibraryDir).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    int i = 0, downloadDirIndex = -1;
    foreach(QFileInfo fi, fiSubDirs) {
        int level = 0;
        QDir dir = fi.dir();
        while (dir != sLibraryDir) {
            ++level;
            dir.cdUp();
        }

        if (level == 0 && fi.fileName() == Settings::libraryDownloadSubDir())
            downloadDirIndex = i;

        QString pad;
        while (level-- > 0)
            pad += "  ";

        if (i == downloadDirIndex)
            ui_download.targetDirectory->addItem(KIcon("folder-downloads"), pad + fi.fileName());
        else
            ui_download.targetDirectory->addItem(KIcon("folder"), pad + fi.fileName());

        cmbIndexToDir.insert(i, fi.fileName());
        ++i;
    }

    if (downloadDirIndex >= 0)
        ui_download.targetDirectory->setCurrentIndex(downloadDirIndex);

    if (dialog->exec() == KDialog::Accepted) {
        QModelIndex item = ui_download.crosswords->currentIndex();
        if (item.isValid()) {
            QString url = item.data(Qt::UserRole).toString();
            if (!url.isEmpty()) {
                QString downloadDir = cmbIndexToDir[ui_download.targetDirectory->currentIndex()].path();
                libraryAddCrossword(KUrl(url), downloadDir);
            }
        }
    }

    delete dialog;
    m_dialog = NULL;
}

void LibraryXmlGuiWindow::libraryCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);

    QString fileName = current.data(Qt::UserRole).toString();
    bool crosswordSelected = current.isValid() && QFileInfo(fileName).isFile();

    action(actionName(Library_Export))->setEnabled(crosswordSelected);
    action(actionName(Library_Open))->setEnabled(crosswordSelected);
    action(actionName(Library_Delete))->setEnabled(current.isValid());

    if (crosswordSelected)
        statusBar()->showMessage(i18n("Current crossword filename: \"%1\"", fileName));
}

void LibraryXmlGuiWindow::libraryItemChanged(QStandardItem *item)
{
    // Move crossword in the library from one folder to another
    // or from/to the base library folder

    // Get item of the first column
    if (item->parent()) {
        if (!item->parent()->child(item->row(), 0)) {
            // Item copied/moved to another column...
            kDebug() << "Remove copied item";
            m_libraryModel->removeRow(item->row(), item->index().parent());
            return;
        }

        item = m_libraryModel->itemFromIndex(item->parent()->child(item->row(), 0)->index());
    } else {
        int row = item->row();
        item = m_libraryModel->item(row);

        if (!item) {
            // Item copied/moved to another column...
            kDebug() << "Remove copied item";
            m_libraryModel->removeRow(row);
            return;
        }
    }

    QString oldCrosswordFilePath = item->data(Qt::UserRole).toString();

    if (!QFileInfo(oldCrosswordFilePath).isFile())
        return;

    QString libraryDir = KGlobal::dirs()->saveLocation("appdata", "library");
    QFileInfo oldCrosswordFileInfo = QFileInfo(oldCrosswordFilePath);
    QString oldCrosswordPath = oldCrosswordFileInfo.absolutePath();
    QString newCrosswordPath = item->parent() ? item->parent()->data(Qt::UserRole).toString() : libraryDir;

    if (oldCrosswordPath.endsWith(QDir::separator()))
        oldCrosswordPath = oldCrosswordPath.left(oldCrosswordPath.length() - 1);
    if (newCrosswordPath.endsWith(QDir::separator()))
        newCrosswordPath = newCrosswordPath.left(newCrosswordPath.length() - 1);
    if (libraryDir.endsWith(QDir::separator()))
        libraryDir = libraryDir.left(libraryDir.length() - 1);

    //  kDebug() << "LIBRARY DIR" << libraryDir;
    //  kDebug() << "OLD FILE PATH" << oldCrosswordFilePath;
    //  kDebug() << "OLD DIR" << oldCrosswordPath;
    //  kDebug() << "NEW DIR" << newCrosswordPath;

    if (oldCrosswordPath != newCrosswordPath) {
        QString fileName = oldCrosswordFileInfo.fileName();
        item->setData(newCrosswordPath + QDir::separator() + fileName, Qt::UserRole);

        QModelIndexList indices = m_libraryModel->match(
                                      m_libraryModel->index(0, 0), Qt::UserRole, oldCrosswordPath, 1,
                                      Qt::MatchCaseSensitive | Qt::MatchFixedString);
        QStandardItem *oldParentItem = indices.isEmpty() ? NULL : m_libraryModel->itemFromIndex(indices[0]);

        if (oldCrosswordPath == libraryDir) {
            if (item->parent())
                item->parent()->setText(libraryFolderText(newCrosswordPath, + 1));
            statusBar()->showMessage(
                i18n("Moved crossword \"%1\" from the main library folder to \"%2\"",
                     fileName, QFileInfo(newCrosswordPath).absoluteDir().dirName()));
        } else if (newCrosswordPath == libraryDir) {
            if (oldParentItem)
                oldParentItem->setText(libraryFolderText(oldCrosswordPath, -1));
            statusBar()->showMessage(
                i18n("Moved crossword \"%1\" from \"%2\" to the main library folder",
                     fileName, QFileInfo(oldCrosswordPath).absoluteDir().dirName()));
        } else {
            if (oldParentItem)
                oldParentItem->setText(libraryFolderText(oldCrosswordPath, -1));
            if (item->parent())
                item->parent()->setText(libraryFolderText(newCrosswordPath, + 1));
            statusBar()->showMessage(
                i18n("Moved crossword \"%1\" from \"%2\" to \"%3\"",
                     fileName, QFileInfo(oldCrosswordPath).absoluteDir().dirName(),
                     QFileInfo(newCrosswordPath).absoluteDir().dirName()));
        }

        KIO::move(KUrl(oldCrosswordFilePath), KUrl(newCrosswordPath));
    }
}

void LibraryXmlGuiWindow::libraryTreeContextMenuRequested(const QPoint& pos)
{
    QMenu *menu = createPopupMenu();

    QModelIndex index = m_libraryTree->indexAt(pos);
    if (index.isValid() && index.data(Qt::UserRole + 2).toBool()) {
        m_libraryPopupIndex = index;
        QAction *setAsSubDirAction = menu->addAction(
                                         KIcon("folder-downloads"), i18n("Set as Folder For New &Downloads"),
                                         this, SLOT(librarySetAsSubDirForDownloads()));
        setAsSubDirAction->setCheckable(true);
        if (QFileInfo(index.data(Qt::UserRole).toString()).fileName() == Settings::libraryDownloadSubDir()) {
            setAsSubDirAction->setChecked(true);
        }
    }

    menu->exec(m_libraryTree->mapToGlobal(pos));
}

void LibraryXmlGuiWindow::librarySetAsSubDirForDownloads()
{
    if (m_libraryPopupIndex.isValid()) {
        QString folderPath = m_libraryPopupIndex.data(Qt::UserRole).toString();
        QString folderName = QFileInfo(folderPath).fileName();

        Settings::setLibraryDownloadSubDir(folderName);
        Settings::self()->writeConfig();

        // TODO: Move crosswords from old downloads folder to the new one?
    }
}

void LibraryXmlGuiWindow::previewJobFailed(const KFileItem &fi)
{
    kDebug() << fi.url();
}

void LibraryXmlGuiWindow::previewJobGotPreview(const KFileItem &fi, const QPixmap &pix)
{
    QString fileName = fi.url().path();
    QModelIndexList list = m_libraryModel->match(m_libraryModel->index(0, 0), Qt::UserRole,
                           fileName, 1, Qt::MatchFixedString | Qt::MatchCaseSensitive | Qt::MatchRecursive);
    if (list.isEmpty())
        kDebug() << "Item for preview image not found";
    else
        m_libraryModel->itemFromIndex(list.first())->setIcon(pix);
}

void LibraryXmlGuiWindow::fillLibrary()
{
    // Get mime type icon
    QString puzIconName = KMimeType::findByPath("crossword.kwp", 0, true)->iconName();
    KIcon puzIcon;
    puzIcon.addPixmap(KIconLoader::global()->loadMimeTypeIcon(puzIconName, KIconLoader::Dialog));

    // Create library model or clear it if it already exists
    m_libraryModel = dynamic_cast<QStandardItemModel*>(m_libraryTree->model());
    if (!m_libraryModel) {
        m_libraryModel = new QStandardItemModel();

        connect(m_libraryModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(libraryItemChanged(QStandardItem*)));
        m_libraryTree->setModel(m_libraryModel);
        connect(m_libraryTree->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(libraryCurrentChanged(QModelIndex, QModelIndex)));
    } else
        m_libraryModel->clear();

    // Create HTML delegate
    if (!m_libraryDelegate) {
        m_libraryDelegate = new HtmlDelegate;
        m_libraryTree->setItemDelegate(m_libraryDelegate);
    }

    // Get library folders
    QString libraryDir = KGlobal::dirs()->saveLocation("appdata", "library");
    QFileInfoList fiSubDirs = QDir(libraryDir).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot) << QFileInfo(libraryDir);

    // Add contained crosswords for each folder (including the root folder)
    KFileItemList fileItemList, fileItemListBaseDir;
    QString sAdditionsColor = additionsColorCSS();
    foreach(QFileInfo fi, fiSubDirs) {
        QString filter;
        bool isBaseLibraryDir = fi.absoluteFilePath() == QFileInfo(libraryDir).absoluteFilePath();
        if (isBaseLibraryDir)
            filter = "library/*.kwp?";
        else
            filter = "library/" + fi.fileName() + "/*.kwp?";

        QStringList libraryFiles = KGlobal::dirs()->findAllResources("appdata", filter, KStandardDirs::NoDuplicates);
        QStandardItem *subDirectoryItem = NULL;
        if (!isBaseLibraryDir) {
            int containedCrosswords = QDir(fi.absoluteFilePath()).entryInfoList(
                                          QStringList() << "*.kwp" << "*.kwpz", QDir::NoDotAndDotDot | QDir::Files).count();
            QString itemText = QString("<b>%1</b><br>&nbsp;&nbsp;<span style='color:%4'><b>%2</b> %3</span>")
                               .arg(fi.fileName())
                               .arg(i18nc("The title for contents descriptions of folders in the library tree view", "Content:"))
                               .arg(i18ncp("Text to describe the contents of library folders in the library tree view", "%1 crossword", "%1 crosswords", containedCrosswords))
                               .arg(sAdditionsColor);

            if (fi.fileName() == Settings::libraryDownloadSubDir())
                subDirectoryItem = new QStandardItem(KIcon("folder-downloads"), itemText);
            else
                subDirectoryItem = new QStandardItem(KIcon("folder"), itemText);

            subDirectoryItem->setData(fi.absoluteFilePath(), Qt::UserRole);
            subDirectoryItem->setData("00000" + fi.fileName(), Qt::UserRole + 1);
            subDirectoryItem->setData(true, Qt::UserRole + 2);
            subDirectoryItem->setDragEnabled(false);

            QStandardItem *lastModifiedItem = new QStandardItem("");
            lastModifiedItem->setData("00000" + fi.fileName(), Qt::UserRole + 1);
            lastModifiedItem->setDragEnabled(false);

            m_libraryModel->appendRow(QList<QStandardItem*>() << subDirectoryItem << lastModifiedItem);
        }

        foreach(const QString & libraryFile, libraryFiles) {
            QFileInfo fi(libraryFile);

            QString errorString;
            KrossWordXmlReader::KrossWordInfo info = KrossWordXmlReader::readInfo(KUrl(libraryFile), &errorString);
            if (!info.isValid()) {
                kDebug() << "Error reading crossword info from library file" << errorString;
            }

            if (isBaseLibraryDir) {
                fileItemListBaseDir << KFileItem(KFileItem::Unknown, KFileItem::Unknown, KUrl(libraryFile), true);
            } else {
                fileItemList << KFileItem(KFileItem::Unknown, KFileItem::Unknown, KUrl(libraryFile), true);
            }

            QIcon preview = puzIcon;
            QString title = info.title.isEmpty() ? fi.fileName().remove(QRegExp("\\." + fi.suffix() + '$', Qt::CaseInsensitive)) : info.title;

            QString itemText = QString("<b>%1</b><br>&nbsp;&nbsp;"
                                       "<span style='color:%8;'><b>%2</b> %3x%4<br>&nbsp;&nbsp;"
                                       "<b>%5</b> %6 - %7</span>")
                               .arg(title)
                               .arg(i18nc("The title for sizes of crosswords in the library tree view", "Size:"))
                               .arg(info.width)
                               .arg(info.height)
                               .arg(i18nc("The title for authors of crosswords in the library tree view", "Author(s):"))
                               .arg(info.authors)
                               .arg(info.copyright)
                               .arg(sAdditionsColor);

            QStandardItem *libraryItem = new QStandardItem(preview, itemText);
            libraryItem->setData(libraryFile, Qt::UserRole);
            libraryItem->setData("00001" + info.title, Qt::UserRole + 1);
            libraryItem->setData(false, Qt::UserRole + 2);
            libraryItem->setDropEnabled(false);

            QStandardItem *lastModifiedItem
                = new QStandardItem(KGlobal::locale()->formatDate(fi.lastModified().date(), KLocale::FancyShortDate));
            lastModifiedItem->setData(fi.lastModified(), Qt::UserRole + 1);
            lastModifiedItem->setDropEnabled(false);

            QList<QStandardItem*> itemList;
            itemList << libraryItem << lastModifiedItem;

            if (subDirectoryItem)
                subDirectoryItem->appendRow(itemList);
            else // File from root folder of the library
                m_libraryModel->appendRow(itemList);
        }
    }

    m_libraryModel->setSortRole(Qt::UserRole + 1);
    m_libraryModel->sort(0, Qt::AscendingOrder);

    m_libraryTree->setCurrentIndex(QModelIndex());
    libraryCurrentChanged(QModelIndex(), QModelIndex());

    m_libraryModel->setHeaderData(0, Qt::Horizontal, i18nc("Used as label of the header for the "
                                  "column containing the crossword names in the library model.", "Name"), Qt::DisplayRole);
    m_libraryModel->setHeaderData(1, Qt::Horizontal, i18nc("Used as label of the header for the "
                                  "column containing the times when the crosswords have been last edited in the library "
                                  "model.", "Last Modified"), Qt::DisplayRole);
    m_libraryTree->resizeColumnToContents(0);

    // Get previews
    while (!fileItemListBaseDir.isEmpty())
        fileItemList.prepend(fileItemListBaseDir.takeLast());

    m_previewJob = new KIO::PreviewJob(fileItemList, 64, 64, 0, 1, false, true, 0);
    m_previewJob->setAutoDelete(true);
    connect(m_previewJob, SIGNAL(gotPreview(KFileItem, QPixmap)), this, SLOT(previewJobGotPreview(KFileItem, QPixmap)));
    connect(m_previewJob, SIGNAL(failed(KFileItem)), this, SLOT(previewJobFailed(KFileItem)));
    m_previewJob->start();
}

QString LibraryXmlGuiWindow::additionsColorCSS()
{
    QColor additionsColor = KColorScheme(QPalette::Active).foreground(KColorScheme::PositiveText).color();
    return QString("rgb(%1,%2,%3)").arg(additionsColor.red()).arg(additionsColor.green()).arg(additionsColor.blue());
}

QString LibraryXmlGuiWindow::libraryFolderText(const QString& path, int crosswordCountOffset)
{
    QFileInfo fi(path);
    int containedCrosswords = crosswordCountOffset + QDir(fi.absoluteFilePath()).entryInfoList(QStringList() << "*.kwp" << "*.kwpz", QDir::NoDotAndDotDot | QDir::Files).count();

    return QString("<b>%1</b><br>&nbsp;&nbsp;<span style='color:%4'><b>%2</b> %3</span>")
           .arg(fi.fileName())
           .arg(i18nc("The title for contents descriptions of folders in the library tree view", "Content:"))
           .arg(i18ncp("Text to describe the contents of library folders in the library tree view", "%1 crossword", "%1 crosswords", containedCrosswords))
           .arg(additionsColorCSS());
}
