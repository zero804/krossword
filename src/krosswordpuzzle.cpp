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

#include "krosswordpuzzle.h"
#include "krosswordpuzzleview.h"
#include "krosswordrenderer.h"
#include "krossworddocument.h"
#include "settings.h"
#include "crosswordxmlguiwindow.h"
#include "libraryxmlguiwindow.h"
#include "io/krosswordxmlreader.h"

// Qt UI includes
#include <QPrintDialog>
#include <QTreeWidget>

// Other Qt includes
#include <QCloseEvent>
#include <QtGui/QPrinter>

// KDE UI includes
#include <KXMLGUIFactory>
#include <KMessageBox>
#include <KConfigDialog>
#include <KPrintPreview>
#include <kdeprintdialog.h>
#include <KFileDialog>
#include <KPushButton>
#include <KStatusBar>
#include <KMenu>
#include <KMenuBar>
#include <KToolBar>
#include <KTabWidget>
////#include <KGameThemeSelector>
#include <KgThemeProvider>
#include <KgThemeSelector>

#include <KShortcutsDialog>

// KDE action includes
#include <KAction>
#include <KActionMenu>
#include <KSelectAction>
#include <KToggleAction>
#include <KActionCollection>
#include <KStandardAction>
#include <KStandardGameAction>
#include <KRecentFilesAction>

// Other KDE includes
#include <KDE/KLocale>
#include <KStandardDirs>
//#include <KGameDifficulty>
#include <KgDifficulty>

#include <KRandom>
#include <KTemporaryFile>
#include <kdeversion.h>
#include <kapplication.h>
#include <knewstuff2/engine.h>
#include <KColorScheme>
#include <kfileplacesmodel.h>



/** A KTabWidget with a menu bar shown directly after the tabs.
  * Simply call @ref takeMenu to move the menu of your main window to the tab
  * widget. An event filter is installed on the main window, to track changes
  * that require the layout for the menu to be redone.
  * @Note The code is mostly copied from Palapeli, but with all code for a tab
  * widget with a menu bar in one class. */
class MenuTabWidget : public KTabWidget
{
public:
    MenuTabWidget(QWidget* parent = 0)
        : KTabWidget(parent), m_menuBar(0) {
        setDocumentMode(true);
    }

    void takeMenu(KMainWindow *mainWindow) {
        m_mainWindow = mainWindow;
        m_menuBar    = mainWindow->menuBar();
        m_menuBar->QWidget::setParent(0);
        m_menuBar->QWidget::setParent(m_mainWindow);
        m_menuBar->raise();
        doMenuLayout();

        mainWindow->installEventFilter(this);
    }

    KMenuBar *menu() const {
        return m_menuBar;
    };

protected:
    virtual void changeEvent(QEvent *ev) {
        doMenuLayout();
        KTabWidget::changeEvent(ev);
    }

    virtual bool eventFilter(QObject *obj, QEvent *event) {
        switch (event->type()) {
        case QEvent::Resize:
        case QEvent::FontChange:
        case QEvent::LanguageChange:
        case QEvent::LayoutDirectionChange:
        case QEvent::LocaleChange:
        case QEvent::StyleChange:
            // Relayout the menu whenever the tabbar size may have changed
            doMenuLayout();
            return true;
        default:
            return QObject::eventFilter(obj, event);
        }
    }

    void doMenuLayout() {
        // Determine geometry of menubar...
        QRect rect              = m_mainWindow->rect();
        const QSize tabBarSize  = tabBar()->sizeHint();
        const QSize menuBarSize = m_menuBar->sizeHint();

        setMinimumWidth(tabBarSize.width());

        //...in X direction
        if (QApplication::isLeftToRight())
            rect.setLeft(tabBarSize.width());
        else
            rect.setRight(rect.width() - tabBarSize.width());

        //...in Y direction
        const int height    = menuBarSize.height();
        const int maxHeight = tabBarSize.height() -
                              style()->pixelMetric(QStyle::PM_TabBarBaseHeight, 0, this);

        // Vertical alignment on tab bar, Difference to palapeli: geometry().top() is added if the tabbar isn't at top == 0
        const int yPos = geometry().top() + (maxHeight - height) / 2;
        rect.setHeight(height);
        rect.moveTop(qMax(yPos, 0)); // Do not allow yPos < 0!

        m_menuBar->setGeometry(rect);
    }

private:
    KMainWindow *m_mainWindow;
    KMenuBar    *m_menuBar;
};


