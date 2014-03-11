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

#ifndef LIBRARYXMLGUIWINDOW_H
#define LIBRARYXMLGUIWINDOW_H

// #include "ui_print_crossword.h"
#include "ui_export_to_image.h"
#include "ui_create_new.h"
#include "ui_download.h"

#include <KXmlGuiWindow>
#include <QFileInfo>

namespace KIO {
  class PreviewJob;
}

class HtmlDelegate;
class QStandardItemModel;
class LibraryModel;
class KrossWordPuzzle;
class QTreeView;
class QStandardItem;
class KFileItem;

class LibraryXmlGuiWindow : public KXmlGuiWindow {
    Q_OBJECT

    public:
	enum Action {
	    Library_Open,
	    Library_Import,
	    Library_Export,
	    Library_Download,
	    Library_Delete,
	    Library_NewFolder,
	    Library_NewCrossword,
	    Library_Update
	};

	enum DownloadProvider {
	    HoustonChronicle,
	    JonesinCrosswords,
	    BostonGlobe,
	    OnionCrosswords,
	    WallStreetJournal,
	    WashingtonPost
	};

	static QList<DownloadProvider> allDownloadProviders() {
	    return QList<DownloadProvider>() << HoustonChronicle
		<< JonesinCrosswords << BostonGlobe << OnionCrosswords
		<< WallStreetJournal << WashingtonPost;
	};

	LibraryXmlGuiWindow( KrossWordPuzzle* parent = 0 );

	QTreeView *libraryTree() const { return m_libraryTree; };
	const char *actionName( Action actionEnum ) const;

	KrossWordPuzzle *mainWindow() const { return m_mainWindow; };
	void setMainWindow( KrossWordPuzzle *mainWindow ) { m_mainWindow = mainWindow; };

// 		void setLibraryTree( QTreeView *treeTree ) { m_libraryTree = libraryTree; };

    public slots:
	void libraryAddCrossword( const KUrl &url, const QString &subFolder = QString() ) {
	  libraryAddCrossword( QList<QUrl>() << url, subFolder ); };
	void libraryAddCrossword( const QList<QUrl> &urls, const QString &subFolder = QString() );

    protected slots:
// 	void createNewCurrentCrosswordTypeChanged(
// 		    const QModelIndex &current, const QModelIndex &previous );
// 	void createNewConfigureDetailsClicked();

	void previewJobGotPreview( const KFileItem &fi, const QPixmap &pix );
	void previewJobFailed( const KFileItem &fi );

	void downloadPreviewJobGotPreview( const KFileItem &fi, const QPixmap &pix );
	void downloadPreviewJobFailed( const KFileItem &fi );

	void downloadProviderChanged( int index );
	void downloadCurrentCrosswordChanged(
		QTreeWidgetItem *current, QTreeWidgetItem *previous );

	void libraryItemChanged( QStandardItem *item );
	void libraryTreeContextMenuRequested( const QPoint &pos );
	void libraryItemDoubleClicked( const QModelIndex &index ) {
	  libraryOpenItem( index ); };
	void libraryOpenItem( const QModelIndex &index );
	void libraryCurrentChanged( const QModelIndex &current,
								const QModelIndex &previous );
	void librarySetAsSubDirForDownloads();

	void libraryOpenSlot() {
	  libraryOpenItem( m_libraryTree->currentIndex() ); };
	void libraryImportSlot();
	void libraryExportSlot();
	void libraryDownloadSlot();
	void libraryDeleteSlot();
	void libraryNewFolderSlot();
	void libraryNewCrosswordSlot();
	void libraryUpdateSlot() {
	  fillLibrary(); };

    private:
	void setupActions();

	void fillLibrary();
	QString additionsColorCSS();
	QString libraryFolderText( const QString &path, int crosswordCountOffset = 0 );

	QList<QTreeWidgetItem*> getDownloadCrosswordItems(
		const QString &rawUrl, const QDate& startDate,
		const QDate& endDate, int dayOffset, const KIcon &puzIcon );

	Ui::create_new ui_create_new;
	Ui::export_to_image ui_export_to_image;
	Ui::download ui_download;

	QTreeView *m_libraryTree;
	KrossWordPuzzle *m_mainWindow;
	KDialog *m_dialog;

	QModelIndex m_libraryPopupIndex;
	QStandardItemModel *m_libraryModel;
	HtmlDelegate *m_libraryDelegate;
	KIO::PreviewJob *m_previewJob, *m_downloadPreviewJob;
};

#endif // LIBRARYXMLGUIWINDOW_H
