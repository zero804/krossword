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
#include "library/librarygui.h"

#include <KConfigDialog>
#include <KMessageBox>
//#include <KStatusBar>
#include <QStackedWidget>
#include <KgThemeSelector>
#include <KShortcutsDialog>

// KDE action includes
#include <KStandardAction>
#include <KStandardGameAction>
#include <KActionCollection>

//#include <kapplication.h>
//#include <kfileplacesmodel.h>
#include <KFilePlacesModel>
#include <QStandardPaths>

#include <QMenuBar>
#include <QStatusBar>

KrossWordPuzzle::KrossWordPuzzle() : KXmlGuiWindow(),
      m_mainLibrary(nullptr),
      m_mainCrossword(nullptr),
      m_loadProgressDialog(nullptr),
      m_mainStackedBar(new QStackedWidget(this))
{
    setAcceptDrops(true);
    setObjectName("mainKrossWordPuzzle");
    setupPlaces();

    setupActions();
    setupGUI(Save | Create);

    setupMainTabWidget();
    setCentralWidget(m_mainStackedBar);

    QString lastUnsavedFileBeforeCrash = Settings::lastUnsavedFileBeforeCrash();
    if (!lastUnsavedFileBeforeCrash.isEmpty()) {
        showRestoreOption(lastUnsavedFileBeforeCrash);
    }
}

QSize KrossWordPuzzle::sizeHint() const
{
    // expand a bit the window ("The size of top-level widgets are constrained to 2/3 of the desktop's height and width")
    // to define the default size for the first run
    return KXmlGuiWindow::sizeHint().expandedTo(QSize(1024, 768));
}

void KrossWordPuzzle::loadFile(const QUrl &url, Crossword::KrossWord::FileFormat fileFormat, bool loadCrashedFile)
{
    m_loadProgressDialog = createLoadProgressDialog();
    m_loadProgressDialog->show();

    bool loaded = m_mainCrossword->loadFile(url, fileFormat, loadCrashedFile);

    QString path = url.path();
    bool is_internal_file = m_mainLibrary->libraryManager()->isInLibrary(path);

    if(!is_internal_file) {
        if(loaded) {
            QString msg = i18n("Would you like to add the crossword into the library?");
            int result = KMessageBox::questionYesNo(this, msg, i18n("Save crossword"), KStandardGuiItem::yes(), KStandardGuiItem::no());

            if (result == KMessageBox::Yes)
                m_mainLibrary->libraryAddCrossword(url);
        }
    }

    if(!loaded) {
        QString msg = i18n("Could not open resource at ") + url.url(QUrl::PreferLocalFile);
        KMessageBox::sorry(this, msg, i18n("Resource unavailable"));
    }
}

bool KrossWordPuzzle::createNewCrossWord(const Crossword::CrosswordTypeInfo &crosswordTypeInfo,
                                         const QSize &crosswordSize, const QString& title,
                                         const QString& authors, const QString& copyright,
                                         const QString& notes)
{
    if (m_mainCrossword->createNewCrossWord(crosswordTypeInfo, crosswordSize, title, authors, copyright, notes)) {
        int indexCrossword = m_mainStackedBar->indexOf(m_mainCrossword);
        m_mainStackedBar->setCurrentIndex(indexCrossword);
        return true;
    } else {
        return false;
    }
}

bool KrossWordPuzzle::createNewCrossWordFromTemplate(const QString& templateFilePath, const QString& title, const QString& authors, const QString& copyright, const QString& notes)
{
    if (m_mainCrossword->createNewCrossWordFromTemplate(templateFilePath, title, authors, copyright, notes)) {
        int indexCrossword = m_mainStackedBar->indexOf(m_mainCrossword);
        m_mainStackedBar->setCurrentIndex(indexCrossword);
        return true;
    } else {
        return false;
    }
}

//=====================================================================================

