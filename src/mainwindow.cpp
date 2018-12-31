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

#include "mainwindow.h"
#include "krosswordrenderer.h"
#include "settings.h"
#include "library/librarygui.h"
#include "dialogs/dictionarydialog.h"
#include "extendedsqltablemodel.h"

#include <KConfigDialog>
#include <KMessageBox>
#include <QStackedWidget>
#include <KgThemeSelector>
#include <KShortcutsDialog>

// KDE action includes
#include <KStandardAction>
#include <KStandardGameAction>
#include <KActionCollection>

#include <KFilePlacesModel>
#include <QStandardPaths>
//#include <KIO/CopyJob> //for copying templates

#include <QMenuBar>
#include <QStatusBar>
#include <QDesktopWidget>
#include <QMimeDatabase>
#include <QMimeData>

MainWindow::MainWindow() : KXmlGuiWindow(),
      m_mainStackedBar(new QStackedWidget(this)),
      m_libraryGui(new LibraryGui(this)),
      m_gameGui(nullptr),
      m_loadDialog(nullptr),
      m_dictionary(new Dictionary)
{
    setAcceptDrops(true);
    setObjectName("mainWindow");
    setupPlaces();

    setupActions();
    setupGUI(Save | Create);

    connect(m_mainStackedBar, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
    m_mainStackedBar->addWidget(m_libraryGui);
    setCentralWidget(m_mainStackedBar);

    QString lastUnsavedFileBeforeCrash = Settings::lastUnsavedFileBeforeCrash();
    if (!lastUnsavedFileBeforeCrash.isEmpty()) {
        // check it's a .kwpz as expected
        QMimeDatabase mimeDatabase;
        QMimeType mimeType = mimeDatabase.mimeTypeForFile(lastUnsavedFileBeforeCrash, QMimeDatabase::MatchDefault);
        if (mimeType.inherits("application/x-krosswordpuzzle-compressed")) {
            showRestoreOption(lastUnsavedFileBeforeCrash);
        }
    }
}

MainWindow::~MainWindow()
{
    delete m_dictionary;
}

QSize MainWindow::sizeHint() const
{
    return KXmlGuiWindow::sizeHint().expandedTo(QDesktopWidget().availableGeometry().size() * 0.7);
}

QDialog* MainWindow::createLoadDialog()
{
    QDialog *dialog = new QDialog(this);
    //dialog->setWindowFlag(Qt::Dialog);  // TODO: No max/min buttons
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(i18n("Loading..."));
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QLabel *label = new QLabel(i18n("Loading..."), this);
    layout->addWidget(label);
    dialog->setLayout(layout);
    dialog->setModal(true);

    return dialog;
}

void MainWindow::loadFile(const QUrl &url, bool loadCrashedFile)
{
    setupGameGui();

    m_loadDialog = createLoadDialog();
    m_loadDialog->show();

    if (m_gameGui->loadFile(url, loadCrashedFile)) {
        if (!m_libraryGui->libraryManager()->isInLibrary(url.path())) {
            if (KMessageBox::questionYesNo(this,
                                           i18n("Would you like to add the crossword into the library?"),
                                           i18n("Save crossword"),
                                           KStandardGuiItem::yes(), KStandardGuiItem::no())
                                           == KMessageBox::Yes) {
                m_libraryGui->libraryAddCrossword(url);
            }
        }
    } else {
        KMessageBox::sorry(this,
                           i18n("Could not open resource at ") + url.url(QUrl::PreferLocalFile),
                           i18n("Resource unavailable"));
    }
}

void MainWindow::createNewCrossWord(const Crossword::CrosswordTypeInfo &crosswordTypeInfo,
                                         const QSize &crosswordSize, const QString& title,
                                         const QString& authors, const QString& copyright,
                                         const QString& notes)
{
    setupGameGui();

    m_gameGui->createNewCrossWord(crosswordTypeInfo, crosswordSize, title, authors, copyright, notes);
}

bool MainWindow::createNewCrossWordFromTemplate(const QString& templateFilePath,
                                                const QString& title, const QString& authors,
                                                const QString& copyright, const QString& notes)
{
    setupGameGui();

    if (m_gameGui->createNewCrossWordFromTemplate(templateFilePath, title, authors, copyright, notes)) {
        return true;
    }

    return false;
}

//=====================================================================================

void MainWindow::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event);

    if (m_gameGui) {
        m_gameGui->closeSlot();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->accept();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        QPoint pt         = m_libraryGui->libraryTree()->mapFrom(this, event->pos());
        QModelIndex index = m_libraryGui->libraryTree()->indexAt(pt);

        if (index.isValid()) {
            if (dynamic_cast<LibraryManager*>(m_libraryGui->libraryTree()->model())->isDir(index)) {
                QString folder = index.data(QFileSystemModel::FilePathRole).toString();
                m_libraryGui->libraryAddCrossword(event->mimeData()->urls().at(0), folder); //TODO: handle more than 1 dropped crossword?
            }
            //else, if isn't a dir, just load it or add it to the parent folder?
        } else {
            QList<QUrl> urls = event->mimeData()->urls();
            loadFile(urls.first());
        }
    }
}

