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

#ifndef KROSSWORD_RENDERER_H
#define KROSSWORD_RENDERER_H

#include <QPixmap>
#include <KGameRenderer>

class KgThemeProvider;
class KrosswordTheme;

class KrosswordRenderer
{
public:
    static KrosswordRenderer* self();

    bool hasElement(const QString &id) const;

    QPixmap background(const QSize &size) const;
    void renderElement(QPainter *painter, const QString& id, const QRectF& rect) const;

    bool setTheme(const QString &themeName);
    KgThemeProvider* getThemeProvider() const;
    const KrosswordTheme *getCurrentTheme() const;

private:
    // disable copy - it's singleton
    KrosswordRenderer();
    KrosswordRenderer(const KrosswordRenderer&);
    KrosswordRenderer& operator=(const KrosswordRenderer&);

    KgThemeProvider *m_provider;
    KGameRenderer m_renderer;
};

#endif // KROSSWORD_RENDERER_H
