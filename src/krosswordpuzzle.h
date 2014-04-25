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

#ifndef KROSSWORDPUZZLE_H
#define KROSSWORDPUZZLE_H

#include <kxmlguiwindow.h>

#include "ui_settings.h"
// #include "ui_print_crossword.h"

#include "krossword.h"
#include "crosswordxmlguiwindow.h"
#include "krosswordtheme.h"

#include <KgThemeProvider>

namespace KIO
{
class PreviewJob;
}
class LibraryXmlGuiWindow;
class QTreeView;
class KRecentFilesAction;
class KUndoStack;
class KUrl;
class KrosswordRenderer;

class MenuTabWidget;
/**
 * This class serves as the main window for KrossWordPuzzle.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Friedrich P?lz <fpuelz@gmx.de>
 * @version %{VERSION}
 */
class KrossWordPuzzle : public KXmlGuiWindow
{
    Q_OBJECT

public:
    /*
    enum Action {
        Game_Download,
        Game_Upload,
        //  Options_Themes,
        RecentTab_RecentFilesRemove
    };
    */

    KrossWordPuzzle();
    virtual ~KrossWordPuzzle() {}

    void loadFile(const KUrl &url, Crossword::KrossWord::FileFormat fileFormat = Crossword::KrossWord::DetermineByFileName, bool loadCrashedFile = false);

    bool createNewCrossWord(const Crossword::CrosswordTypeInfo &crosswordTypeInfo, const QSize &crosswordSize,
                            const QString &title, const QString &authors, const QString &copyright, const QString &notes);

    bool createNewCrossWordFromTemplate(const QString &templateFilePath, const QString &title,
                                        const QString &authors, const QString &copyright, const QString &notes);

    /** Checks if the crossword with the given filename is in the library. */
    bool isFileInLibrary(const QString &fileName);

    //! Unused methods
    //const char *actionName(Action action) const;
    //CrossWordXmlGuiWindow *mainCrossword() const;

protected:
    virtual void closeEvent(QCloseEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent* event);

public slots:
    // Game actions
    //void gameNewSlot();
    void downloadSlot();
    void uploadSlot();

    void loadSlot(const KUrl &url = KUrl());
    //void loadRecentSlot(const KUrl &url); //Useless
    void loadFile(const QString &fileName);
    void saveSlot();
    void saveAsSlot();

    // Settings actions
    void optionsPreferencesSlot();
    void settingsChanged();
    AnimationTypes animationTypesFromSettings();
    // void changeThemeSlot( int themeId );

    void showStatusbarGlobal(bool show);
    int  configureShortcutsGlobal();
    void configureToolbarsGlobal();
    void currentTabChanged(int);

protected slots:
    void crosswordLoadingComplete(const QString &fileName);
    void crosswordErrorLoading(const QString &fileName);
    void crosswordCurrentChanged(const QString &fileName, const QString &oldFileName);
    void crosswordClosed(const QString &fileName);
    void crosswordModificationsChanged(CrossWordXmlGuiWindow::ModificationTypes modificationTypes);
    void crosswordAutoSaveFileChanged(const QString &fileName);

//     void loadRecentItem();
//     void recentFileExecuted( QListWidgetItem *item );
//     void recentFileListContextMenuRequested( const QPoint &pos );

// Popup menu actions
//     void recentFilesRemoveSlot();
//     void recentFilesClearSlot();

private:
    KDialog* createLoadProgressDialog();
    void setupMainTabWidget();
//     QMenu *popupMenuRecentFilesList();   // Unused
    void setupPlaces();
    void setupActions();
    void showRestoreOption(const QString &lastUnsavedFileBeforeCrash);

    /** Removes the path for crosswords that are in the library. */
    static QString displayFileName(const QString &fileName);

    Ui::settings           ui_settings;

    LibraryXmlGuiWindow   *m_mainLibrary;           //Owned
    CrossWordXmlGuiWindow *m_mainCrossword;         //Owned

    KDialog               *m_loadProgressDialog;    //Owned
    // KRecentFilesAction    *m_recentFilesAction;  //Unused
    QList<KToolBar*>       m_hiddenToolBars;
    QString                m_caption;
    MenuTabWidget         *m_mainTabBar;            //Owned
};

#endif // _KROSSWORDPUZZLE_H_