KrossWordPuzzle::KrossWordPuzzle()
    : KXmlGuiWindow(),
      m_mainLibrary(0),
      m_mainCrossword(0),
      m_loadProgressDialog(0),
      m_mainTabBar(new MenuTabWidget(this))
{
    if (Settings::libraryDownloadSubDir().isEmpty()) {
        Settings::setLibraryDownloadSubDir(i18n("Downloads"));
        Settings::self()->writeConfig();
    }

    setAcceptDrops(true);
    setObjectName("mainKrossWordPuzzle");
    setupPlaces();
    setupActions();

    setupGUI(Save | Create);

    m_mainTabBar->takeMenu(this);
    setupMainTabWidget();
    setCentralWidget(m_mainTabBar);
    m_mainLibrary->statusBar()->showMessage(i18n("Welcome to KrossWordPuzzle!"));

    // For compatibility with versions of KrossWordPuzzle <= 0.15.6.2 where the
    // menuBar was invisible (another menuBar was created and then set as the
    // corner widget of the tab widget instead).
    menuBar()->show();

    QString lastUnsavedFileBeforeCrash = Settings::lastUnsavedFileBeforeCrash();
    if (!lastUnsavedFileBeforeCrash.isEmpty()) {
//       KInstance
//  KSysGuard::Processes *processes = KSysGuard::Processes::getInstance();
//  QList<KSysGuard::Process *> processlist = processes->getAllProcesses();
//  foreach(KSysGuard::Process * process, processlist) {
//      QString name = process->name;
//  }
//  QString sPid = lastUnsavedFileBeforeCrash.mid( lastUnsavedFileBeforeCrash.lastIndexOf("PID") + 3 );
//  sPid = sPid.left( sPid.length() - 4 ); // Remove ".tmp" from the end
//  bool ok;
//  long int pid = sPid.toLong( &ok );
//  if ( ok ) {
//      KSysGuard::Processes *processes = KSysGuard::Processes::getInstance();
//      KSysGuard::Process *process = processes->getProcess( pid );
//      if ( process ) {
//      kDebug() << "Game didn't crash, it's still running!";
//      }
//  }

        KGuiItem restoreButton(i18n("&Restore"), KIcon("document-open"), i18n("Restore the automatically saved version of an edited crossword before the crash"));
        int result = KMessageBox::questionYesNo(this, i18n("An unsaved crossword has been found. Most likely the game crashed, sorry!\n "
                                                "Do you want to restore the crossword?"),
                                                QString(),
                                                restoreButton,
                                                KStandardGuiItem::discard());
        if (result == KMessageBox::Yes) {
            loadFile(lastUnsavedFileBeforeCrash, Crossword::KrossWord::KrossWordPuzzleCompressedXmlFile, true);
        } else {
            m_mainCrossword->removeTempFile(lastUnsavedFileBeforeCrash);
        }
    }
}

void KrossWordPuzzle::setupPlaces()
{
    KUrl libraryUrl   = KUrl(KGlobal::dirs()->saveLocation("appdata", "library"));
    KUrl templatesUrl = KUrl(KGlobal::dirs()->saveLocation("appdata", "templates"));

    KFilePlacesModel *placesModel = new KFilePlacesModel();
    if (placesModel->url(placesModel->closestItem(libraryUrl)) != libraryUrl) {
        placesModel->addPlace(i18n("Library"), KUrl(libraryUrl), "favorites", KApplication::applicationName());
    }

    if (placesModel->url(placesModel->closestItem(templatesUrl)) != templatesUrl) {
        placesModel->addPlace(i18n("Templates"), KUrl(templatesUrl), "krosswordpuzzle", KApplication::applicationName());
    }

    delete placesModel;
}

void KrossWordPuzzle::showStatusbarGlobal(bool show)
{
    if (m_mainTabBar->currentWidget() == m_mainCrossword) {
        m_mainCrossword->statusBar()->setVisible(show);
    } else {
        m_mainLibrary->statusBar()->setVisible(show);
    }
}

int KrossWordPuzzle::configureShortcutsGlobal()
{
    KShortcutsDialog dlg(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);
    if (m_mainTabBar->currentWidget() == m_mainCrossword) {
        dlg.addCollection(m_mainCrossword->actionCollection());
    } else {
        dlg.addCollection(m_mainLibrary->actionCollection());
    }

    return dlg.configure(true);
}

