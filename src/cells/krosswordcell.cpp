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

#include "krosswordcell.h"
#include "krossword.h"
#include "krosswordtheme.h"
#include "krosswordrenderer.h"
#include "cells/cluecell.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <qevent.h>

#include <QDebug>

#include <QPropertyAnimation>
#include <QGraphicsEffect>
#include "animator.h"

namespace Crossword
{

void GlowEffect::draw(QPainter* painter)
{
    drawSource(painter);

    painter->save();
    painter->setClipRegion(QRegion(boundingRect().toRect()).subtracted(QRegion(sourceBoundingRect().toRect())));
    QGraphicsDropShadowEffect::draw(painter);
    painter->restore();
}

KrossWordCell::KrossWordCell(KrossWord* krossWord, CellType cellType, const Coord& coord)
    : QGraphicsObject(krossWord),
      m_blockCacheClearing(false),
      m_cache(0),
      m_redraw(true),
      m_blurAnim(0)
{

    m_krossWord = krossWord;
    m_cellType = cellType;
    m_coord = coord;
    m_highlight = false;

    setFlag(QGraphicsItem::ItemIsFocusable);
    setFlag(QGraphicsItem::ItemIsSelectable);

    setPositionFromCoordinates(false);

    // Set new cells completely transparent, they will fade in when they are
    // added to the crossword by KrossWord::replaceCell.
    setOpacity(0);

    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);

    // Create a glow effect (enabled by focus in)
    GlowEffect *effect = new GlowEffect;
    effect->setOffset(0);
    effect->setEnabled(false);
    setGraphicsEffect(effect);
}

KrossWordCell::~KrossWordCell()
{
    if (scene()) {
        scene()->removeItem(this);
    }

    removeSynchronization();
    delete m_cache;
}

int KrossWordCell::type() const {
    return Type;
}

CellType KrossWordCell::getCellType() const {
    return m_cellType;
}

bool KrossWordCell::isType(CellType cellType) const {
    return m_cellType == cellType;
}

bool KrossWordCell::isLetterCell() const {
    return false;
}

KrossWord *KrossWordCell::krossWord() const {
    return m_krossWord;
}

Grid2D::Coord KrossWordCell::coord() const {
    return m_coord;
}

QPixmap KrossWordCell::pixmap() const {
    return *m_cache;
}

qreal KrossWordCell::scaleX() const
{
    return transform().m11();
}

void KrossWordCell::setScaleX(qreal scaleX)
{
    // No need to translate any longer, since the bounding rect is now adjusted
    // to have the items position in it's center.
//   translate( transformOriginPoint().x(), transformOriginPoint().y() );
    setTransform(QTransform(scaleX / transform().m11(), 0, 0, 1, 0, 0), true);
//   translate( -transformOriginPoint().x(), -transformOriginPoint().y() );
}

QList<SyncCategory> KrossWordCell::allSynchronizationCategories() const {
    return QList< SyncCategory >() << OtherSynchronization
                                   << SolutionLetterSynchronization
                                   << SameCharacterLetterSynchronization;
}

const QHash<SyncCategory, QHash<KrossWordCell *, SyncMethods> > &KrossWordCell::synchronizationByCategory() const {
    return m_synchronizedCells;
}

void KrossWordCell::setCoord(Coord newCoord, bool updateInCrosswordGrid)
{
    if (updateInCrosswordGrid) {
        // For clue cells (and maybe others): Only remove from the old position
        // if this cell is at that position (don't remove letter cells when the clue
        // cell is hidden).
        if (krossWord()->at(coord()) == this) {
            krossWord()->removeCell(coord(), false);
        }

        krossWord()->replaceCell(newCoord, this);
    } else {
        m_coord = newCoord;
    }

    emit cellMoved(newCoord);
}

bool KrossWordCell::setPositionFromCoordinates(bool animate)
{
    QPointF newPos((coord().first + 0.5) * krossWord()->getCellSize().width(),
                   (coord().second + 0.5) * krossWord()->getCellSize().height());

    if (animate && krossWord()->isAnimationEnabled() && this->getCellType() != ImageCellType) {
        krossWord()->m_animator->animate(Animator::AnimatePositionChange, this, newPos, Animator::VerySlow);
//       connect( anim, SIGNAL(finished()), this, SLOT(updateTransformOriginPoint()) );
    } else {
        setPos(newPos);
//     updateTransformOriginPoint();
    }

    return true;
}

// void KrossWordCell::updateTransformOriginPoint() {
//   setTransformOriginPoint( 0, 0 );
// //   setTransformOriginPoint( boundingRect().width() / 2,
// //       boundingRect().height() / 2 );
// }