//=====================================================================================

void MainWindow::setupGameGui()
{
    m_gameGui = new GameGui(this);
    m_mainStackedBar->addWidget(m_gameGui);

    m_gameGui->krossWord()->setAnimationEnabled(Settings::animate());

    connect(m_gameGui, SIGNAL(gameReady()),
            this, SLOT(showGame()));

    connect(m_gameGui, SIGNAL(currentFileChanged(QString, QString)),
            this, SLOT(crosswordCurrentChanged(QString, QString)));

    connect(m_gameGui, SIGNAL(fileClosed(QString)),
            this, SLOT(crosswordClosed(QString)));

    connect(m_gameGui, SIGNAL(modificationTypesChanged(GameGui::ModificationTypes)),
            this, SLOT(crosswordModificationsChanged(GameGui::ModificationTypes)));

    connect(m_gameGui, SIGNAL(tempAutoSaveFileChanged(QString)),
            this, SLOT(crosswordAutoSaveFileChanged(QString)));
}

Dictionary* MainWindow::getDictionary()
{
    return m_dictionary;
}

void MainWindow::setupPlaces()
{
    QUrl libraryDir = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                          + QLatin1Char('/') + "library");
    if (!QDir(libraryDir.toString(QUrl::PreferLocalFile)).exists()) {
        QDir().mkpath(libraryDir.toString(QUrl::PreferLocalFile));
    }

    QUrl templateDir = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                            + QLatin1Char('/') + "templates");
    if (!QDir(templateDir.toString(QUrl::PreferLocalFile)).exists()) {
        //QString sysTemplateDir = QStandardPaths::locate(QStandardPaths::AppDataLocation, "templates", QStandardPaths::LocateDirectory);
        QDir().mkpath(templateDir.toString(QUrl::PreferLocalFile));

        /*
        // creating subdirectories if needed
        QDirIterator it(sysTemplateDir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString subDir = it.next().remove(sysTemplateDir);
            QDir().mkpath(templateDir.toString(QUrl::PreferLocalFile) + subDir);
        }

        // copy templates
        QDirIterator itT(sysTemplateDir, QStringList() << "*.kwpz", QDir::Files, QDirIterator::Subdirectories);
        while (itT.hasNext()) {
            QString dest = itT.next().remove(sysTemplateDir);
            KIO::copy(QUrl::fromLocalFile(itT.filePath()), QUrl::fromLocalFile(templateDir.toString(QUrl::PreferLocalFile) + dest), KIO::HideProgressInfo);
        }

        qDebug() << "Created local templates directory and copied default crossword templates.";
        */
    }

    KFilePlacesModel *placesModel = new KFilePlacesModel();
    if (placesModel->url(placesModel->closestItem(libraryDir)) != libraryDir) {
        placesModel->addPlace(i18n("Library"),libraryDir, "favorites", QApplication::applicationName());
    }

    if (placesModel->url(placesModel->closestItem(templateDir)) != templateDir) {
        placesModel->addPlace(i18n("Templates"), templateDir, "krossword", QApplication::applicationName());
    }

    delete placesModel;
}

