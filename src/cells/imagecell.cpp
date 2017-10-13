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
#include "krosswordtheme.h"

#include <QGraphicsScene>
#include <QTextOption>
#include <QStyleOption>
#include <QPainter>
//#include <KIO/NetAccess>

namespace Crossword
{

ImageCell::ImageCell(KrossWord* krossWord, const Coord& coordTopLeft,
                     int horizontalCellSpan, int verticalCellSpan, const QUrl &url)
    : SpannedCell(krossWord, ImageCellType, coordTopLeft,
                  horizontalCellSpan, verticalCellSpan)
{
    setUrl(url);

    setFlag(QGraphicsItem::ItemIsFocusable, krossWord->isEditable());
    setFlag(QGraphicsItem::ItemIsSelectable, krossWord->isEditable());
}

void ImageCell::setUrl(const QUrl &url)
{
    //CHECK: external url really needed?
    /*
    if (!url.isLocalFile()) {
        QString fileName;
        if (KIO::NetAccess::download(url, fileName, 0)) {
            m_image = QImage(fileName);
            KIO::NetAccess::removeTempFile(fileName);
        }
    } else {
        m_image = QImage(url.url(QUrl::PreferLocalFile));
    }
    */

    m_image = QImage(url.url(QUrl::PreferLocalFile));
    m_url = url;

    clearCache();
    update();
}

void ImageCell::drawBackground(QPainter *p,
                               const QStyleOptionGraphicsItem *options)
{
    if (m_image.isNull()) {
        QTextOption textOption(Qt::AlignCenter);
        textOption.setWrapMode(QTextOption::WordWrap);
        p->drawText(options->rect, i18n("Edit image properties to set an image"),
                    textOption);
    } else {
        p->drawImage(options->rect, m_image);
    }

    if (isHighlighted()) {
        p->fillRect(options->rect, krossWord()->theme()->selectionColor());
        p->setPen(Qt::red);
        p->drawRect(options->rect);
    } else {
        p->setPen(Qt::black);
        p->drawRect(options->rect);
    }
}

void ImageCell::drawBackgroundForPrinting(QPainter *p,
        const QStyleOptionGraphicsItem *options)
{
    drawBackground(p, options);
}

void ImageCell::focusInEvent(QFocusEvent* event)
{
    krossWord()->setHighlightedClue(NULL);
    if (krossWord()->isEditable())
        setHighlight();
    KrossWordCell::focusInEvent(event);
}

void ImageCell::focusOutEvent(QFocusEvent* event)
{
    // Only remove highlight if the scene still has focus
    if (scene() && scene()->hasFocus())
        setHighlight(false);
    KrossWordCell::focusOutEvent(event);
}

}; // namespace Crossword