void KrossWordPuzzle::configureToolbarsGlobal()
{
    if (m_mainTabBar->currentWidget() == m_mainCrossword) {
        m_mainCrossword->configureToolbars();
    } else {
        m_mainLibrary->configureToolbars();
    }
}

const char *KrossWordPuzzle::actionName(KrossWordPuzzle::Action actionEnum) const
{
    switch (actionEnum) {
    case Game_Download:
        return "game_download";
    case Game_Upload:
        return "game_upload";
        //  case Options_Themes:
        //      return "options_themes";
    case RecentTab_RecentFilesRemove:
        return "recent_files_remove";
    default:
        kWarning() << "Action enumerable not handled in switch" << actionEnum;
        return "";
    }
}

bool KrossWordPuzzle::createNewCrossWord(const Crossword::CrosswordTypeInfo &crosswordTypeInfo,
        const QSize &crosswordSize,
        const QString& title,
        const QString& authors,
        const QString& copyright,
        const QString& notes)
{
    if (m_mainCrossword->createNewCrossWord(crosswordTypeInfo, crosswordSize, title, authors, copyright, notes)) {
        int indexCrossword = m_mainTabBar->indexOf(m_mainCrossword);
        m_mainTabBar->setTabEnabled(indexCrossword, true);
        m_mainTabBar->setCurrentIndex(indexCrossword);
        return true;
    } else {
        return false;
    }
}

bool KrossWordPuzzle::createNewCrossWordFromTemplate(const QString& templateFilePath, const QString& title, const QString& authors, const QString& copyright, const QString& notes)
{
    if (m_mainCrossword->createNewCrossWordFromTemplate(templateFilePath, title, authors, copyright, notes)) {
        int indexCrossword = m_mainTabBar->indexOf(m_mainCrossword);
        m_mainTabBar->setTabEnabled(indexCrossword, true);
        m_mainTabBar->setCurrentIndex(indexCrossword);
        return true;
    } else {
        return false;
    }
}