void KrossWordPuzzle::closeEvent(QCloseEvent* event)
{
    if (m_mainCrossword->isModified()) {
        QString msg = i18n("The current crossword has been modified.\nWould you like to save it?");

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
            if (dynamic_cast<LibraryManager*>(m_mainLibrary->libraryTree()->model())->isDir(index)) {
                QString folder = index.data(QFileSystemModel::FilePathRole).toString();
                m_mainLibrary->libraryAddCrossword(event->mimeData()->urls().at(0), folder); //TODO: handle more than 1 dropped crossword?
            }
            //else, if isn't a dir, just load it or add it to the parent folder?
        } else {
            QList<QUrl> urls = event->mimeData()->urls();
            loadFile(urls.first());
        }
    }
}

//=====================================================================================

QDialog* KrossWordPuzzle::createLoadProgressDialog()
{
    QDialog *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    // TODO: No max/min buttons
    dialog->setWindowTitle(i18n("Loading..."));

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QLabel *lblLoad = new QLabel(i18n("Loading the crossword, please wait..."), this);
    layout->addWidget(lblLoad);
    dialog->setLayout(layout);

    dialog->setModal(true);
    return dialog;
}

void KrossWordPuzzle::setupMainTabWidget()
{
    m_mainLibrary      = new LibraryGui(this);
    int indexLibrary   = m_mainStackedBar->addWidget(m_mainLibrary);
    m_mainCrossword    = new CrossWordXmlGuiWindow(this);
    m_mainStackedBar->addWidget(m_mainCrossword);

    m_mainCrossword->krossWord()->setAnimationEnabled(hasAnimationFromSettings());

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
    QAction *settingsMenu = this->menuBar()->actions().at(1);
    foreach(QAction * action, m_mainCrossword->menuBar()->actions()) {                              
        if (action->menu() && (action->menu()->objectName() == "edit" ||
                               action->menu()->objectName() == "move" ||
                               action->menu()->objectName() == "view")) {
            this->menuBar()->insertMenu(settingsMenu, action->menu());
        }
    }

    m_mainStackedBar->setCurrentIndex(indexLibrary);
    connect(m_mainStackedBar, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
    currentTabChanged(indexLibrary);
}

void KrossWordPuzzle::setupPlaces()
{
    QUrl libraryUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + "library");
    QUrl templatesUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + "templates");

    KFilePlacesModel *placesModel = new KFilePlacesModel();
    if (placesModel->url(placesModel->closestItem(libraryUrl)) != libraryUrl) {
        placesModel->addPlace(i18n("Library"),libraryUrl, "favorites", QApplication::applicationName());
    }

    if (placesModel->url(placesModel->closestItem(templatesUrl)) != templatesUrl) {
        placesModel->addPlace(i18n("Templates"), templatesUrl, "krossword", QApplication::applicationName());
    }

    delete placesModel;
}

void KrossWordPuzzle::setupActions()
{
    KStandardAction::showStatusbar(this, SLOT(showStatusbarGlobal(bool)), actionCollection());
    KStandardAction::keyBindings(this, SLOT(configureShortcutsGlobal()), actionCollection());
    KStandardAction::configureToolbars(this, SLOT(configureToolbarsGlobal()), actionCollection());
    KStandardAction::preferences(this, SLOT(optionsPreferencesSlot()), actionCollection());
    KStandardGameAction::quit(qApp, SLOT(closeAllWindows()), actionCollection());
}

void KrossWordPuzzle::showRestoreOption(const QString& lastUnsavedFileBeforeCrash)
{
    KGuiItem restoreButton(i18n("&Restore"), QIcon::fromTheme(QStringLiteral("document-open")), i18n("Restore the automatically saved version of an edited crossword before the crash"));
    int result = KMessageBox::questionYesNo(this, i18n("An unsaved crossword has been found. Most likely the game crashed, sorry!\n "
                                            "Do you want to restore the crossword?"),
                                            QString(),
                                            restoreButton,
                                            KStandardGuiItem::discard());
    if (result == KMessageBox::Yes) {
        loadFile(QUrl::fromLocalFile(lastUnsavedFileBeforeCrash), Crossword::KrossWord::KrossWordPuzzleCompressedXmlFile, true);
    } else {
        m_mainCrossword->removeTempFile(lastUnsavedFileBeforeCrash);
    }
}

