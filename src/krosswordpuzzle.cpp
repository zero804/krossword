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
#include "krosswordrenderer.h"
#include "settings.h"
#include "libraryxmlguiwindow.h"

// KDE UI includes
#include <KConfigDialog>
#include <KMessageBox>
#include <KStatusBar>
#include <KMenuBar>
#include <KTabWidget>

#include <KgThemeSelector>

#include <KShortcutsDialog>

// KDE action includes
#include <KStandardAction>
#include <KActionCollection>

// Other KDE includes
#include <KStandardDirs>

//#include <KGameDifficulty> -> #include <KgDifficulty>
#include <kapplication.h>
#include <kfileplacesmodel.h>

KrossWordPuzzle::KrossWordPuzzle()
    : KXmlGuiWindow(),
      m_mainLibrary(nullptr),
      m_mainCrossword(nullptr),
      m_loadProgressDialog(nullptr),
      m_mainTabBar(new KTabWidget(this))
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

    setupMainTabWidget();
    setCentralWidget(m_mainTabBar);
    m_mainLibrary->statusBar()->showMessage(i18n("Welcome to KrossWordPuzzle!"));

    // For compatibility with versions of KrossWordPuzzle <= 0.15.6.2 where the
    // menuBar was invisible (another menuBar was created and then set as the
    // corner widget of the tab widget instead).
    menuBar()->show();

    QString lastUnsavedFileBeforeCrash = Settings::lastUnsavedFileBeforeCrash();
    if (!lastUnsavedFileBeforeCrash.isEmpty()) {
        showRestoreOption(lastUnsavedFileBeforeCrash);
    }
}

void KrossWordPuzzle::loadFile(const KUrl &url, Crossword::KrossWord::FileFormat fileFormat, bool loadCrashedFile)
{
    m_mainLibrary->statusBar()->showMessage(i18n("Loading..."));
    m_loadProgressDialog = createLoadProgressDialog();
    m_loadProgressDialog->show();

    m_mainCrossword->loadFile(url, fileFormat, loadCrashedFile);
}

bool KrossWordPuzzle::createNewCrossWord(const Crossword::CrosswordTypeInfo &crosswordTypeInfo,
                                         const QSize &crosswordSize, const QString& title,
                                         const QString& authors, const QString& copyright,
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

//=====================================================================================

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

//=====================================================================================

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


    // Add menus of the embedded crossword window to the menu bar of this (main) window
    QAction *firstMenu = this->menuBar()->actions().last();
    foreach(QAction * action, m_mainCrossword->menuBar()->actions()) {
        if (action->menu() && (action->menu()->objectName() == "game" ||
                               action->menu()->objectName() == "edit" ||
                               action->menu()->objectName() == "move" ||
                               action->menu()->objectName() == "view")) {
            this->menuBar()->insertMenu(firstMenu, action->menu());
        }
    }


    m_mainTabBar->setCurrentIndex(indexLibrary);
    connect(m_mainTabBar, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
    currentTabChanged(indexLibrary);
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

void KrossWordPuzzle::setupActions()
{
    KStandardAction::showStatusbar(this, SLOT(showStatusbarGlobal(bool)), actionCollection());
    KStandardAction::keyBindings(this, SLOT(configureShortcutsGlobal()), actionCollection());
    KStandardAction::configureToolbars(this, SLOT(configureToolbarsGlobal()), actionCollection());
    KStandardAction::preferences(this, SLOT(optionsPreferencesSlot()), actionCollection());
    KStandardAction::quit(qApp, SLOT(closeAllWindows()), actionCollection());
}

void KrossWordPuzzle::showRestoreOption(const QString& lastUnsavedFileBeforeCrash)
{
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

//=====================================================================================

void KrossWordPuzzle::loadSlot(const KUrl &url)
{
    loadFile(url);
}

void KrossWordPuzzle::loadFile(const QString &fileName)
{
    loadFile(KUrl(fileName));
}

void KrossWordPuzzle::optionsPreferencesSlot()
{
    // Avoid to have 2 dialogs shown
    if (KConfigDialog::showDialog("settings")) {
        return;
    }

    KConfigDialog *dialog = new KConfigDialog(this, "settings", Settings::self());

    QWidget *animationSettingsDlg = new QWidget;
    ui_settings.setupUi(animationSettingsDlg);
    dialog->addPage(animationSettingsDlg, i18n("Animations"), "package_settings");

    QWidget *themeSelectorDlg = new QWidget;

    KgThemeSelector *themeSelector = new KgThemeSelector(KrosswordRenderer::self()->getThemeProvider(), KgThemeSelector::EnableNewStuffDownload, themeSelectorDlg);
    dialog->addPage(themeSelector, i18n("Theme"), "games-config-theme");

    connect(KrosswordRenderer::self()->getThemeProvider(), SIGNAL(currentThemeChanged(const KgTheme*)), m_mainCrossword ,SLOT(updateTheme()));
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

void KrossWordPuzzle::currentTabChanged(int index)
{
    bool crosswordTabShown = index == m_mainTabBar->indexOf(m_mainCrossword);

    foreach(QAction * action, this->menuBar()->actions()) {
        if (action->menu() && (action->menu()->objectName() == "game" ||
                               action->menu()->objectName() == "edit" ||
                               action->menu()->objectName() == "move" ||
                               action->menu()->objectName() == "view")) {
            action->setEnabled(crosswordTabShown);
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
    } else {
        if (modificationTypes.testFlag(CrossWordXmlGuiWindow::ModifiedCrossword)) {
            m_mainTabBar->setTabText(iCrossword, i18nc("The title for the "
                                     "crossword tab with an edited crossword opened",
                                     "&Crossword *"));
        } else if (modificationTypes.testFlag(CrossWordXmlGuiWindow::ModifiedState)) {
            m_mainTabBar->setTabText(iCrossword, i18nc("The title for the "
                                     "crossword tab with an unmodified or no crossword opened",
                                     "&Crossword"));
        }
    }

    crosswordCurrentChanged(m_mainCrossword->currentFileName(), m_mainCrossword->currentFileName());
}

void KrossWordPuzzle::crosswordAutoSaveFileChanged(const QString &fileName)
{
    Settings::setLastUnsavedFileBeforeCrash(fileName);
    Settings::self()->writeConfig();
}

//!????
//#include "krosswordpuzzle.moc"