void KrossWordPuzzle::setupMainTabWidget()
{
    m_mainLibrary      = new LibraryXmlGuiWindow(this);
    int indexLibrary   = m_mainTabBar->addTab(m_mainLibrary, i18n("&Library"));
    m_mainCrossword    = new CrossWordXmlGuiWindow(this);
    int indexCrossword = m_mainTabBar->addTab(m_mainCrossword, i18n("&Crossword"));

    // Set tab icons
    QString puzIconName = KMimeType::findByPath("crossword.kwp", 0, true)->iconName();
    KIcon puzIcon;
    puzIcon.addPixmap(KIconLoader::global()->loadMimeTypeIcon(puzIconName, KIconLoader::Dialog));

    m_mainTabBar->setTabIcon(indexLibrary, KIcon("favorites"));
    m_mainTabBar->setTabIcon(indexCrossword, puzIcon);
    m_mainTabBar->setTabEnabled(indexCrossword, false);

    m_mainCrossword->krossWord()->setAnimationTypes(animationTypesFromSettings());

    connect(m_mainCrossword, SIGNAL(loadingFileComplete(QString)),
            this,            SLOT(crosswordLoadingComplete(QString)));

    connect(m_mainCrossword, SIGNAL(errorLoadingFile(QString)),
            this,            SLOT(crosswordErrorLoading(QString)));

    connect(m_mainCrossword, SIGNAL(currentFileChanged(QString, QString)),
            this,            SLOT(crosswordCurrentChanged(QString, QString)));

    connect(m_mainCrossword, SIGNAL(fileClosed(QString)),
            this,            SLOT(crosswordClosed(QString)));

    connect(m_mainCrossword, SIGNAL(modificationTypesChanged(CrossWordXmlGuiWindow::ModificationTypes)),
            this,            SLOT(crosswordModificationsChanged(CrossWordXmlGuiWindow::ModificationTypes)));

    connect(m_mainCrossword, SIGNAL(tempAutoSaveFileChanged(QString)),
            this,            SLOT(crosswordAutoSaveFileChanged(QString)));

    // Setup recent tab
//     ui_start_new.openRecent->setIcon( KIcon("document-open-recent") );
    // TODO: revival of the recent files action?
//     foreach ( KUrl url, m_recentFilesAction->urls() ) {
//  QString iconName = KMimeType::findByUrl( url, 0, /*url.isLocalFile()*/true )->iconName();
//  KIcon icon;
//  icon.addPixmap( KIconLoader::global()->loadMimeTypeIcon(iconName, KIconLoader::Dialog) );
//  QListWidgetItem *item = new QListWidgetItem( icon,
//      QString("%1\n  (%2)").arg(url.fileName()).arg(url.pathOrUrl()) );
// //   item->setData( Qt::UserRole, url ); // QListWidgetItem doesn't store Qt::UserRole-data...
//
//  ui_start_new.recentFiles->addItem( item );
//     }
    /*
        if ( ui_start_new.recentFiles->count() == 0 ) {
        ui_start_new.openRecent->hide();
        ui_start_new.recentFiles->hide();
        ui_start_new.lblNoRecentFiles->show();

    //  m_mainTabBar->setCurrentIndex( indexLoad );
        } else {
        ui_start_new.lblNoRecentFiles->hide();
        ui_start_new.tabRecent->layout()->removeItem( ui_start_new.spacerNoRecentFilesTop );
        ui_start_new.tabRecent->layout()->removeItem( ui_start_new.spacerNoRecentFilesBetween );
        ui_start_new.tabRecent->layout()->removeItem( ui_start_new.spacerNoRecentFilesBottom );
        ui_start_new.recentFiles->setCurrentRow( 0 );
        QListWidgetItem *mostRecentItem = ui_start_new.recentFiles->item( 0 );
        QFont font = mostRecentItem->font();
        font.setBold( true );
        mostRecentItem->setFont( font );

    //  m_mainTabBar->setCurrentIndex( indexRecent );
        }*/

//     connect( ui_start_new.recentFiles, SIGNAL(executed(QListWidgetItem*)),
//      this, SLOT(recentFileExecuted(QListWidgetItem*)) );
//     connect( ui_start_new.recentFiles, SIGNAL(customContextMenuRequested(QPoint)),
//      this, SLOT(recentFileListContextMenuRequested(QPoint)) );
//     connect( ui_start_new.openRecent, SIGNAL(clicked()), this, SLOT(loadRecentItem()) );

    // Add menus of the embedded crossword window to the menu bar of this (main) window
    QAction *firstMenu = m_mainTabBar->menu()->actions().first();
    foreach(QAction * action, m_mainCrossword->menuBar()->actions()) {
        if (action->menu() && (action->menu()->objectName() == "game" ||
                               action->menu()->objectName() == "edit" ||
                               action->menu()->objectName() == "move" ||
                               action->menu()->objectName() == "view")) {
            m_mainTabBar->menu()->insertMenu(firstMenu, action->menu());
        }
    }

    m_mainTabBar->setCurrentIndex(indexLibrary);
    connect(m_mainTabBar, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
    currentTabChanged(indexLibrary);
}

void KrossWordPuzzle::currentTabChanged(int index)
{
    bool crosswordTabShown = index == m_mainTabBar->indexOf(m_mainCrossword);

    foreach(QAction * action, m_mainTabBar->menu()->actions()) {
        if (action->menu() && (action->menu()->objectName() == "game" ||
                               action->menu()->objectName() == "edit" ||
                               action->menu()->objectName() == "move" ||
                               action->menu()->objectName() == "view")) {
            action->setVisible(crosswordTabShown);
        }
    }

    unplugActionList("options_list");

    QList<QAction*> optionsList;
    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    if (crosswordTabShown) {
        setCaption(m_caption, m_mainCrossword->isModified());
        optionsList << m_mainCrossword->action(m_mainCrossword->actionName(CrossWordXmlGuiWindow::Options_Dictionaries));
        optionsList << separator;
        //  optionsList << m_mainCrossword->action( "options_show_menubar" );
        optionsList << m_mainCrossword->toolBarMenuAction();
        optionsList << m_mainCrossword->action(m_mainCrossword->actionName(CrossWordXmlGuiWindow::ShowClueDock));
        optionsList << m_mainCrossword->action(m_mainCrossword->actionName(CrossWordXmlGuiWindow::ShowUndoViewDock));
        optionsList << m_mainCrossword->action(m_mainCrossword->actionName(CrossWordXmlGuiWindow::ShowCurrentCellDock));

        kDebug() << optionsList;
    } else { // tabLibrary
        setCaption(i18n("Library"));
        optionsList << m_mainCrossword->action(m_mainCrossword->actionName(CrossWordXmlGuiWindow::Options_Dictionaries));
        optionsList << separator;
        optionsList << m_mainLibrary->toolBarMenuAction();
    }

    plugActionList("options_list", optionsList);
}

void KrossWordPuzzle::crosswordLoadingComplete(const QString& fileName)
{
    Q_UNUSED(fileName);

    if (m_loadProgressDialog) // When loading a template there is no load progress dialog
        m_loadProgressDialog->close();

    int indexCrossword = m_mainTabBar->indexOf(m_mainCrossword);
    m_mainTabBar->setTabEnabled(indexCrossword, true);
    m_mainTabBar->setCurrentIndex(indexCrossword);

    m_mainLibrary->statusBar()->showMessage(i18nc("Loaded '%1'", "Statusbar text "
                                            "when a crossword has been loaded, %1 gets replaced by the file name",
                                            fileName));
}

void KrossWordPuzzle::crosswordErrorLoading(const QString& fileName)
{
    m_loadProgressDialog->close();
    m_mainLibrary->statusBar()->showMessage(i18nc("Error loading file '%1'",
                                            "Statusbar text when there was an error while loading a crossword, "
                                            "%1 gets replaced by the file name", fileName));
}

void KrossWordPuzzle::crosswordClosed(const QString& fileName)
{
    Q_UNUSED(fileName);

    m_mainTabBar->setTabEnabled(m_mainTabBar->indexOf(m_mainCrossword), false);
    m_mainTabBar->setCurrentWidget(m_mainLibrary);
}

void KrossWordPuzzle::crosswordCurrentChanged(const QString& fileName, const QString& oldFileName)
{
    Q_UNUSED(oldFileName);

    if (fileName.isEmpty()) {
        m_caption.clear();
    } else {
        stateChanged("no_file_opened", StateReverse);

        if (m_mainCrossword->krossWord()->title().isEmpty())
            m_caption = displayFileName(fileName);
        else
            m_caption = m_mainCrossword->krossWord()->title();
    }

    setCaption(m_caption, m_mainCrossword->isModified());
}

void KrossWordPuzzle::crosswordModificationsChanged(CrossWordXmlGuiWindow::ModificationTypes modificationTypes)
{
    int iCrossword = m_mainTabBar->indexOf(m_mainCrossword);
    if (modificationTypes == CrossWordXmlGuiWindow::NoModification) {
        m_mainTabBar->setTabText(iCrossword, i18nc("The title for the "
                                 "crossword tab with an unmodified or no crossword opened",
                                 "&Crossword"));
        //  if (m_mainTabBar->isTabEnabled(iCrossword)) {
        //      m_mainTabBar->setTabTextColor( iCrossword, KColorScheme(QPalette::Active).foreground().color());
        //  } else {
        //      m_mainTabBar->setTabTextColor( iCrossword, KColorScheme(QPalette::Disabled).foreground().color());
        //  }
    } else {
        //  m_mainTabBar->setTabTextColor(iCrossword, KColorScheme(QPalette::Active).foreground(KColorScheme::NeutralText).color());

        if (modificationTypes.testFlag(CrossWordXmlGuiWindow::ModifiedCrossword)) {
            m_mainTabBar->setTabText(iCrossword, i18nc("The title for the "
                                     "crossword tab with an edited crossword opened",
                                     "&Crossword *"));

            //  if (modificationTypes.testFlag(CrossWordXmlGuiWindow::ModifiedState))
            //  // Both edited and state changed
            //      m_mainTabBar->setTabTextColor( iCrossword, KColorScheme(QPalette::Active).foreground(KColorScheme::NeutralText).color());
            //  else
            //  // Only edited
            //     m_mainTabBar->setTabTextColor(iCrossword, KColorScheme(QPalette::Active).foreground(KColorScheme::NeutralText).color());
        } else if (modificationTypes.testFlag(CrossWordXmlGuiWindow::ModifiedState)) {
            m_mainTabBar->setTabText(iCrossword, i18nc("The title for the "
                                     "crossword tab with an unmodified or no crossword opened",
                                     "&Crossword"));

            // // Only state changed
            // m_mainTabBar->setTabTextColor(iCrossword, KColorScheme(QPalette::Active).foreground(KColorScheme::NeutralText).color());
        }
    }

    crosswordCurrentChanged(m_mainCrossword->currentFileName(), m_mainCrossword->currentFileName());
}

void KrossWordPuzzle::crosswordAutoSaveFileChanged(const QString &fileName)
{
    Settings::setLastUnsavedFileBeforeCrash(fileName);
    Settings::self()->writeConfig();
}

QString KrossWordPuzzle::displayFileName(const QString &fileName)
{
    QString libraryDir = KGlobal::dirs()->saveLocation("appdata", "library");
    if (fileName.startsWith(libraryDir)) {
        // Cut the library path
        QString libraryFileName = fileName.mid(libraryDir.length());
        libraryFileName.prepend(i18nc("This string is used to replace the library path "
                                      "for crossword files that are in the library with a shorter user visible string, "
                                      "e.g. replacing ~/.kde4/apps/krosswordpuzzle/library/crossword.kwpz with "
                                      "Library/crossword.kwpz", "Library") + QDir::separator());
        return libraryFileName;
    } else {
        return fileName;
    }
}

bool KrossWordPuzzle::isFileInLibrary(const QString& fileName)
{
    QString libraryDir = KGlobal::dirs()->saveLocation("appdata", "library");
    return fileName.startsWith(libraryDir);
}

void KrossWordPuzzle::loadFile(const KUrl &url, Crossword::KrossWord::FileFormat fileFormat, bool loadCrashedFile)
{
    m_mainLibrary->statusBar()->showMessage(i18n("Loading..."));
    m_loadProgressDialog = createLoadProgressDialog();
    m_loadProgressDialog->show();

    m_mainCrossword->loadFile(url, fileFormat, loadCrashedFile);
}

void KrossWordPuzzle::closeEvent(QCloseEvent* event)
{
    if (m_mainCrossword->isModified()) {
        QString msg = i18n("The current crossword has been modified.\n"
                           "Would you like to save it?");

        int result = KMessageBox::warningYesNoCancel(this, msg, i18n("Close Document"), KStandardGuiItem::save(), KStandardGuiItem::discard());

        if (result == KMessageBox::Cancel || (result == KMessageBox::Yes && !m_mainCrossword->save()))
            event->ignore();
        else
            event->accept();
    } else {
        event->accept();
    }

    if (event->isAccepted()) {
        // Closing not aborted, so the temporary file can be deleted
        m_mainCrossword->removeTempFile();
        m_mainCrossword->setState(CrossWordXmlGuiWindow::ShowingNothing); // Cleanup
    }
}

void KrossWordPuzzle::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->accept();
}

