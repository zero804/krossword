#include "zoomwidget.h"
#include <QDebug>
#include <QTimer>
#include <QToolTip>

#include "krosswordpuzzleview.h"

ZoomWidget::ZoomWidget(unsigned int maxZoomFactor, unsigned int buttonStep , QWidget *parent)
    : QWidget(parent),
      m_layout(new QBoxLayout(QBoxLayout::LeftToRight)),
      m_btnZoomOut(new QToolButton),
      m_btnZoomIn(new QToolButton),
      m_zoomSlider(new QSlider(Qt::Horizontal))
{
    if(maxZoomFactor <= 1) {
        qDebug() << "\'maxZoomFactor\' in ZoomWidget ctor must be at least 2";
        maxZoomFactor = 2;
    }

    this->setLayout(m_layout);

    m_btnZoomOut->setIcon(QIcon::fromTheme(QStringLiteral("zoom-out")));
    m_btnZoomIn->setIcon(QIcon::fromTheme(QStringLiteral("zoom-in")));
    m_btnZoomOut->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_btnZoomIn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_btnZoomOut->setAutoRaise(true);
    m_btnZoomIn->setAutoRaise(true);
    m_zoomSlider->setPageStep(buttonStep);
    m_zoomSlider->setRange(100, 100*maxZoomFactor);
    m_zoomSlider->setValue(100);

    m_layout->setSpacing(0);
    m_layout->setMargin(0);
    m_layout->addWidget(m_btnZoomOut);
    m_layout->addWidget(m_zoomSlider);
    m_layout->addWidget(m_btnZoomIn);


    connect(m_btnZoomOut, SIGNAL(pressed()), this, SLOT(zoomOutBtnPressedSlot()));
    connect(m_btnZoomIn,  SIGNAL(pressed()), this, SLOT(zoomInBtnPressedSlot()));
    connect(m_zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(zoomSliderChangedSlot(int)));
}

unsigned int ZoomWidget::currentZoom() const
{
    return m_zoomSlider->value();
}

unsigned int ZoomWidget::minimumZoom() const
{
    return m_zoomSlider->minimum();
}

unsigned int ZoomWidget::maximumZoom() const
{
    return m_zoomSlider->maximum();
}

void ZoomWidget::setZoom(unsigned int zoom)
{
    unsigned int min = minimumZoom();
    unsigned int max = maximumZoom();
    if(zoom >= min) {
        if(zoom <= max) {
            m_zoomSlider->setValue(zoom);
            emit zoomChanged(zoom);
        } else {
            m_zoomSlider->setValue(max);
            emit zoomChanged(max);
        }
    } else {
        m_zoomSlider->setValue(min);
        emit zoomChanged(min);
    }

    if(zoom == min) {
        m_btnZoomOut->setEnabled(false);
    } else if(zoom == max) {
        m_btnZoomIn->setEnabled(false);
    } else {
        m_btnZoomOut->setEnabled(true);
        m_btnZoomIn->setEnabled(true);
    }
}

void ZoomWidget::setSliderToolTip(const QString& tooltip)
{
    m_zoomSlider->setToolTip(tooltip);
}

QString ZoomWidget::sliderToolTip() const
{
    return m_zoomSlider->toolTip();
}

void ZoomWidget::zoomOutBtnPressedSlot()
{
    setZoom(currentZoom() - m_zoomSlider->pageStep());
}

void ZoomWidget::zoomInBtnPressedSlot()
{
    setZoom(currentZoom() + m_zoomSlider->pageStep());
}

void ZoomWidget::zoomSliderChangedSlot(int change)
{
    setZoom(change);
}

//=============================================================

ViewZoomController::ViewZoomController(KrossWordPuzzleView* view, ZoomWidget* widget, QObject *parent)
    : QObject(parent),
      m_controlledWidget(widget),
      m_view(view)
{
    Q_ASSERT(view != nullptr);
    Q_ASSERT(widget != nullptr);
    connect(m_controlledWidget, SIGNAL(zoomChanged(int)), this, SLOT(changeViewZoomSlot(int)));
    connect(m_view, SIGNAL(signalChangeZoom(int)), this, SLOT(changeZoomSliderSlot(int)));
}

void ViewZoomController::changeViewZoomSlot(int zoomValue)
{
    qreal maximumZoomFactor = m_controlledWidget->maximumZoom() / qreal(m_controlledWidget->minimumZoom());
    qreal minZoomScale = m_view->getMinimumZoomScale();
    qreal maxZoomScale = maximumZoomFactor * minZoomScale;

    qreal zoom = zoomValue / qreal(m_controlledWidget->maximumZoom());
    zoom *= maxZoomScale;

    QTransform t = m_view->transform();
    t.setMatrix(zoom,    t.m12(), t.m13(),
                t.m21(), zoom,    t.m23(),
                t.m31(), t.m32(), t.m33());

    m_view->setTransform(t);

    //XXX: This was responsible of the flickering, I don't know why it was here and what it does...
    //QTimer::singleShot(500, m_view->krossWord(), SLOT(clearCache()));

    m_controlledWidget->setSliderToolTip(i18n("Zoom: %1%", zoomValue));
    QToolTip::showText(QCursor::pos(), m_controlledWidget->sliderToolTip(), m_controlledWidget);
}

void ViewZoomController::changeZoomSliderSlot(int zoomValue)
{
    unsigned int new_zoom = m_controlledWidget->currentZoom() + zoomValue;
    m_controlledWidget->setZoom(new_zoom);
}
