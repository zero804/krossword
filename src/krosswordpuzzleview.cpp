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

#include "krosswordpuzzleview.h"
#include "settings.h"
#include "krosswordrenderer.h"
#include <QDebug>


KrossWordPuzzleView::KrossWordPuzzleView(KrossWordPuzzleScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent), m_scene(scene), m_minimumZoomScale(1.0f)
{
    // setOptimizationFlags( QGraphicsView::DontSavePainterState );
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    // setMinimumSize( 100, 100 );
    setCacheMode(CacheBackground);
    settingsChanged();
    // setAutoFillBackground(true);

    setObjectName("krosswordpuzzleview");
}

KrossWordPuzzleView::~KrossWordPuzzleView()
{

}

QSize KrossWordPuzzleView::sizeHint() const
{
    if (krossWord()) {
        QSize sz = krossWord()->cellSize().toSize() * 0.8;
        return QSize(krossWord()->width() * sz.width() + 4, krossWord()->height() * sz.height());
    } else
        return QGraphicsView::sizeHint();
}

void KrossWordPuzzleView::keyPressEvent(QKeyEvent* event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        krossWord()->setAcceptedMouseButtons(Qt::NoButton);
        setDragMode(QGraphicsView::ScrollHandDrag);
    }

    QGraphicsView::keyPressEvent(event);
}

void KrossWordPuzzleView::keyReleaseEvent(QKeyEvent* event)
{
    if (!event->modifiers().testFlag(Qt::ControlModifier)) {
        krossWord()->setAcceptedMouseButtons(Qt::LeftButton | Qt::MidButton | Qt::RightButton);
        setDragMode(QGraphicsView::NoDrag);
    }

    QGraphicsView::keyReleaseEvent(event);
}

void KrossWordPuzzleView::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier))
        emit signalChangeZoom(event->delta() / 10);
    else
        QGraphicsView::wheelEvent(event);
    
}

void KrossWordPuzzleView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    emit resized(event->oldSize(), event->size());
    
    this->fitInView(this->sceneRect().adjusted(150, 150, -150, -150), Qt::KeepAspectRatio);
    updateZoomMinimumScale();
    emit signalChangeZoom(0);
}

void KrossWordPuzzleView::renderToPrinter(QPainter* painter, const QRectF& target, const QRect& source, Qt::AspectRatioMode aspectRatioMode)
{
    bool wasDrawingForPrinting = krossWord()->isDrawingForPrinting();
    krossWord()->setDrawForPrinting();

    render(painter, target, source, aspectRatioMode);

    krossWord()->setDrawForPrinting(wasDrawingForPrinting);
}

void KrossWordPuzzleView::settingsChanged()
{
    Settings::self()->writeConfig();
    emit signalChangeStatusbar(i18n("Settings changed"));
}

qreal KrossWordPuzzleView::getMinimumZoomScale()
{
    return m_minimumZoomScale;
}

void KrossWordPuzzleView::updateZoomMinimumScale()
{
    m_minimumZoomScale = this->matrix().m11();
    if (m_minimumZoomScale > 3.0)
        m_minimumZoomScale /= 2.0;
}
