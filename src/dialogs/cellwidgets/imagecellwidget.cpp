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

#include "imagecellwidget.h"

#include "../../krossword.h"
#include "../../cells/imagecell.h"
#include <KFileDialog>

ImageCellWidget::ImageCellWidget( ImageCell* imageCell, QWidget* parent )
				  : QWidget( parent ) {
  ui_image_properties_dock.setupUi( this );
  ui_image_properties_dock.browse->setIcon( KIcon("document-open") );
  setImageCell( imageCell );

  connect( ui_image_properties_dock.browse, SIGNAL(clicked()),
	   this, SLOT(browseClicked()) );
  connect( ui_image_properties_dock.horizontalCellSpan, SIGNAL(valueChanged(int)),
	   this, SLOT(horizontalCellSpanChanged(int)) );
  connect( ui_image_properties_dock.verticalCellSpan, SIGNAL(valueChanged(int)),
	   this, SLOT(verticalCellSpanChanged(int)) );
}

void ImageCellWidget::setImageCell( ImageCell* imageCell ) {
  Q_ASSERT( imageCell );

  if ( m_imageCell == imageCell )
    return;
  
  m_imageCell = imageCell;
  ui_image_properties_dock.fileName->setText( imageCell->url().pathOrUrl() );
  QSize maxSize = imageCell->krossWord()->emptyCellSpan(
	  imageCell->krossWord()->currentCell()->coord(),
	  dynamic_cast<SpannedCell*>(imageCell) );
  ui_image_properties_dock.horizontalCellSpan->setRange( 1, maxSize.width() );
  ui_image_properties_dock.verticalCellSpan->setRange( 1, maxSize.height() );
  ui_image_properties_dock.horizontalCellSpan->setValue( imageCell->horizontalCellSpan() );
  ui_image_properties_dock.verticalCellSpan->setValue( imageCell->verticalCellSpan() );
}

void ImageCellWidget::browseClicked() {
  KUrl imageUrl = KFileDialog::getOpenUrl( KUrl("kfiledialog:///openImage"),
      "image/gif image/x-xpm image/x-xbm image/jpeg image/x-bmp image/png "
      "image/x-ico image/x-portable-bitmap image/x-portable-pixmap "
      "image/x-portable-greymap image/tiff image/jp2", this );

  if ( imageUrl.isEmpty() )
    return; // No file was chosen
  else {
    ui_image_properties_dock.fileName->setText( imageUrl.pathOrUrl() );
    m_imageCell->setUrl( imageUrl );
//     ui_image_properties_dock.kimagefilepreview->showPreview( imageUrl );
  }
}

void ImageCellWidget::horizontalCellSpanChanged( int value ) {
  m_imageCell->setHorizontalCellSpan( value );
}

void ImageCellWidget::verticalCellSpanChanged( int value ) {
  m_imageCell->setVerticalCellSpan( value );
}