void KrossWordCell::clearCache(Animator::Duration duration)
{
    if (m_blockCacheClearing) {   // Only used with Qt 4.6 to wait for animations
        return;
    }

    if (m_cache && !m_redraw) {
        emit appearanceAboutToChange();
        if (duration != Animator::Instant && krossWord()->isAnimationEnabled() && this->getCellType() != ImageCellType) {
            krossWord()->m_animator->animateTransition(this, duration);
        }
    }

    m_redraw = true;
    update();
}

void KrossWordCell::synchronizeWith(const KrossWordCellList &cellList,
                                    SyncMethods syncMethods,
                                    SyncCategory syncCategory)
{
    foreach(KrossWordCell * cell, cellList) {
        synchronizeWith(cell, syncMethods, syncCategory);
    }
}

void KrossWordCell::synchronizeWith(KrossWordCell* cell,
                                    SyncMethods syncMethods,
                                    SyncCategory syncCategory)
{
    Q_ASSERT(cell);
    if (cell == this) {
        return; // Don't sync cell to itself
    }

    bool syncContent = syncMethods.testFlag(SyncContent);
    bool syncSelection = syncMethods.testFlag(SyncSelection);
    bool contentSynced = isSynchronizedWith(cell, SyncContent);
    bool selectionSynced = isSynchronizedWith(cell, SyncSelection);
    if ((contentSynced || !syncContent) && (selectionSynced || !syncSelection)) {
        return;
    }

//     qDebug() << "Synchronize" << coord() << "with" << cell->coord()
//  << "syncMethods =" << syncMethods << "syncCategory =" << syncCategory;
    if (syncContent && !contentSynced) {
        connect(cell, SIGNAL(currentLetterChanged(LetterCell*, QChar)), this, SLOT(setCurrentLetterSlot(LetterCell*, QChar)));
        connect(this, SIGNAL(currentLetterChanged(LetterCell*, QChar)), cell, SLOT(setCurrentLetterSlot(LetterCell*, QChar)));
    }

    if (syncSelection && !selectionSynced) {
        connect(cell, SIGNAL(gotFocus(KrossWordCell*)), this, SLOT(setFocusSlot(KrossWordCell*)));
        connect(this, SIGNAL(gotFocus(KrossWordCell*)), cell, SLOT(setFocusSlot(KrossWordCell*)));
    }

    if (m_synchronizedCells[syncCategory].contains(cell)) {
        Q_ASSERT(cell->m_synchronizedCells[syncCategory].contains(this));
        cell->m_synchronizedCells[syncCategory][this] |= syncMethods;
        this->m_synchronizedCells[syncCategory][cell] |= syncMethods;
    } else {
        cell->m_synchronizedCells[syncCategory].insert(this, syncMethods);
        this->m_synchronizedCells[syncCategory].insert(cell, syncMethods);
    }
}

bool KrossWordCell::isSynchronizedWith(KrossWordCell* cell,
                                       SyncCategory syncCategory,
                                       SyncMethods syncMethods)
{
    // Return true, if at least one of syncMethods is synced in syncCategory
    return m_synchronizedCells.value(syncCategory).contains(cell)
           && (m_synchronizedCells.value(syncCategory).value(cell) & syncMethods) != 0;
}

bool KrossWordCell::isSynchronizedWith(KrossWordCell* cell,
                                       SyncMethods syncMethods,
                                       SyncCategories syncCategories)
{
    QList< SyncCategory > cats = allSynchronizationCategories();
    foreach(const SyncCategory & cat, cats) {
        if (!syncCategories.testFlag(cat)) {
            continue;
        }

        if (isSynchronizedWith(cell, cat, syncMethods)) {
            return true;
        }
    }

    return false;
}

