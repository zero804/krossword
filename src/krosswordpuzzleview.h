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

#ifndef KROSSWORDPUZZLEVIEW_H
#define KROSSWORDPUZZLEVIEW_H

#include <QGraphicsView>

#include "krosswordpuzzlescene.h"

class QPainter;
class KUrl;

/**
 * This is the main view class for KrossWordPuzzle.
 *
 * @short Main view
 * @author Friedrich Pülz <fpuelz@gmx.de>
 * @version 0.15
 */
class KrossWordPuzzleView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit KrossWordPuzzleView(KrossWordPuzzleScene *scene, QWidget *parent = 0);
    virtual ~KrossWordPuzzleView();

    inline Crossword::KrossWord *krossWord() const {
        return ((KrossWordPuzzleScene*)scene())->krossWord();
    }

    void renderToPrinter(QPainter *painter, const QRectF &target = QRectF(),
                         const QRect &source = QRect(), Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio);

    virtual QSize sizeHint() const;

    qreal getMinimumZoomScale() const;

protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

    virtual void resizeEvent(QResizeEvent* event);

private:
    KrossWordPuzzleScene *m_scene;
    qreal m_minimumZoomScale;

signals:
    /**
     * Use this signal to change the content of the statusbar
     */
    void signalChangeStatusbar(const QString& text);

    /**
     * Use this signal to change the content of the caption
     */
    void signalChangeCaption(const QString& text);

    void signalChangeZoom(int zoomChange);

    void resized(const QSize &oldSize, const QSize &newSize);

private slots:
    void settingsChanged();
};

#endif // KROSSWORDPUZZLEVIEW_H