void KrossWordPuzzle::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        QPoint pt         = m_mainLibrary->libraryTree()->mapFrom(this, event->pos());
        QModelIndex index = m_mainLibrary->libraryTree()->indexAt(pt);

        if (index.isValid()) {
            if (index.data(Qt::UserRole + 2).toBool() || index.parent().isValid()) {
                // Dropped onto library folder
                QString subFolder = index.parent().isValid() ?
                                    QFileInfo(index.parent().data(Qt::UserRole).toString()).fileName() :
                                    QFileInfo(index.data(Qt::UserRole).toString()).fileName();

                m_mainLibrary->libraryAddCrossword(event->mimeData()->urls(), subFolder);
            } else {
                m_mainLibrary->libraryAddCrossword(event->mimeData()->urls());
            }
        } else {
            QList<QUrl> urls = event->mimeData()->urls();
            loadFile(urls.first());
        }
    }
}

KDialog* KrossWordPuzzle::createLoadProgressDialog()
{
    KDialog *dialog = new KDialog(this);
    dialog->setButtons(KDialog::None);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    // TODO: No max/min buttons
    dialog->setWindowTitle(i18n("Loading..."));
    QLabel *lblLoad = new QLabel(i18n("Loading the crossword, please wait..."));

    dialog->setMainWidget(lblLoad);
    dialog->setModal(true);
    return dialog;
}