void MainWindow::setupActions()
{
    KStandardAction::showStatusbar(this, SLOT(showStatusbarGlobal(bool)), actionCollection());
    KStandardAction::keyBindings(this, SLOT(configureShortcutsGlobal()), actionCollection());
    KStandardAction::configureToolbars(this, SLOT(configureToolbarsGlobal()), actionCollection());
    KStandardAction::preferences(this, SLOT(optionsPreferencesSlot()), actionCollection());
    KStandardGameAction::quit(qApp, SLOT(closeAllWindows()), actionCollection());

    // Settings
    QAction *dictionaryAction = new QAction(QIcon::fromTheme(QStringLiteral("crossword-dictionary")), i18n("&Dictionary..."), this);
    dictionaryAction->setToolTip(i18n("Add or remove crossword dictionaries"));
    actionCollection()->addAction("options_dictionaries", dictionaryAction);
    connect(dictionaryAction, SIGNAL(triggered()), this, SLOT(optionsDictionarySlot()));
}

void MainWindow::showRestoreOption(const QString& lastUnsavedFileBeforeCrash)
{
    KGuiItem restoreButton(i18n("&Restore"), QIcon::fromTheme(QStringLiteral("document-open")), i18n("Restore the automatically saved version of an edited crossword before the crash"));
    int result = KMessageBox::questionYesNo(this, i18n("An unsaved crossword has been found. Most likely the game crashed, sorry!\n "
                                            "Do you want to restore the crossword?"),
                                            QString(),
                                            restoreButton,
                                            KStandardGuiItem::discard());
    if (result == KMessageBox::Yes) {
        loadFile(QUrl::fromLocalFile(lastUnsavedFileBeforeCrash), true);
    }

    //m_gameGui->removeTempFile(lastUnsavedFileBeforeCrash);
    QFile::remove(lastUnsavedFileBeforeCrash);
    crosswordAutoSaveFileChanged(QString());
}

QString MainWindow::displayFileName(const QString &fileName)
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

void MainWindow::optionsPreferencesSlot()
{
    // Avoid to have 2 dialogs shown
    if (KConfigDialog::showDialog("settings")) {
        return;
    }

    KConfigDialog *dialog = new KConfigDialog(this, "settings", Settings::self());

    QWidget *animationSettingsDlg = new QWidget;
    ui_settings.setupUi(animationSettingsDlg);
    dialog->addPage(animationSettingsDlg, i18n("General"), "package_settings");

    QWidget *themeSelectorDlg = new QWidget;

    KgThemeSelector *themeSelector = new KgThemeSelector(KrosswordRenderer::self()->getThemeProvider(), KgThemeSelector::DefaultBehavior, themeSelectorDlg);
    dialog->addPage(themeSelector, i18n("Theme"), "games-config-theme");

    connect(KrosswordRenderer::self()->getThemeProvider(), SIGNAL(currentThemeChanged(const KgTheme*)), m_gameGui, SLOT(updateTheme()));
    connect(dialog, SIGNAL(settingsChanged(QString)), this, SLOT(settingsChanged()));
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    dialog->show();
}

void MainWindow::optionsDictionarySlot()
{
    if (!m_dictionary->openDatabase(this)) {
        KMessageBox::error(this, i18n("Couldn't connect to the database."));
        return;
    }

    QPointer<DictionaryDialog> dialog = new DictionaryDialog(m_dictionary, this);
    dialog->exec();
    dialog->databaseTable()->submitAll();
    //m_dictionary->closeDatabase();
    delete dialog;
}

void MainWindow::settingsChanged()
{
    m_gameGui->updateTheme();

    m_gameGui->krossWord()->setAnimationEnabled(Settings::animate());
}

void MainWindow::showStatusbarGlobal(bool show)
{
    if (m_mainStackedBar->currentWidget() == m_gameGui) {
        m_gameGui->statusBar()->setVisible(show);
    } else {
        m_libraryGui->statusBar()->setVisible(show);
    }
}

int MainWindow::configureShortcutsGlobal()
{
    KShortcutsDialog dlg(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);
    if (m_mainStackedBar->currentWidget() == m_gameGui) {
        dlg.addCollection(m_gameGui->actionCollection());
    } else {
        dlg.addCollection(m_libraryGui->actionCollection());
    }

    return dlg.configure(true);
}