QString KrossWordPuzzle::displayFileName(const QString &fileName)
{
    QString libraryDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + "library";
    if (fileName.startsWith(libraryDir)) {
        // Cut the library path
        QString libraryFileName = fileName.mid(libraryDir.length());
        libraryFileName.prepend(i18nc("This string is used to replace the library path "
                                      "for crossword files that are in the library with a shorter user visible string, "
                                      "e.g. replacing ~/.kde4/apps/krossword/library/crossword.kwpz with "
                                      "Library/crossword.kwpz", "Library") + QDir::separator());
        return libraryFileName;
    } else {
        return fileName;
    }
}

//=====================================================================================

void KrossWordPuzzle::loadSlot(const QUrl &url)
{
    loadFile(url);
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

    KgThemeSelector *themeSelector = new KgThemeSelector(KrosswordRenderer::self()->getThemeProvider(), KgThemeSelector::DefaultBehavior, themeSelectorDlg);
    dialog->addPage(themeSelector, i18n("Theme"), "games-config-theme");

    connect(KrosswordRenderer::self()->getThemeProvider(), SIGNAL(currentThemeChanged(const KgTheme*)), m_mainCrossword, SLOT(updateTheme()));
    connect(dialog, SIGNAL(settingsChanged(QString)), this, SLOT(settingsChanged()));
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    dialog->show();
}

void KrossWordPuzzle::settingsChanged()
{
    m_mainCrossword->updateTheme();

    m_mainCrossword->krossWord()->setAnimationEnabled(hasAnimationFromSettings());
}

bool KrossWordPuzzle::hasAnimationFromSettings()
{
    if (!Settings::animate()) {
        return false;
    }

    return true;
}

void KrossWordPuzzle::showStatusbarGlobal(bool show)
{
    if (m_mainStackedBar->currentWidget() == m_mainCrossword) {
        m_mainCrossword->statusBar()->setVisible(show);
    } else {
        m_mainLibrary->statusBar()->setVisible(show);
    }
}

int KrossWordPuzzle::configureShortcutsGlobal()
{
    KShortcutsDialog dlg(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);
    if (m_mainStackedBar->currentWidget() == m_mainCrossword) {
        dlg.addCollection(m_mainCrossword->actionCollection());
    } else {
        dlg.addCollection(m_mainLibrary->actionCollection());
    }

    return dlg.configure(true);
}

void KrossWordPuzzle::configureToolbarsGlobal()
{
    if (m_mainStackedBar->currentWidget() == m_mainCrossword) {
        m_mainCrossword->configureToolbars();
    } else {
        m_mainLibrary->configureToolbars();
    }
}