bool KrossWordCell::removeSynchronizationWith(KrossWordCell* cell,
        SyncMethods syncMethods,
        SyncCategories syncCategories)
{
    if (!isSynchronizedWith(cell)) {
        qDebug() << "Not Synchronized";
        return false;
    }
    Q_ASSERT(cell->isSynchronizedWith(this));

    QList< SyncCategory > cats = allSynchronizationCategories();
    bool stillSyncedContent = false, stillSyncedSelection = false;
    foreach(const SyncCategory & cat, cats) {
        if (syncCategories.testFlag(cat)) {
            cell->m_synchronizedCells[ cat ][this] &= ~syncMethods;
            this->m_synchronizedCells[ cat ][cell] &= ~syncMethods;

            if (this->m_synchronizedCells[ cat ][cell] == SyncNothing) {
                cell->m_synchronizedCells[ cat ].remove(this);
                this->m_synchronizedCells[ cat ].remove(cell);
            }
        } else if (!stillSyncedContent && m_synchronizedCells[cat].contains(cell)
                   && m_synchronizedCells[cat][cell].testFlag(SyncContent)) {
            stillSyncedContent = true;
        } else if (!stillSyncedSelection && m_synchronizedCells[cat].contains(cell)
                   && m_synchronizedCells[cat][cell].testFlag(SyncSelection)) {
            stillSyncedSelection = true;
        }
    }

    if (!stillSyncedContent && syncMethods.testFlag(SyncContent)
            && isLetterCell() && cell->isLetterCell()) {
        disconnect(cell, SIGNAL(currentLetterChanged(LetterCell*, QChar)), this, SLOT(setCurrentLetterSlot(LetterCell*, QChar)));
        disconnect(this, SIGNAL(currentLetterChanged(LetterCell*, QChar)), cell, SLOT(setCurrentLetterSlot(LetterCell*, QChar)));
    }

    if (!stillSyncedSelection && syncMethods.testFlag(SyncSelection)) {
        disconnect(cell, SIGNAL(gotFocus(KrossWordCell*)), this, SLOT(setFocusSlot(KrossWordCell*)));
        disconnect(this, SIGNAL(gotFocus(KrossWordCell*)), cell, SLOT(setFocusSlot(KrossWordCell*)));
    }

    return true;
}

QString KrossWordCell::syncInfoString() const
{
    QString info = QString("%1,%2,%3")
                   .arg(m_synchronizedCells[SolutionLetterSynchronization].count())
                   .arg(m_synchronizedCells[SameCharacterLetterSynchronization].count())
                   .arg(m_synchronizedCells[OtherSynchronization].count());

    return info;
}

void KrossWordCell::removeSynchronization(SyncMethods syncMethods,
        SyncCategories syncCategories)
{
    QList< SyncCategory > cats = allSynchronizationCategories();
    foreach(const SyncCategory & cat, cats) {
        if (!syncCategories.testFlag(cat)) {
            continue;
        }

        for (int i = m_synchronizedCells[cat].count() - 1; i >= 0; --i) {
            KrossWordCell *cell = m_synchronizedCells[cat].keys()[ i ];
            removeSynchronizationWith(cell, syncMethods, cat);
        }
    }
}

void KrossWordCell::setFocusSlot(KrossWordCell* cell)
{
    Q_UNUSED(cell);   // TODO: remove cell parameter?

    setFocus(); // TEST
}

void KrossWordCell::deleteAndRemoveFromSceneLater()
{
    if (scene()) {
        scene()->removeItem(this);
    }
    setFlag(ItemIsFocusable, false);
    deleteLater();
}

QRectF KrossWordCell::boundingRect() const
{
    qreal width = krossWord()->getCellSize().width();
    qreal height = krossWord()->getCellSize().height();
    return QRectF(-width / 2 - 0.5, -height / 2 - 0.5, width + 1, height + 1);
}

QVariant KrossWordCell::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedChange) {
        update();
    }
    return value;
}

void KrossWordCell::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (!krossWord()->acceptedMouseButtons().testFlag(event->button())) {
        event->ignore();
        return;
    }

    if (!krossWord()->isInteractive()) {
        QGraphicsItem::mousePressEvent(event);
        return;
    }

    krossWord()->emitMousePressed(event->scenePos(), event->button(), this);

    if (event->button() == Qt::RightButton) {
        if (isLetterCell() || getCellType() == EmptyCellType) {
            krossWord()->setHighlightedClue(nullptr);
            setHighlight();
        }
        krossWord()->emitCustomContextMenuRequested(event->scenePos(), this);
        event->accept();
    } else {
        event->accept();
        QGraphicsItem::mousePressEvent(event);
    }
}

void KrossWordCell::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
}