/*
void KrossWordPuzzle::recentFileListContextMenuRequested( const QPoint &pos ) {
    QMenu *menu = popupMenuRecentFilesList();

    action( actionName(RecentTab_RecentFilesRemove) )->setEnabled( ui_start_new.recentFiles->itemAt(pos) );
    menu->exec( ui_start_new.recentFiles->mapToGlobal(pos) );
}*/
/*
void KrossWordPuzzle::recentFilesClearSlot() {
    ui_start_new.recentFiles->clear();
    m_recentFilesAction->clear();
    m_recentFilesAction->saveEntries( Settings::self()->config()->group("") );
}*/
/*
void KrossWordPuzzle::recentFilesRemoveSlot() {
    QListWidgetItem *item = ui_start_new.recentFiles->currentItem();
    if ( !item ) {
    kDebug() << "No current item";
    return;
    }

    // Extract url from display string, because QListWidgetItem doesn't store Qt::UserRole-data..
    QString url = item->text().remove(QRegExp("(^.*\\(|\\)$)"));
    delete ui_start_new.recentFiles->takeItem( ui_start_new.recentFiles->row(item) );
    m_recentFilesAction->removeUrl( KUrl(url) );
    m_recentFilesAction->saveEntries( Settings::self()->config()->group("") );
}*/
/*
void KrossWordPuzzle::loadRecentItem() {
    QListWidgetItem *item = ui_start_new.recentFiles->currentItem();
    if ( !item )
    return;

    // Extract url from display string, because QListWidgetItem doesn't store Qt::UserRole-data..
    QString url = item->text().remove(QRegExp("(^.*\\(|\\)$)"));
    loadSlot( KUrl(url) );
}

void KrossWordPuzzle::recentFileExecuted( QListWidgetItem *item ) {
    if ( !item )
    return;

    // Extract url from display string, because QListWidgetItem doesn't store Qt::UserRole-data..
    QString url = item->text().remove(QRegExp("(^.*\\(|\\)$)"));
    loadSlot( KUrl(url) );
}*/

