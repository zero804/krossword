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

#include <QDebug>

class KgTheme;
class KGameRenderer;
class KgThemeProvider;
class KrosswordTheme;

class KrosswordRenderer
{
public:
    static KrosswordRenderer* self();

    bool hasElement(const QString &elementid) const;

    void renderBackground(QPainter *p, const QRectF& r) const;
    QPixmap background(const QSize &size) const;
    void renderElement(QPainter *p, const QString& elementid, const QRectF& r, const QColor &alpha = Qt::black) const;
    bool setTheme(const QString &themeName);
    KgThemeProvider* getThemeProvider() const;
    QString getCurrentThemeName() const;
    const KgTheme *getCurrentTheme() const;

private:
    // disable copy - it's singleton
    KrosswordRenderer();
    KrosswordRenderer(const KrosswordRenderer&);
    KrosswordRenderer& operator=(const KrosswordRenderer&);
    ~KrosswordRenderer();


    KgThemeProvider *m_provider;
    KGameRenderer   *m_renderer;

    QString m_themeName;
};

#endif // KROSSWORD_RENDERER_H
