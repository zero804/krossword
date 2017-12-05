#ifndef ZOOMWIDGET_H
#define ZOOMWIDGET_H

#include <QWidget>
#include <QBoxLayout>
#include <QToolButton>
#include <QSlider>

class KrossWordPuzzleView;

class ZoomWidget : public QWidget
{
    Q_OBJECT

public:
    ZoomWidget(unsigned int maxZoomFactor, unsigned int buttonStep, QWidget *parent = nullptr);

    unsigned int currentZoom() const;
    void setZoom(unsigned int zoom);
    void setSliderToolTip(const QString& tooltip);

    unsigned int minimumZoom() const;
    unsigned int maximumZoom() const;
    QString sliderToolTip() const;

signals:
    void zoomChanged(int zoomValue);

private slots:
    void zoomOutBtnPressedSlot();
    void zoomInBtnPressedSlot();
    void zoomSliderChangedSlot(int change);

private:
    int m_currentZoomValue;
    QBoxLayout  *m_layout;
    QToolButton *m_btnZoomOut;
    QToolButton *m_btnZoomIn;
    QSlider     *m_zoomSlider;
};

class ViewZoomController : QObject
{
    Q_OBJECT

public:
    ViewZoomController(KrossWordPuzzleView* view, ZoomWidget* widget, QObject *parent=nullptr);

public slots:
    void changeViewZoomSlot(int zoomValue);
    void changeZoomSliderSlot(int zoomValue);

private:
    ZoomWidget* m_controlledWidget; // Not owned
    KrossWordPuzzleView* m_view;    // Now owned
};

#endif // ZOOMWIDGET_H