void KrossWordPuzzle::setupActions()
{
    KStandardAction::showStatusbar(this, SLOT(showStatusbarGlobal(bool)), actionCollection());
    KStandardAction::keyBindings(this, SLOT(configureShortcutsGlobal()), actionCollection());
    KStandardAction::configureToolbars(this, SLOT(configureToolbarsGlobal()), actionCollection());
    KStandardAction::preferences(this, SLOT(optionsPreferencesSlot()), actionCollection());
    KStandardAction::quit(qApp, SLOT(closeAllWindows()), actionCollection());

//     KStandardGameAction::load(this, SLOT(loadSlot()), actionCollection())->setStatusTip( i18n("Load a crossword from a file") );

//     KAction *downloadAction = new KAction( KIcon("get-hot-new-stuff"),
//                     i18n("Get new crosswords..."), actionCollection() );
//     downloadAction->setStatusTip( i18n("Download crosswords from other users.") );
//     actionCollection()->addAction( actionName(Game_Download), downloadAction );
//     connect( downloadAction, SIGNAL(triggered()), this, SLOT(downloadSlot()) );
//
//     KAction *uploadAction = new KAction( KIcon("network-server"),
//                     i18n("Upload current crossword..."), actionCollection() );
//     uploadAction->setStatusTip( i18n("Share the current crossword with other users.") );
//     actionCollection()->addAction( actionName(Game_Upload), uploadAction );
//     connect( uploadAction, SIGNAL(triggered()), this, SLOT(uploadSlot()) );
    /*
        m_recentFilesAction = KStandardGameAction::loadRecent(
            this, SLOT(loadRecentSlot(KUrl)), actionCollection());
        m_recentFilesAction->setIcon( KIcon("document-open-recent") ); // Not set by KStandardAction...
        m_recentFilesAction->loadEntries( Settings::self()->config()->group("") );
        m_recentFilesAction->setStatusTip( i18n("Load recent crosswords") );*/
    //     KStandardAction::openNew(this, SLOT(fileNew()), actionCollection());
//     KStandardGameAction::quit(qApp, SLOT(closeAllWindows()), actionCollection())->setStatusTip( i18n("Quit the game") );
    //     KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());
    /*
        m_undoStack->createUndoAction( actionCollection(), actionName(Edit_Undo) );
        m_undoStack->createRedoAction( actionCollection(), actionName(Edit_Redo) );*/
    /*
        KStandardGameAction::gameNew(this, SLOT(gameNewSlot()), actionCollection())->setStatusTip( i18n("Start a new crossword") );*/
}

void KrossWordPuzzle::gameNewSlot()
{
    // create a new window
    (new KrossWordPuzzle)->show();
}

