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

#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <QList>
#include <QGraphicsPixmapItem>
#include <QVariantAnimation>
#include <QParallelAnimationGroup>

#include <QDebug>

namespace Crossword
{

class KrossWord;
class KrossWordCell;

/** Pixmap item that is used by @ref Animator to create transition animations. */
class GraphicsPixmapObject : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit GraphicsPixmapObject(const QPixmap &pixmap, QGraphicsItem *parent = 0)
        : QObject(0), QGraphicsPixmapItem(pixmap, parent) {};

//   public slots:
//     void setRotationOfParent() {
//  setRotation( parentItem()->rotation() ); };
//     void setScaleOfParent() {
//  setScale( parentItem()->scale() ); };
//     void setPosOfParent() {
//  setScale( parentItem()->scale() ); };
};

/** This animation fades from an old appearance of a crossword cell to a new one.
  * A pixmap item is created that fades out, showing the old appearance of the
  * cell. In parallel the cell itself fades in, showing the new appearance of
  * the cell. The pixmap items opacity and the opacity of the cell adds to 1
  * in every frame of the animation. */
class TransitionAnimation : public QVariantAnimation
{
    Q_OBJECT

public:
    /** Creates a new transition animation for the given @p cell. This takes the
      * current cache pixmap from the @p cell and creates a pixmap item with it
      * which fades out over the cell while the cell fades in, to create the
      * transition effect.
      * @param cell The crossword cell which should be animated.
      * @note You should make sure, that the appearance of the cell is changed
      * immediately after creating and starting this animation. Otherwise no
      * transition may be visible. */
    TransitionAnimation(KrossWordCell *cell);

    virtual ~TransitionAnimation();

    /** Creates a new pixmap with the cell and the current pixmap item drawn
      * into (with their current opacity values). */
    QPixmap composedCellPixmap() const;

    /** The cell that is animated. */
    KrossWordCell *cell() const {
        return m_cell;
    };

    /** A pixmap item that fades out over the cell to create the transition effect. */
    GraphicsPixmapObject *pixmapObject() const {
        return m_pixmapObject;
    };

signals:
    /** Emitted when the animation finishes, with the animated cell as argument. */
    void finished(KrossWordCell *cell);

protected slots:
    void appearanceAboutToChange();
    void cellDestroyed(QObject *obj);

protected:
    virtual void updateCurrentValue(const QVariant& value);
    virtual void updateState(State newState, State oldState);

private:
    KrossWordCell *m_cell;
    GraphicsPixmapObject *m_pixmapObject;
};

/** Manages animations. It can queue animations to start all enqueued animations
  * at once when they are ready.
  * There are some basic animations (see @ref BasicAnimation) that can be
  * started using @ref animate.
  * @brief Manages animations. */
class Animator : QObject
{
    Q_OBJECT

public:
    /** Different durations for animations. */
    enum Duration {
        Slowest,      /**< Five times the default duration. */
        VerySlow,     /**< Three times the default duration. */
        Slow,         /**< Two times the default duration. */
        DefaultDuration,  /**< Default duration, ie. defaultDuration(). */
        Fast,     /**< Two thirds of the default duration. */
        VeryFast,     /**< One third of the default duration.  */

        Instant       /**< Zero duration, ie. no animation. */
    };

    /** Basic animations that don't require a parameter. */
    enum BasicAnimation {
        AnimateFadeIn = 0,    /**< Animate to full opacity. */
        AnimateFadeOut,       /**< Animate to full transparency. */
        AnimateBounce         /**< Scales a bit in and out again with
                   * QEasingCurve::Bounce. */
    };

    /** Animations that require one parameter. */
    enum OneParameterAnimation {
        AnimatePositionChange = 100 /**< Animate moving from the current position
                     * to a new one (given as QPointF-argument). */
    };

    /** Flags for animation setup / start. */
    enum AnimateFlag {
        AnimateDontStart = 0x000, /**< Do not start the animation, if one was created. */
        AnimateStart = 0x001, /**< Start the animation, if one was created. */
        AnimateDeleteWhenStopped = 0x002, /**< Delete the animation when it stops. */

        AnimateDefaults = AnimateStart | AnimateDeleteWhenStopped
                          /**< Default animation flags, that automatically
                             * start the animation and delete it when it stops. */
    };
    Q_DECLARE_FLAGS(AnimateFlags, AnimateFlag);

    /** Creates an animation manager. */
    Animator() : QObject(), m_queuedAnimationGroup(0), m_queueBeginCount(0),
        m_enabled(true) {};

    /** The default duration of animations in milliseconds. */
    inline int defaultDuration() const {
        return 150;
    };

    /** Starts or enqueues the given @p animation. If not enqueued the @p flags
      * are used to determine how to start the animation.
      * @return True, if the animation was started (not enqueued). False, otherwise. */
    bool startOrEnqueue(QAbstractAnimation *animation, AnimateFlags flags = AnimateDefaults);

