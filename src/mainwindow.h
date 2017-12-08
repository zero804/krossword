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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <kxmlguiwindow.h>

#include "ui_settings.h"
#include "gamegui.h"

#include "dictionary.h"

namespace KIO
{
class PreviewJob;
}

class LibraryGui;
class QUrl;
class QStackedWidget;

/**
 * This class serves as the main window for KrossWordPuzzle.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Friedrich P?lz <fpuelz@gmx.de>
 * @version %{VERSION}
 */
class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    MainWindow();
    virtual ~MainWindow();

    QSize sizeHint() const;

    void loadFile(const QUrl &url, Crossword::KrossWord::FileFormat fileFormat = Crossword::KrossWord::DetermineByType, bool loadCrashedFile = false);

    bool createNewCrossWord(const Crossword::CrosswordTypeInfo &crosswordTypeInfo, const QSize &crosswordSize,
                            const QString &title, const QString &authors, const QString &copyright, const QString &notes);

    bool createNewCrossWordFromTemplate(const QString &templateFilePath, const QString &title,
                                        const QString &authors, const QString &copyright, const QString &notes);

    void setupGameGui();

    Dictionary* getDictionary();
    
protected:
    virtual void closeEvent(QCloseEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent* event);

public slots:
    void loadSlot(const QUrl &url/* = QUrl::fromLocalFile()*/);
    //void loadFile(const QString &fileName);

    // Settings actions
    void optionsPreferencesSlot();
    void optionsDictionarySlot();
    void settingsChanged();

    void showStatusbarGlobal(bool show);
    int  configureShortcutsGlobal();
    void configureToolbarsGlobal();
    void currentTabChanged(int);

protected slots:
    void crosswordCurrentChanged(const QString &fileName, const QString &oldFileName);
    void crosswordClosed(const QString &fileName);
    void crosswordModificationsChanged(GameGui::ModificationTypes modificationTypes);
    void crosswordAutoSaveFileChanged(const QString &fileName);

private:
    void setupMainTabWidget();
    void setupPlaces();
    void setupActions();
    void showRestoreOption(const QString &lastUnsavedFileBeforeCrash);

    /** Removes the path for crosswords that are in the library. */
    static QString displayFileName(const QString &fileName);

    Ui::settings ui_settings;

    LibraryGui *m_libraryGui;         //Owned
    GameGui *m_gameGui;               //Owned

    QString m_caption;
    QStackedWidget *m_mainStackedBar; //Owned

    Dictionary *m_dictionary;         // Owned
};

#endif // _MAINWINDOW_H_