void KrossWordPuzzle::downloadSlot()
{
    KNS::Entry::List entries = KNS::Engine::download();
    kDebug() << "Entries count =" << entries.count();
//     KNS::Engine engine( this );
//     if ( engine.init("krosswordpuzzle.knsrc" )) {
//  KNS::Entry::List entries = engine.downloadDialogModal( this );
//
//  kDebug() << "Entries count =" << entries.count();
//  if (entries.size() > 0) {
//      foreach ( KNS::Entry *entry, entries ) {
//      // Downloaded file has the name "hotstuff-access" which is wrong (maybe it works
//      // better with archives). So rename the file to the right name from the payload:
//      QString filename = entry->payload().representation()
//          .remove( QRegExp("^.*\\?file=") ).remove( QRegExp("&site=.*$") );
//      QStringList installedFiles = entry->installedFiles();
//
//      kDebug() << "installedFiles =" << installedFiles;
// //       if ( !installedFiles.isEmpty() ) {
// //           QString installedFile = installedFiles[0];
// //
// //           QString path = KUrl( installedFile ).path().remove( QRegExp("/[^/]*$") ) + "/";
// //           QFile( installedFile ).rename( path + filename );
//
// //           qDebug() << "PublicTransportSettings::downloadServiceProvidersClicked" <<
// //           "Rename" << installedFile << "to" << path + filename;
// //       }
//      }
//  }
//     }
}

void KrossWordPuzzle::uploadSlot()
{
// TODO: Get filename from CrossWordXmlGuiWindow
//     KNS::Entry *entry = KNS::Engine::upload( m_curFileName );
//     if ( entry )
//  kDebug() << "Entry =" << entry->payload().translated( entry->payload().language() );
//     else
//  kDebug() << "No uploaded entry";
}

void KrossWordPuzzle::optionsPreferencesSlot()
{
    // Avoid to have 2 dialogs shown
    if (KConfigDialog::showDialog("settings")) {
        return;
    }

    KConfigDialog *dialog = new KConfigDialog(this, "settings", Settings::self());

#if QT_VERSION >= 0x040600
    QWidget *animationSettingsDlg = new QWidget;
    ui_settings.setupUi(animationSettingsDlg);
    dialog->addPage(animationSettingsDlg, i18n("Animations"), "package_settings");
#endif

    QWidget *themeSelectorDlg = new QWidget;
    //KGameThemeSelector *themeSelector = new KGameThemeSelector( themeSelectorDlg, Settings::self(), KGameThemeSelector::NewStuffDisableDownload );

    KgThemeProvider* provider = new KgThemeProvider(QByteArray(), themeSelectorDlg);
    provider->discoverThemes("appdata", QLatin1String("themes"));

    KgThemeSelector *themeSelector = new KgThemeSelector(provider, KgThemeSelector::EnableNewStuffDownload, themeSelectorDlg);
    dialog->addPage(themeSelector, i18n("Theme"), "games-config-theme");

    connect(dialog, SIGNAL(settingsChanged(QString)), this, SLOT(settingsChanged()));
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    dialog->show();
}

void KrossWordPuzzle::settingsChanged()
{
    m_mainCrossword->updateTheme();

    m_mainCrossword->krossWord()->setAnimationTypes(animationTypesFromSettings());
    if (m_mainCrossword->solutionKrossWord())
        m_mainCrossword->solutionKrossWord()->setAnimationTypes(animationTypesFromSettings());
}

AnimationTypes KrossWordPuzzle::animationTypesFromSettings()
{
    if (!Settings::animate())
        return NoAnimation;

    AnimationTypes anim = NoAnimation;
    if (Settings::animateSizeChange())
        anim |= AnimateSizeChange;
    if (Settings::animatePosChange())
        anim |= AnimatePosChange;
    if (Settings::animateAppear())
        anim |= AnimateAppear;
    if (Settings::animateDisappear())
        anim |= AnimateDisappear;
    if (Settings::animateChangeLetter())
        anim |= AnimateChangeLetter;
    if (Settings::animateFocusIn())
        anim |= AnimateFocusIn;
    if (Settings::animateTransition())
        anim |= AnimateTransition;

    return anim;
}

// QMenu* KrossWordPuzzle::popupMenuRecentFilesList() {
//     return static_cast<QMenu*>( factory()->container("recent_files_list_popup", this) );
// }

#include "krosswordpuzzle.moc"