void KrossWordCell::focusInEvent(QFocusEvent* event)
{
    QGraphicsItem::focusInEvent(event);

    krossWord()->setCurrentCell(this);
    emit gotFocus(this);   // Used for focus synchronization with letter cells in a separate solution word KrossWord.

    clearCache();

    GlowEffect *effect = static_cast< GlowEffect* >(graphicsEffect());
    if (effect) {
        effect->setEnabled(true);
        if (krossWord()->isAnimationEnabled() && this->getCellType() != ImageCellType) {
            if (m_blurAnim) {
                disconnect(m_blurAnim, SIGNAL(finished()), 0, 0);
                connect(m_blurAnim, SIGNAL(finished()),
                        this, SLOT(blurAnimationInFinished()));
                m_blurAnim->setStartValue(m_blurAnim->currentValue().toInt());
                m_blurAnim->setEndValue(10);
                m_blurAnim->setCurrentTime(0);
            } else {
                m_blurAnim = new QPropertyAnimation(effect, "blurRadius");
                m_blurAnim->setDuration(krossWord()->animator()->defaultDuration() * 3);
                m_blurAnim->setStartValue(effect->blurRadius());
                m_blurAnim->setEndValue(10);
                m_blurAnim->setEasingCurve(QEasingCurve(QEasingCurve::OutCirc));
                connect(m_blurAnim, SIGNAL(finished()),
                        this, SLOT(blurAnimationInFinished()));
                m_blurAnim->start();
            }

            effect->setColor(krossWord()->theme()->glowFocusColor());
        } else {
            effect->setColor(krossWord()->theme()->glowFocusColor());
            effect->setBlurRadius(10);
        }
        setZValue(5);
    }

    if (krossWord()->isAnimationEnabled() && this->getCellType() != ImageCellType) {
        krossWord()->animator()->animate(Animator::AnimateBounce, this, Crossword::Animator::Slow);
    }
}

void KrossWordCell::blurAnimationInFinished()
{
    delete m_blurAnim;
    m_blurAnim = nullptr;
}

void KrossWordCell::blurAnimationOutFinished()
{
    delete m_blurAnim;
    m_blurAnim = nullptr;

    GlowEffect *effect = static_cast< GlowEffect* >(graphicsEffect());
    if (effect) {
        effect->setEnabled(false);
    }
}

void KrossWordCell::clearCacheAndUpdate() {
    clearCache(Crossword::Animator::Instant);
    update();
}

void KrossWordCell::focusOutEvent(QFocusEvent* event)
{
    GlowEffect *effect = static_cast< GlowEffect* >(graphicsEffect());
    if (effect) {
        if (krossWord()->isAnimationEnabled() && this->getCellType() != ImageCellType) {
            if (m_blurAnim) {
                disconnect(m_blurAnim, SIGNAL(finished()), 0, 0);
                connect(m_blurAnim, SIGNAL(finished()),
                        this, SLOT(blurAnimationOutFinished()));
                m_blurAnim->setStartValue(m_blurAnim->currentValue().toInt());
                m_blurAnim->setEndValue(0);
                m_blurAnim->setCurrentTime(0);
            } else {
                m_blurAnim = new QPropertyAnimation(effect, "blurRadius");
                m_blurAnim->setDuration(krossWord()->animator()->defaultDuration() * 3);
                m_blurAnim->setStartValue(effect->blurRadius());
                m_blurAnim->setEasingCurve(QEasingCurve(QEasingCurve::InOutCirc));
                m_blurAnim->setEndValue(0);
                connect(m_blurAnim, SIGNAL(finished()),
                        this, SLOT(blurAnimationOutFinished()));
                m_blurAnim->start();
            }
        } else {
            effect->setEnabled(false);
        }
        setZValue(0);
    }

    clearCache();
    update();
    QGraphicsItem::focusOutEvent(event);
}

void KrossWordCell::setHighlight(bool enable)
{
    m_highlight = enable;

    if (enable) {
        clearCache(Animator::Instant);   // No animation when getting highlight
    } else {
        clearCache(Animator::VerySlow);   // Longer animation when loosing highlight
    }

    update();
}


EmptyCell::EmptyCell(KrossWord* krossWord, Coord coord) : KrossWordCell(krossWord, EmptyCellType, coord)
{
    setFlag(QGraphicsItem::ItemIsFocusable, krossWord->isEditable());
    setFlag(QGraphicsItem::ItemIsSelectable, krossWord->isEditable());

    //setFlag(QGraphicsItem::ItemHasNoContents, !krossWord->isEditable());
}

LetterCell* EmptyCell::toLetterCell(const QChar& correctContent)
{
    Q_ASSERT(!krossWord()->letterContentToClueNumberMapping().isEmpty());

    int clueNumber = krossWord()->letterContentToClueNumberMapping().indexOf(correctContent) + 1;
    ClueCell *numberClue = new ClueCell(krossWord(), coord(), Qt::Horizontal, OnClueCell, clueNumber <= 0 ? QString() : QString::number(clueNumber), correctContent);
    LetterCell *letter = new LetterCell(krossWord(), coord(), numberClue);

    krossWord()->replaceCell(coord(), letter, false);
    deleteLater();

    return letter;
}

