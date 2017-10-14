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

#ifndef LIBRARYGUI_H
#define LIBRARYGUI_H

#include "ui_export_to_image.h"
#include "ui_create_new.h"
#include "ui_download.h"

#include "library/librarymanager.h"

#include <QTemporaryFile>
#include <KXmlGuiWindow>

namespace KIO
{
class PreviewJob;
}

class HtmlDelegate;
class KrossWordPuzzle;

class LibraryGui : public KXmlGuiWindow
{
    Q_OBJECT

public:
    enum Action {
        Library_CreateCrossword,
        Library_Add,
        Library_Delete,
        Library_NewFolder,
        Library_Export,
        Library_Download
    };

    enum DownloadProvider {
        JonesinCrosswords,
        WallStreetJournal,
        ChronicleHigherEducation,
        CrossNerd,
        /*SwearCrossword,*/
        Motscroisesch/*,
        ChrisWords*/
    };

    LibraryGui(KrossWordPuzzle* parent = 0);
    virtual ~LibraryGui() { }

    QTreeView* libraryTree() const;

    const char *actionName(Action actionEnum) const;

    LibraryManager* libraryManager() const;

public slots:
    void libraryAddCrossword(const QUrl &url, const QString &folder = QString());

protected slots:
    void downloadPreviewJobGotPreview(const KFileItem &fi, const QPixmap &pix);
    void downloadPreviewJobFailed(const KFileItem &fi);

    void downloadProviderChanged(int index);
    void downloadCurrentCrosswordChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void downloadCrosswordResult(KJob *job);

    void libraryOpenItem(const QModelIndex &index);
    void libraryItemDoubleClicked(const QModelIndex &index);
    void libraryCurrentChanged(const QModelIndex &current, const QModelIndex &previous);

    void libraryCreateCrosswordSlot();
    void libraryAddSlot();
    void libraryDeleteSlot();
    void libraryNewFolderSlot();
    void libraryExportSlot();
    void libraryDownloadSlot();

private:
    Ui::create_new ui_create_new;
    Ui::export_to_image ui_export_to_image;
    Ui::download ui_download;

    KrossWordPuzzle *m_mainWindow;

    QTreeView *m_libraryTree;
    HtmlDelegate *m_libraryDelegate;
    LibraryManager *m_libraryModel;
    QModelIndex m_libraryPopupIndex;

    QScopedPointer<QTemporaryFile> m_downloadedCrossword;
    KIO::PreviewJob *m_downloadPreviewJob;
    QDialog *m_downloadCrosswordsDlg;

    void setupActions();

    void getDownloadCrosswordItems(const QString &rawUrl, const QDate& startDate, const QDate& endDate, int dayOffset);

    static QList<DownloadProvider> allDownloadProviders();
};

#endif // LIBRARYGUI_H
