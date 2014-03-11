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

#include "crosswordpreviewwidget.h"
#include <kfileitem.h>
#include <KIO/PreviewJob>
#include <KLocalizedString>


CrosswordPreviewWidget::CrosswordPreviewWidget( QWidget* parent )
	: QLabel( parent ), m_previewJob( 0 ) {
  m_previewSize = QSize( 256, 256 );
}

void CrosswordPreviewWidget::showPreview( const QString& fileName,
					  const QString& mimeType ) {    
  KFileItem crossword( KUrl(fileName), mimeType, KFileItem::Unknown );
  m_previewJob = new KIO::PreviewJob( KFileItemList() << crossword,
				      m_previewSize.width(), m_previewSize.height(),
				      0, 1, false, true, 0 );
  m_previewJob->setAutoDelete( true );
  connect( m_previewJob, SIGNAL(gotPreview(KFileItem,QPixmap)),
	this, SLOT(previewJobGotPreview(KFileItem,QPixmap)) );
  connect( m_previewJob, SIGNAL(failed(KFileItem)),
	this, SLOT(previewJobFailed(KFileItem)) );
  m_previewJob->start();

  setText( i18n("Loading preview...") );
  setPixmap( QPixmap() );
  setEnabled( false );
}

void CrosswordPreviewWidget::previewJobFailed( const KFileItem& fileItem ) {
  Q_UNUSED( fileItem );
  
  setText( i18n("Preview failed") );
  setPixmap( QPixmap() );
  setEnabled( false );
}

void CrosswordPreviewWidget::previewJobGotPreview( const KFileItem& fileItem,
						   const QPixmap& pixmap ) {
  Q_UNUSED( fileItem );
  
  setText( QString() );
  setPixmap( pixmap );
  setEnabled( true );
}