    /** Starts or enqueues the given @p animation. If not enqueued the @p flags
      * are used to determine how to start the animation.
      * @note This method also sets the duration of the animation, before
      * starting / enqueing it. It can be useful if you want to start / enqueue
      * custom animations and use @ref Duration to compute the actual duration.
      * @return True, if the animation was started (not enqueued). False, otherwise. */
    inline bool startOrEnqueue(QVariantAnimation *animation, Duration duration,
                               AnimateFlags flags = AnimateDefaults) {
        animation->setDuration(defaultDuration() * durationToDurationFactor(duration));
        return startOrEnqueue(animation, flags);
    };

    /** Starts a transition animation for the given @p cell. This is done by
      * creating a pixmap item from the cell's cached pixmap, which then gets
      * faded out, while the cell itself gets faded it.
      * @note For this to work, the cell's appearance should change, ie. it's
      * cache pixmap should be redrawn.
      * @param cell The cell to animate.
      * @param duration The duration of the animation.
      * @param flags Flags for the animation, eg. @ref AnimateDontStart. */
    TransitionAnimation* animateTransition(KrossWordCell *cell,
                                           Duration duration = DefaultDuration,
                                           AnimateFlags flags = AnimateDefaults);

    /** Whether or not the given @p cell is currently in a transition, ie. a
      * transition animation is running for that cell. */
    inline bool isInTransition(KrossWordCell *cell) const {
        return m_transitionItems.contains(cell)
               && m_transitionItems[ cell ]->state() == QAbstractAnimation::Running;
    };

    /** Performs @p basicAnimation on @p obj.
      * @param basicAnimation The animation to perform.
      * @param obj The object to apply the animation to.
      * @param argument An argument to the animation if needed or QVariant().
      * @param duration The duration of the animation.
      * @param flags Flags for the animation, eg. @ref AnimateDontStart.
      * @return The newly created animation. */
    QAbstractAnimation *animate(BasicAnimation basicAnimation,
                                QGraphicsObject *obj,
                                Duration duration = DefaultDuration,
                                AnimateFlags flags = AnimateDefaults);

    /** Performs @p basicAnimation on @p obj.
      * @note This is a convenience method for animations that need no argument.
      * You can't use it for animations that require an argument.
      * @param basicAnimation The animation to perform.
      * @param obj The object to apply the animation to.
      * @param duration The duration of the animation.
      * @param flags Flags for the animation, eg. @ref AnimateDontStart.
      * @return The newly created animation. */
    QAbstractAnimation *animate(OneParameterAnimation animation,
                                QGraphicsObject *obj, const QVariant &argument,
                                Duration duration = DefaultDuration,
                                AnimateFlags flags = AnimateDefaults);

    /** Creates a QParallelAnimationGroup and adds all following animations to
      * it until @ref endQueueAnimations is called. If this method is called
      * more then once before calling @ref endQueueAnimations no new animation
      * group gets created.
      * @note The number of calls to @ref beginQueueAnimations must match the
      * number of calls to @ref endQueueAnimations to stop adding animations
      * to the QParallelAnimationGroup. This also means that you can have
      * nested calls to those functions.
      * @see endEnqueueAnimations
      * @see isQueuingAnimations */
    void beginEnqueueAnimations();

    /** Starts the QParallelAnimationGroup created by a call to
      * @ref beginQueueAnimations. If @ref beginQueueAnimations wasn't called
      * before, nothing will be done.
      * @note You can call @ref beginQueueAnimations more than once before calling
      * this method. To start the animations in the animation group the number
      * of calls to @ref beginQueueAnimations must match the number of calls to
      * @ref endQueueAnimations.
      * @see beginEnqueueAnimations
      * @see isQueuingAnimations */
    void endEnqueueAnimations();

    /** Checks if animations are currently queued instead of started directly.
      * It will return true as long as the number of calls to
      * @ref endQueueAnimations is less than the number of calls to
      * @ref beginQueueAnimations.
      * @see beginEnqueueAnimations
      * @see endEnqueueAnimations
      * @see queuedAnimationCount */
    inline bool isQueuingAnimations() const {
        return m_queuedAnimationGroup;
    };

    /** Gets the number of queued animations.
      * @see beginEnqueueAnimations
      * @see endEnqueueAnimations
      * @see isQueuingAnimations */
    inline int queuedAnimationCount() const {
        return m_queuedAnimationGroup->animationCount();
    };

    /** Whether or not animations are enabled. If false, no animation will be
      * started, nor will it be queued.
      * @see setEnabled */
    inline bool isEnabled() const {
        return m_enabled;
    };
    /** Enables or disables animations. If set to false, no animation will be
      * started, nor will it be queued.
      * @see isEnabled */
    void setEnabled(bool enabled = true) {
        m_enabled = enabled;
    };

protected slots:
    /** Called, when a transition animation is finished. */
    void transitionAnimationFinished(KrossWordCell *cell);

private:
    qreal durationToDurationFactor(Duration duration) const;

    QParallelAnimationGroup *m_queuedAnimationGroup;
    int m_queueBeginCount;
    QHash< KrossWordCell*, TransitionAnimation* > m_transitionItems;
    bool m_enabled;
};

}; // namespace
Q_DECLARE_OPERATORS_FOR_FLAGS(Crossword::Animator::AnimateFlags)

#endif // ANIMATOR_H
