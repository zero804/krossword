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

#include "imagecell.h"
#include "krossword.h"

#include <QGraphicsScene>
#include <KIO/NetAccess>


ImageCell::ImageCell( KrossWord* krossWord, const Coord& coordTopLeft,
                      int horizontalCellSpan, int verticalCellSpan, const KUrl &url )
        : SpannedCell( krossWord, KrossWordCell::ImageCellType, coordTopLeft,
                       horizontalCellSpan, verticalCellSpan )
{
    if ( !url.isLocalFile() ) {
        QString fileName;
        if ( KIO::NetAccess::download( url, fileName, 0 ) ) {
            m_image = QImage( fileName );
            KIO::NetAccess::removeTempFile( fileName );
        }
    } else {
        m_image = QImage( url.pathOrUrl() );
    }

    m_url = url;
}

void ImageCell::drawBackgroundForPrinting( QPainter *p,
        const QStyleOptionGraphicsItem *options )
{
    p->drawImage( options->rect, m_image );

    p->setPen( Qt::black );
    p->drawRect( options->rect );
}


