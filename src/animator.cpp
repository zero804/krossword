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

#include "animator.h"

#include "krossword.h"
#include "cells/krosswordcell.h"
#include <QGraphicsObject>
#include <QPropertyAnimation>
#include <QGraphicsScene>
#include <QPainter>
#include <QDateTime>
#include <QParallelAnimationGroup>

namespace Crossword
{

TransitionAnimation::TransitionAnimation(KrossWordCell *cell)
    : QVariantAnimation(), m_cell(cell)
{
    m_pixmapObject = new GraphicsPixmapObject(cell->pixmap(), cell->parentItem());
    m_pixmapObject->setTransformationMode(Qt::SmoothTransformation);
    m_pixmapObject->setObjectName("transitionItem");
    m_pixmapObject->setZValue(1000);
//   m_pixmapObject->setTransform( cell->transform() );
    m_pixmapObject->setPos(cell->pos() + cell->boundingRect().topLeft()
                           + QPointF(0.5, 0.5));
    qreal scale = cell->boundingRect().width() / m_pixmapObject->pixmap().width();
    m_pixmapObject->scale(scale, scale);

    m_pixmapObject->setTransformOriginPoint(cell->boundingRect().center());

    // Fade old item appearance out (transition item)
    setStartValue(1.0);
    setEndValue(0.0);
    setEasingCurve(QEasingCurve(QEasingCurve::OutExpo));
    m_cell->setOpacity(0);

    connect(cell, SIGNAL(destroyed(QObject*)),
            this, SLOT(cellDestroyed(QObject*)));
    connect(cell, SIGNAL(appearanceAboutToChange()),
            this, SLOT(appearanceAboutToChange()));
}

TransitionAnimation::~TransitionAnimation()
{
    if (m_cell) {
        m_cell->setOpacity(1);
        emit finished(m_cell);
    }

    if (m_pixmapObject) {
        if (m_pixmapObject->scene())
            m_pixmapObject->scene()->removeItem(m_pixmapObject);
        delete m_pixmapObject;
    }
}

void TransitionAnimation::updateCurrentValue(const QVariant& value)
{
    if (!m_cell)
        return; // Without this it crashes when cells get destroyed

    if (!m_pixmapObject) {
        m_cell->setOpacity(1);
        return;
    }

    qreal opacity = value.toReal();
    m_pixmapObject->setOpacity(opacity);
    m_cell->setOpacity(1 - opacity);
}

void TransitionAnimation::cellDestroyed(QObject *obj)
{
    qDebug() << "Cell destroyed! Delete transition item for ?" << obj;
    m_cell = NULL;
    stop();
}

QPixmap TransitionAnimation::composedCellPixmap() const
{
//   kDebug() << "Draw composed cell pixmap" << m_cell->coord();

    int opacity = m_pixmapObject->opacity() * 255;
    if (opacity <= 1)
        return m_cell->pixmap();
    else if (opacity >= 254)
        return m_pixmapObject->pixmap();

    QPixmap pix(m_pixmapObject->pixmap().size());
    pix.fill(Qt::transparent);

    QPixmap pix1(m_pixmapObject->pixmap().size());
    QPixmap pix2(m_pixmapObject->pixmap().size());
    pix1.fill(Qt::transparent);
    pix2.fill(Qt::transparent);

    // Draw m_pixmapObject->pixmap() with m_pixmapObject->opacity() to pix1
    QPainter p1(&pix1);
    p1.setCompositionMode(QPainter::CompositionMode_Source);
    p1.drawPixmap(0, 0, m_pixmapObject->pixmap());
    p1.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p1.fillRect(QRect(QPoint(0, 0), pix1.size()), QColor(0, 0, 0, opacity));
//   p1.fillRect( pix1.rect(), QColor(0, 0, 0, m_pixmapObject->opacity()) );
    p1.end();

    // Draw m_cell->pixmap() with m_cell->opacity() to pix2
    QPainter p2(&pix2);
    p2.setCompositionMode(QPainter::CompositionMode_Source);
    p2.drawPixmap(0, 0, m_cell->pixmap());
    p2.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p2.fillRect(QRect(QPoint(0, 0), pix2.size()), QColor(0, 0, 0, 255 - opacity));
    p2.end();

    // Draw pix1 and pix2 into pix
    QPainter p3(&pix);
    p3.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p3.drawPixmap(0, 0, pix1);
    p3.drawPixmap(0, 0, pix2);
    p3.end();

    return pix;
}

void TransitionAnimation::appearanceAboutToChange()
{
    if (currentTime() > 25) {
        m_pixmapObject->setPixmap(composedCellPixmap());

        // Restart animation
//     if ( state() == QAbstractAnimation::Stopped )
//       emit finished( m_cell );
        setCurrentTime(0);
    }
}

void TransitionAnimation::updateState(QAbstractAnimation::State newState,
                                      QAbstractAnimation::State oldState)
{
    if (newState == QAbstractAnimation::Stopped && m_pixmapObject) {
//     kDebug() << "FINISHED transition of" << m_cell;
//     kDebug() << "Delete transition item for" << m_cell->coord();
        if (m_cell) {
//       disconnect( m_cell, SIGNAL(destroyed(QObject*)),
//        this, SLOT(cellDestroyed(QObject*)) );
            disconnect(m_cell, SIGNAL(appearanceAboutToChange()),
                       this, SLOT(appearanceAboutToChange()));
            m_cell->setOpacity(1);
        }

        if (m_pixmapObject->scene()) {
            m_pixmapObject->scene()->removeItem(m_pixmapObject);
        }
        delete m_pixmapObject;
        m_pixmapObject = 0;

        QVariantAnimation::updateState(newState, oldState);
        emit finished(m_cell);
    } else
        QVariantAnimation::updateState(newState, oldState);
}

qreal Animator::durationToDurationFactor(Duration duration) const
{
    switch (duration) {
    case Slowest:   return 5;
    case VerySlow:  return 3;
    case Slow:      return 2;
    case Fast:      return 0.66;
    case VeryFast:  return 0.33;
    case Instant:   return 0;

    default:
    case DefaultDuration: return 1;
    }
}

void Animator::beginEnqueueAnimations()
{
    if (!isQueuingAnimations())
        m_queuedAnimationGroup = new QParallelAnimationGroup;

    ++m_queueBeginCount;
}

void Animator::endEnqueueAnimations()
{
    if (m_queueBeginCount == 1) {
        m_queuedAnimationGroup->start(QAbstractAnimation::DeleteWhenStopped);
        m_queuedAnimationGroup = NULL;
        m_queueBeginCount = 0;
    } else if (m_queueBeginCount > 1)
        --m_queueBeginCount;
}

TransitionAnimation* Animator::animateTransition(KrossWordCell* cell,
        Duration duration,
        AnimateFlags flags)
{
    if (!isEnabled() || duration == Instant)
        return NULL;

    if (isInTransition(cell)) {
        return m_transitionItems[ cell ];
    } else {
        TransitionAnimation *anim = new TransitionAnimation(cell);
        anim->setDuration(defaultDuration() * durationToDurationFactor(duration));
        m_transitionItems.insert(cell, anim);
        connect(anim, SIGNAL(finished(KrossWordCell*)),
                this, SLOT(transitionAnimationFinished(KrossWordCell*)));

        startOrEnqueue(anim, flags);
        return anim;
    }
}

void Animator::transitionAnimationFinished(KrossWordCell* cell)
{
    QAbstractAnimation *anim = m_transitionItems.take(cell);

    if (m_queuedAnimationGroup) {
        int index = m_queuedAnimationGroup->indexOfAnimation(anim);
        if (index >= 0)
            m_queuedAnimationGroup->removeAnimation(anim);
    }
}

QAbstractAnimation* Animator::animate(BasicAnimation basicAnimation,
                                      QGraphicsObject *obj,
                                      Duration duration, AnimateFlags flags)
{
    if (!isEnabled() || duration == Instant) {
        switch (basicAnimation) {
        case AnimateFadeIn:
            obj->show();
            obj->setOpacity(1);
            break;
        case AnimateFadeOut:
            obj->hide();
            obj->setOpacity(0);
            break;
        case AnimateBounce:
            break;
        }
        return NULL;
    } else {
        QPropertyAnimation *anim;
        switch (basicAnimation) {
        case AnimateFadeIn:
            anim = new QPropertyAnimation(obj, "opacity");
            anim->setStartValue(obj->opacity());
            anim->setEndValue(1);
            obj->show();
            break;
        case AnimateFadeOut:
            anim = new QPropertyAnimation(obj, "opacity");
            anim->setStartValue(obj->opacity());
            anim->setEndValue(0);
            break;
        case AnimateBounce:
            anim = new QPropertyAnimation(obj, "scale");
            anim->setStartValue(1.0);
            anim->setKeyValueAt(0.5, 1.2);
            anim->setEndValue(1.0);
            anim->setEasingCurve(QEasingCurve(QEasingCurve::InOutBack));
            break;

        default:
            kWarning() << "Animation type unknown:" << basicAnimation;
            return NULL;
        }

        startOrEnqueue(anim, duration, flags);
        return anim;
    }
}

QAbstractAnimation* Animator::animate(OneParameterAnimation animation,
                                      QGraphicsObject* obj, const QVariant& argument,
                                      Duration duration, AnimateFlags flags)
{
    if (!isEnabled() || duration == Instant) {
        switch (animation) {
        case AnimatePositionChange:
            Q_ASSERT(argument.canConvert(QVariant::PointF));
            obj->setPos(argument.toPointF());
            break;
        }
        return NULL;
    } else {
        QPropertyAnimation *anim;
        switch (animation) {
        case AnimatePositionChange:
            Q_ASSERT(argument.canConvert(QVariant::PointF));
            anim = new QPropertyAnimation(obj, "pos");
            anim->setStartValue(obj->pos());
            anim->setEndValue(argument.toPointF());
            anim->setEasingCurve(QEasingCurve(QEasingCurve::InOutBack));
            break;

        default:
            kWarning() << "Animation type unknown:" << animation;
            return NULL;
        }

        startOrEnqueue(anim, duration, flags);
        return anim;
    }
}

bool Animator::startOrEnqueue(QAbstractAnimation* anim, AnimateFlags flags)
{
    if (isQueuingAnimations()) {
        m_queuedAnimationGroup->addAnimation(anim);
        return false;
    } else if (flags.testFlag(AnimateStart)) {
        anim->start(flags.testFlag(AnimateDeleteWhenStopped)
                    ? QAbstractAnimation::DeleteWhenStopped
                    : QAbstractAnimation::KeepWhenStopped);
        return true;
    } else
        return false;
}

} // namespace
