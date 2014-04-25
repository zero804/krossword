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

#include <KgTheme>
#include <KGameRenderer>
#include <KgThemeProvider>

KrosswordRenderer* KrosswordRenderer::self()
{
    static KrosswordRenderer instance;
    return &instance;
}

KrosswordRenderer::KrosswordRenderer()
    : m_provider(new KgThemeProvider(QByteArray("Theme"))),
      m_renderer(new KGameRenderer(m_provider))
{    
    m_provider->discoverThemes("appdata", QLatin1String("themes"));
    m_provider->currentTheme();
}

KrosswordRenderer::~KrosswordRenderer()
{
    //NOTE Crash if delete them... (?)
    //delete m_provider;
    //delete m_renderer;
}

bool KrosswordRenderer::setTheme(KgTheme *theme)
{
    if (m_themeFileName == theme->graphicsPath())
        return true;

    m_themeFileName = theme->graphicsPath();
    
    m_provider->addTheme(theme);
    return true;
}

QPixmap KrosswordRenderer::background(const QSize &size) const
{
    QPixmap pix;
    pix = m_renderer->spritePixmap("background", size);

    return pix;
}

bool KrosswordRenderer::hasElement(const QString& elementid) const
{
    return m_renderer->spriteExists(elementid);
}

void KrosswordRenderer::renderBackground(QPainter* p, const QRectF& r) const
{
    renderElement(p, "background", r);
}

void KrosswordRenderer::renderElement(QPainter* p, const QString& elementid, const QRectF& r, const QColor &alpha) const
{
    QPixmap pix;
    
    if (alpha != Qt::black) {
        QPixmap pixAlpha = QPixmap(r.size().toSize());
        pixAlpha.fill(alpha);
        pix.setAlphaChannel(pixAlpha);
    }
    
    pix = m_renderer->spritePixmap(elementid, r.size().toSize());
    
    p->setRenderHints(QPainter::HighQualityAntialiasing | QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    p->drawPixmap(static_cast<int>(r.x()), static_cast<int>(r.y()), pix);
}

KgThemeProvider* KrosswordRenderer::getThemeProvider() const
{
    return m_provider;
}
