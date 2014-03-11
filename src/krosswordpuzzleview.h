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

// #include <QtGui/QWidget>
#include <QGraphicsView>
#include <QScrollBar>

// #include "ui_krosswordpuzzleview_base.h"
#include "krosswordpuzzlescene.h"

// class QGraphicsView;
class QPainter;
class KUrl;

// TEST
// class ScrollbarAnimator : public QObject {
//   Q_OBJECT
//   Q_PROPERTY( int value READ value WRITE setValue )
// 
//   public:
//     ScrollbarAnimator( QAbstractSlider *slider, QWidget* parent = 0 );
// 
//     int value() const { return m_slider->value(); };
//     void setValue( int value );
// 
//   protected slots:
//     void sliderActionTriggered( int action );
//     void sliderValueChanged( int value );
//     void sliderDestroyed( QObject* );
//     void animationFinished();
// 
//   private:
//     void startAnimation( int scrollOffset );
// 
//     QAbstractSlider *m_slider;
//     QPropertyAnimation *m_animation;
//     int m_oldValue;
//     bool m_setValueSlotDeactivated;
// };


/**
 * This is the main view class for KrossWordPuzzle.
 *
 * @short Main view
 * @author Friedrich Pülz <fpuelz@gmx.de>
 * @version 0.15
 */
class KrossWordPuzzleView : public QGraphicsView //, public Ui::krosswordpuzzleview_base
{
    Q_OBJECT
public:
    /**
     * Default constructor
     */
    explicit KrossWordPuzzleView( KrossWordPuzzleScene *scene, QWidget *parent = 0 );

    /**
     * Destructor
     */
    virtual ~KrossWordPuzzleView();

    inline Crossword::KrossWord *krossWord() const {
	return ((KrossWordPuzzleScene*)scene())->krossWord(); };
//     void setTheme( const QString &theme = "default" );

    void renderToPrinter( QPainter *painter, const QRectF &target = QRectF(),
		const QRect &source = QRect(),
                Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio );

    virtual QSize sizeHint() const;

protected:
    virtual void keyPressEvent( QKeyEvent* event );
    virtual void keyReleaseEvent( QKeyEvent* event );
    virtual void wheelEvent( QWheelEvent* event );

    virtual void resizeEvent( QResizeEvent* event );

private:
//     Ui::krosswordpuzzleview_base ui_krosswordpuzzleview_base;
    KrossWordPuzzleScene *m_scene;

signals:
    /**
     * Use this signal to change the content of the statusbar
     */
    void signalChangeStatusbar( const QString& text );

    /**
     * Use this signal to change the content of the caption
     */
    void signalChangeCaption( const QString& text );

    void signalChangeZoom( int zoomChange );

    void resized( const QSize &oldSize, const QSize &newSize );

private slots:
    void settingsChanged();
};

#endif // KrossWordPuzzleVIEW_H
