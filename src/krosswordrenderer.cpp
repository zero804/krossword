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

#include <QPainter>
#include "settings.h"

#include <KgTheme>
#include <KGameRenderer>
#include <KgThemeProvider>

#include "krosswordtheme.h"

KrosswordRenderer* KrosswordRenderer::self()
{
    static KrosswordRenderer instance;
    return &instance;
}

KrosswordRenderer::KrosswordRenderer()
    : m_provider(new KgThemeProvider(QByteArray("Theme"))),
      m_renderer(m_provider)
{
    m_provider->discoverThemes("appdata", QLatin1String("themes"), QLatin1String("default"), &KrosswordTheme::staticMetaObject);
}

bool KrosswordRenderer::setTheme(const QString& themeName)
{
    if (getCurrentTheme()->name() == themeName) {
        return true;
    }

    QList<const KgTheme*> themeList = m_provider->themes();
    auto found = std::find_if(themeList.begin(), themeList.end(), [&themeName](const KgTheme* theme) {
        return theme->name() == themeName;
    });

    if (found != themeList.end()) {
        m_provider->setCurrentTheme(*found);
        return true;
    }

    return false;
}

QPixmap KrosswordRenderer::background(const QSize &size) const
{
    return m_renderer.spritePixmap("background", size);
}

bool KrosswordRenderer::hasElement(const QString& id) const
{
    return m_renderer.spriteExists(id);
}

void KrosswordRenderer::renderBackground(QPainter* painter, const QRectF& rect) const
{
    renderElement(painter, "background", rect);
}

void KrosswordRenderer::renderElement(QPainter* painter, const QString& id, const QRectF& rect) const
{
    QPixmap pix = m_renderer.spritePixmap(id, rect.size().toSize());
    painter->setRenderHints(QPainter::HighQualityAntialiasing | QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    painter->drawPixmap(static_cast<int>(rect.x()), static_cast<int>(rect.y()), pix);
}

KgThemeProvider* KrosswordRenderer::getThemeProvider() const
{
    return m_provider;
}

const KrosswordTheme *KrosswordRenderer::getCurrentTheme() const
{
    return (const KrosswordTheme*)m_provider->currentTheme();
}