void KrossWordPuzzle::currentTabChanged(int index)
{
    bool crosswordTabShown = index == m_mainStackedBar->indexOf(m_mainCrossword);

    foreach(QAction * action, this->menuBar()->actions()) {
        if (action->menu() && (action->menu()->objectName() == "edit" ||
                               action->menu()->objectName() == "move" ||
                               action->menu()->objectName() == "view")) {
            action->setEnabled(crosswordTabShown);
        }
    }

    unplugActionList("library_game_list");
    unplugActionList("crossword_game_list");
    unplugActionList("options_list");

    QList<QAction*> libraryGameList;
    QList<QAction*> crosswordGameList;
    QList<QAction*> optionsList;

    QAction *separator = new QAction(this);
    QAction *separator2 = new QAction(this);
    separator->setSeparator(true);
    separator2->setSeparator(true);

    if (crosswordTabShown) {
        setCaption(m_caption, m_mainCrossword->isModified());

        crosswordGameList << m_mainCrossword->action("game_save");
        crosswordGameList << m_mainCrossword->action("game_save_as");
        crosswordGameList << m_mainCrossword->action("game_save_template_as");
        crosswordGameList << separator;
        crosswordGameList << m_mainCrossword->action("game_print");
        crosswordGameList << m_mainCrossword->action("game_print_preview");
        crosswordGameList << separator2;
        crosswordGameList << m_mainCrossword->action("game_close");

        optionsList << m_mainCrossword->action(m_mainCrossword->actionName(CrossWordXmlGuiWindow::Options_Dictionaries));
        optionsList << separator;

        optionsList << m_mainCrossword->toolBarMenuAction();
        optionsList << m_mainCrossword->action(m_mainCrossword->actionName(CrossWordXmlGuiWindow::ShowClueDock));
        optionsList << m_mainCrossword->action(m_mainCrossword->actionName(CrossWordXmlGuiWindow::ShowUndoViewDock));
        optionsList << m_mainCrossword->action(m_mainCrossword->actionName(CrossWordXmlGuiWindow::ShowCurrentCellDock));
    } else { // tabLibrary
        setCaption(i18n("Library"));

        libraryGameList << m_mainLibrary->action("library_create_crossword");
        libraryGameList << m_mainLibrary->action("library_new_folder");
        libraryGameList << separator;
        libraryGameList << m_mainLibrary->action("library_download");
        libraryGameList << m_mainLibrary->action("library_add");
        libraryGameList << m_mainLibrary->action("library_delete");
        libraryGameList << separator2;
        libraryGameList << m_mainLibrary->action("library_export");


        optionsList << m_mainCrossword->action(m_mainCrossword->actionName(CrossWordXmlGuiWindow::Options_Dictionaries));
        optionsList << separator;

        optionsList << m_mainLibrary->toolBarMenuAction();
    }

    plugActionList("library_game_list", libraryGameList);
    plugActionList("crossword_game_list", crosswordGameList);
    plugActionList("options_list", optionsList);
}

void KrossWordPuzzle::crosswordLoadingComplete(const QString& fileName)
{
    Q_UNUSED(fileName);

    if (m_loadProgressDialog) // When loading a template there is no load progress dialog
        m_loadProgressDialog->close();

    int indexCrossword = m_mainStackedBar->indexOf(m_mainCrossword);
    m_mainStackedBar->setCurrentIndex(indexCrossword);
}

void KrossWordPuzzle::crosswordErrorLoading(const QString& fileName)
{
    m_loadProgressDialog->close();
    KMessageBox::error(this, i18n("Error loading file '%1'", fileName));
}

void KrossWordPuzzle::crosswordClosed(const QString& fileName)
{
    Q_UNUSED(fileName);

    m_mainStackedBar->setCurrentWidget(m_mainLibrary);
}

void KrossWordPuzzle::crosswordCurrentChanged(const QString& fileName, const QString& oldFileName)
{
    Q_UNUSED(oldFileName);

    if (fileName.isEmpty()) {
        m_caption.clear();
    } else {
        stateChanged("no_file_opened", StateReverse);

        if (m_mainCrossword->krossWord()->getTitle().isEmpty())
            m_caption = displayFileName(fileName);
        else
            m_caption = m_mainCrossword->krossWord()->getTitle();
    }

    setCaption(m_caption, m_mainCrossword->isModified());
}

void KrossWordPuzzle::crosswordModificationsChanged(CrossWordXmlGuiWindow::ModificationTypes modificationTypes)
{
    Q_UNUSED(modificationTypes);
    crosswordCurrentChanged(m_mainCrossword->currentFileName(), m_mainCrossword->currentFileName());
}

void KrossWordPuzzle::crosswordAutoSaveFileChanged(const QString &fileName)
{
    Settings::setLastUnsavedFileBeforeCrash(fileName);
    Settings::self()->save();
}