void EmptyCell::focusInEvent(QFocusEvent* event)
{
    krossWord()->setHighlightedClue(nullptr);
    if (krossWord()->isEditable()) {
        setHighlight();
    }
    KrossWordCell::focusInEvent(event);
}

void EmptyCell::focusOutEvent(QFocusEvent* event)
{
    // Only remove highlight if the scene still has focus
    if (scene() && scene()->hasFocus()) {
        setHighlight(false);
    }
    KrossWordCell::focusOutEvent(event);
}

void EmptyCell::keyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event);
    // TODO
//     if ( krossWord()->isEditable() && event->text().length() >= 1
//      && krossWord()->crosswordTypeInfo().isCharacterLegal(event->text().at(0))
//      && krossWord()->crosswordTypeInfo().clueType == KrossWord::NumberClues1To26
//      && krossWord()->crosswordTypeInfo().clueMapping == KrossWord::CluesReferToCells
//      && krossWord()->crosswordTypeInfo().letterCellContent == KrossWord::Characters ) {
//  QChar newChar = event->text().toUpper()[0];
//  toLetterCell( newChar );
//     }
}

void EmptyCell::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (!krossWord()->isEditable()) {
        event->ignore();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        setHighlight();
        event->accept();
    } else {
        event->ignore();
    }

    KrossWordCell::mousePressEvent(event);
}

void EmptyCell::drawBackground(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    if (KrosswordRenderer::self()->hasElement("empty_cell")) {
        KrosswordRenderer::self()->renderElement(p, "empty_cell", option->rect);
    } else {
        // The empty cell isn't themed in the current theme
        p->fillRect(option->rect, Qt::black);
        p->drawRect(option->rect.adjusted(0, 0, -1, -1));
    }

    if (krossWord()->isEditable()) { // in editor mode
        if (isHighlighted()) {
            QColor selColor = krossWord()->theme()->selectionColor();
            selColor.setAlpha(128);
            p->fillRect(option->rect, selColor);
            p->drawRect(option->rect.adjusted(0, 0, -1, -1));
        }
    }
}

void EmptyCell::drawBackgroundForPrinting(QPainter* p, const QStyleOptionGraphicsItem* option)
{
    p->fillRect(option->rect, krossWord()->emptyCellColorForPrinting());
}

void KrossWordCell::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    if (krossWord()->isDrawingForPrinting()) {
        drawBackgroundForPrinting(painter, option);
        drawForegroundForPrinting(painter, option);
    } else {
        qreal levelOfDetail = QStyleOptionGraphicsItem::levelOfDetailFromTransform(QTransform(option->matrix));
        QSize size = option->rect.size() * levelOfDetail;
        //qDebug() << "SIZE =" << size << "FROM" << option->rect.size() << "*" << levelOfDetail;

        if (m_cache) {
            if (m_cache->size() != size) {
                delete m_cache;
                m_cache = new QPixmap(size);   // Create cache pixmap with new size
                m_redraw = true;
            }
        } else {
            m_cache = new QPixmap(size);   // Create cache pixmap
            m_redraw = true;
        }

        if (m_redraw) {
            m_redraw = false;

            m_cache->fill(Qt::transparent);
            QPainter p(m_cache);
            p.translate(-option->rect.topLeft());

            QStyleOptionGraphicsItem *scaledOption = new QStyleOptionGraphicsItem(*option);
            scaledOption->rect.setWidth(option->rect.width() * levelOfDetail);
            scaledOption->rect.setHeight(option->rect.height() * levelOfDetail);
            drawBackground(&p, scaledOption);
            drawForeground(&p, scaledOption);
            p.end();
        }
//  if ( parentItem() && qgraphicsitem_cast<DoubleClueCell*>(parentItem()) ) {
//    qDebug() << "Draw pixmap of clue in 2clue" << option->rect << this;
        painter->setRenderHints(QPainter::SmoothPixmapTransform);
        painter->drawPixmap(option->rect, *m_cache);
//  } else
//    painter->drawPixmap( option->rect.topLeft(), *m_cache );

//  painter->setRenderHints( QPainter::SmoothPixmapTransform );
//  painter->drawPixmap( option->rect.topLeft(), *m_cache );
    }
}

bool KrossWordCell::isHighlighted() const
{
    return m_highlight && !krossWord()->isDrawingForPrinting();
}


// Sorting functions:
bool lessThanCellType(const KrossWordCell* cell1, const KrossWordCell* cell2)
{
    return cell1->getCellType() < cell2->getCellType();
}

bool greaterThanCellType(const KrossWordCell* cell1, const KrossWordCell* cell2)
{
    return cell1->getCellType() > cell2->getCellType();
}

}
// namespace Crossword
