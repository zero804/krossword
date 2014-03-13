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

#include "krosswordrenderer.h"

#include <kstandarddirs.h>
#include <ksvgrenderer.h>
#include <kimagecache.h>

#include <QPainter>
#include "settings.h"

#include <memory>


KrosswordRenderer* KrosswordRenderer::self()
{
    static KrosswordRenderer instance;
    return &instance;
}

KrosswordRenderer::KrosswordRenderer()
{
    int *foo = nullptr;
    
    m_cache = new KImageCache("krosswordpuzzle-cache", 1024);
    m_cache->setPixmapCacheLimit(2 * 1024);

    m_renderer = new KSvgRenderer();
//     m_renderer->load( KStandardDirs::locate( "appdata", "themes/scribble_theme.svgz" ) );
//     setTheme( Settings::theme() );
}

KrosswordRenderer::~KrosswordRenderer()
{
    m_cache->clear();
    delete m_renderer;
    delete m_cache;
}

bool KrosswordRenderer::setTheme(const QString& fileName)
{
    if (m_themeFileName == fileName)
        return true;

    m_themeFileName = fileName;
    m_cache->clear();

//     QString themeFile = KStandardDirs::locate( "appdata",
//             "themes/" + m_themeName + ".desktop" );
//     if ( themeFile.isNull() )
//       return false; // Theme not found

    return m_renderer->load(fileName);
}

QPixmap KrosswordRenderer::background(const QSize &size) const
{
    QPixmap pix;
//     QString cacheStr = "background" + QString("_%1x%2")
//          .arg(size.width()).arg(size.height());
//     if ( !m_cache->find(cacheStr, pix) ) {

    pix = QPixmap(size);
    pix.fill(Qt::transparent);
    QPainter paint(&pix);
    m_renderer->render(&paint, "background");
    paint.end();
//  m_cache->insert( cacheStr, pix );
//     }

    return pix;
}

bool KrosswordRenderer::hasElement(const QString& elementid) const
{
    return m_renderer->elementExists(elementid);
}

void KrosswordRenderer::renderBackground(QPainter* p, const QRectF& r) const
{
    renderElement(p, "background", r);
}

// void KrosswordRenderer::renderElement( QPainter* p, const QString& elementid,
//       const QRectF& r ) const {
//     QPixmap pix;
//
//     QString cacheStr = elementid + QString( "_%1x%2" )
//   .arg( r.width() ).arg( r.height() );
//     if ( !m_cache->find(cacheStr, pix) ) {
//  pix = QPixmap( r.size().toSize() );
//  pix.fill( Qt::transparent );
//  QPainter painter( &pix );
//  painter.setRenderHints( QPainter::HighQualityAntialiasing | QPainter::Antialiasing
//      | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing );
//  m_renderer->render( &painter, elementid );
//  painter.end();
//  m_cache->insert( cacheStr, pix );
//     }
//
// //     p->setRenderHints( QPainter::HighQualityAntialiasing | QPainter::Antialiasing
// //      | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing );
//     p->drawPixmap( static_cast<int>(r.x()), static_cast<int>(r.y()), pix );
// }

void KrosswordRenderer::renderElement(QPainter* p, const QString& elementid, const QRectF& r, const QColor &alpha) const
{
    QPixmap pix;

    QString cacheStr = elementid + QString("_%1x%2").arg(r.width()).arg(r.height());
    if (!m_cache->findPixmap(cacheStr, &pix)) {
        pix = QPixmap(r.size().toSize());
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setRenderHints(QPainter::HighQualityAntialiasing | QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
        m_renderer->render(&painter, elementid);
        painter.end();
        m_cache->insertPixmap(cacheStr, pix);
    }

    if (alpha != Qt::black) {
        QPixmap pixAlpha = QPixmap(r.size().toSize());
        pixAlpha.fill(alpha);
        pix.setAlphaChannel(pixAlpha);
    }

    p->setRenderHints(QPainter::HighQualityAntialiasing | QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    p->drawPixmap(static_cast<int>(r.x()), static_cast<int>(r.y()), pix);
}