void MainWindow::configureToolbarsGlobal()
{
    if (m_mainStackedBar->currentWidget() == m_gameGui) {
        m_gameGui->configureToolbars();
    } else {
        m_libraryGui->configureToolbars();
    }
}

void MainWindow::currentTabChanged(int index)
{
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

    bool crosswordTabShown = (index == m_mainStackedBar->indexOf(m_gameGui));

    if (crosswordTabShown) {
        setCaption(m_caption, m_gameGui->isModified());

        // Add menus of the embedded crossword window to the menu bar of this (main) window
        QAction *settingsMenu = this->menuBar()->actions().at(1);
        foreach(QAction * action, m_gameGui->menuBar()->actions()) {
            if (action->menu() && (action->menu()->objectName() == "edit" ||
                                   action->menu()->objectName() == "move" ||
                                   action->menu()->objectName() == "view")) {
                this->menuBar()->insertMenu(settingsMenu, action->menu());
            }
        }

        crosswordGameList << m_gameGui->action("game_save");
        crosswordGameList << m_gameGui->action("game_save_as");
        crosswordGameList << m_gameGui->action("game_save_template_as");
        crosswordGameList << separator;
        crosswordGameList << m_gameGui->action("game_print");
        crosswordGameList << m_gameGui->action("game_print_preview");
        crosswordGameList << separator2;
        crosswordGameList << m_gameGui->action("game_close");

        optionsList << action("options_dictionaries");
        optionsList << separator;
        optionsList << m_gameGui->toolBarMenuAction();
        optionsList << m_gameGui->action(m_gameGui->actionName(GameGui::ShowClueDock));
        optionsList << m_gameGui->action(m_gameGui->actionName(GameGui::ShowUndoViewDock));
        optionsList << m_gameGui->action(m_gameGui->actionName(GameGui::ShowCurrentClueDock));
    } else { // tabLibrary
        setCaption(i18n("Library"));

        libraryGameList << m_libraryGui->action("library_create_crossword");
        libraryGameList << m_libraryGui->action("library_new_folder");
        libraryGameList << separator;
        libraryGameList << m_libraryGui->action("library_download");
        libraryGameList << m_libraryGui->action("library_add");
        libraryGameList << m_libraryGui->action("library_delete");
        libraryGameList << separator2;
        libraryGameList << m_libraryGui->action("library_export");

        optionsList << action("options_dictionaries");
        optionsList << separator;
        optionsList << m_libraryGui->toolBarMenuAction();
    }

    plugActionList("library_game_list", libraryGameList);
    plugActionList("crossword_game_list", crosswordGameList);
    plugActionList("options_list", optionsList);
}

void MainWindow::showGame()
{
    if (m_loadDialog) { // When loading a template there is no load progress dialog
        m_loadDialog->close(); // using Qt::WA_DeleteOnClose
        m_loadDialog = nullptr;
    }

    m_mainStackedBar->setCurrentWidget(m_gameGui);
}

void MainWindow::crosswordClosed(const QString& fileName)
{
    Q_UNUSED(fileName);

    m_mainStackedBar->setCurrentWidget(m_libraryGui);

    delete m_gameGui;
    m_gameGui = nullptr;
}

void MainWindow::crosswordCurrentChanged(const QString& fileName, const QString& oldFileName)
{
    Q_UNUSED(oldFileName);

    if (fileName.isEmpty()) {
        m_caption.clear();
    } else {
        stateChanged("no_file_opened", StateReverse);

        if (m_gameGui->krossWord()->getTitle().isEmpty()) {
            m_caption = displayFileName(fileName);
        } else {
            m_caption = m_gameGui->krossWord()->getTitle();
        }
    }

    setCaption(m_caption, m_gameGui->isModified());
}

void MainWindow::crosswordModificationsChanged(GameGui::ModificationTypes modificationTypes)
{
    Q_UNUSED(modificationTypes);
    crosswordCurrentChanged(m_gameGui->currentFileName(), m_gameGui->currentFileName());
}

void MainWindow::crosswordAutoSaveFileChanged(const QString &fileName)
{
    Settings::setLastUnsavedFileBeforeCrash(fileName);
    Settings::self()->save();
}
