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

#include <klocale.h>
#include <QtGui/QLabel>
// #include <QGLWidget>
#include "krosswordrenderer.h"
#include <QDebug>



/*
#include <QPropertyAnimation>
ScrollbarAnimator::ScrollbarAnimator( QAbstractSlider* slider, QWidget* parent )
        : QObject( parent ), m_slider( slider ), m_animation( 0 ) {
  m_oldValue = m_slider->value();
  m_setValueSlotDeactivated = false;
  connect( m_slider, SIGNAL(actionTriggered(int)), this, SLOT(sliderActionTriggered(int)) );
  connect( m_slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)) );
  connect( m_slider, SIGNAL(destroyed(QObject*)), this, SLOT(sliderDestroyed(QObject*)) );
}

void ScrollbarAnimator::sliderDestroyed( QObject* ) {
  deleteLater();
}

void ScrollbarAnimator::sliderValueChanged( int value ) {
//   if ( m_setValueSlotDeactivated )
//     return;
  if ( m_animation ) {
    int change = value - m_animation->currentValue().toInt();
//     kDebug() << "Restart?" << change;
    if ( change != 0 )
      startAnimation( change );
  } else {
    int change = value - m_oldValue;
    if ( qAbs(change) > 10 )
      startAnimation( change );
  }
//   m_oldValue = value;
}

void ScrollbarAnimator::sliderActionTriggered( int action ) {
//   switch( action ) {
//     case QAbstractSlider::SliderPageStepAdd:
//       startAnimation( m_slider->pageStep() );
//       break;
//     case QAbstractSlider::SliderPageStepSub:
//       startAnimation( -m_slider->pageStep() );
//       break;
//     case QAbstractSlider::SliderSingleStepAdd:
//       startAnimation( m_slider->singleStep() );
//       break;
//     case QAbstractSlider::SliderSingleStepSub:
//       startAnimation( -m_slider->singleStep() );
//       break;
//     case QAbstractSlider::SliderMove:
//       startAnimation( m_slider->sliderPosition() - m_slider->value() );
//       break;
//     case QAbstractSlider::SliderToMinimum:
//       startAnimation( m_slider->minimum() - m_slider->sliderPosition() );
//       break;
//     case QAbstractSlider::SliderToMaximum:
//       startAnimation( m_slider->maximum() - m_slider->sliderPosition() );
//       break;
//
//     case QAbstractSlider::SliderNoAction:
//     default:
//       kDebug() << "SliderNoAction" << (m_slider->sliderPosition() - m_slider->value());
//       break;
//   }
}

void ScrollbarAnimator::startAnimation( int scrollOffset ) {
  if ( m_animation ) {
    m_animation->setStartValue( m_animation->currentValue().toInt() );
    m_animation->setEndValue( m_animation->currentValue().toInt() + scrollOffset );
    m_animation->setCurrentTime( 0 );
  } else {
    m_animation = new QPropertyAnimation( this, "value" );
    connect( m_animation, SIGNAL(finished()), this, SLOT(animationFinished()) );
    m_animation->setDuration( 250 );
    m_animation->setStartValue( m_oldValue );
    m_animation->setEndValue( m_oldValue + scrollOffset );
    m_animation->setEasingCurve( QEasingCurve(QEasingCurve::OutCurve) );
    m_animation->start();
  }
}

void ScrollbarAnimator::setValue( int value ) {
  if ( value != m_slider->value() )
    m_slider->setValue( value );
}

void ScrollbarAnimator::animationFinished() {
  m_oldValue = m_slider->value();
  delete m_animation;
  m_animation = NULL;
}*/


KrossWordPuzzleView::KrossWordPuzzleView(KrossWordPuzzleScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent), m_scene(scene), m_minimum_zoom(0)
{
//     new ScrollbarAnimator( verticalScrollBar() );
//     new ScrollbarAnimator( horizontalScrollBar() );

//     setOptimizationFlags( QGraphicsView::DontSavePainterState );
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

//     setMinimumSize( 100, 100 );
    setCacheMode(CacheBackground);
    settingsChanged();
//     setViewport( new QGLWidget );
//     setAutoFillBackground(true);

    setObjectName("krosswordpuzzleview");
}

KrossWordPuzzleView::~KrossWordPuzzleView()
{

}

QSize KrossWordPuzzleView::sizeHint() const
{
    if (krossWord()) {
        QSize sz = krossWord()->cellSize().toSize() * 0.8;
//  kDebug() << QSize( krossWord()->width() * sz.width(), krossWord()->height() * sz.height() );
        return QSize(krossWord()->width() * sz.width() + 4, krossWord()->height() * sz.height());
//  return mapFromScene( krossWord()->boundingRect() ).boundingRect().size();
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
        emit signalChangeZoom(event->delta() / 10, m_minimum_zoom);
    else
        QGraphicsView::wheelEvent(event);
    
}

void KrossWordPuzzleView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    emit resized(event->oldSize(), event->size());
    
    this->fitInView(this->sceneRect().adjusted(150, 150, -150, -150), Qt::KeepAspectRatio);
    m_minimum_zoom = this->matrix().m11() * 100;
    emit signalChangeZoom(0, m_minimum_zoom);
}
/*
void KrossWordPuzzleView::setTheme( const QString &theme )
{
    if ( !KrosswordRenderer::self()->setTheme(theme) )
    return;

    scene()->update();
    Settings::setTheme( theme );
    settingsChanged();
}*/

void KrossWordPuzzleView::renderToPrinter(QPainter* painter, const QRectF& target, const QRect& source, Qt::AspectRatioMode aspectRatioMode)
{
    bool wasDrawingForPrinting = krossWord()->isDrawingForPrinting();
    krossWord()->setDrawForPrinting();

    render(painter, target, source, aspectRatioMode);

    krossWord()->setDrawForPrinting(wasDrawingForPrinting);
}

void KrossWordPuzzleView::settingsChanged()
{
//     QPalette pal;
//     pal.setColor( QPalette::Window, Settings::col_background());
//     pal.setColor( QPalette::WindowText, Settings::col_foreground());
//     ui_krosswordpuzzleview_base.kcfg_sillyLabel->setPalette( pal );

//     ui_krosswordpuzzleview_base.kcfg_sillyLabel->setText( i18n("This project is %1 days old",Settings::val_time()) );
    Settings::self()->writeConfig();
    emit signalChangeStatusbar(i18n("Settings changed"));
}

#include "krosswordpuzzleview.moc"
