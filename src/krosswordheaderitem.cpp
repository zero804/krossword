#include "krosswordheaderitem.h"
#include "krossword.h"
#include "krosswordtheme.h"

#include <QGraphicsScene>
#include <QFontDatabase>

namespace Crossword
{

KrossWordHeaderItem::KrossWordHeaderItem(QGraphicsItem* parent)
    : QGraphicsObject(parent),
    m_titleItem(0), m_copyrightItem(0)
{
    this->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
}

QRectF KrossWordHeaderItem::boundingRect() const
{
    if (!m_titleItem)
        return QRectF(0, 0, 0, 0);
    else if (m_copyrightItem) {
        return QRectF(0, 0, qMax(m_titleItem->boundingRect().width(),
                                 m_copyrightItem->boundingRect().width()),
                      m_titleItem->boundingRect().height() +
                      m_copyrightItem->boundingRect().height() + 10);
    } else {
        return QRectF(0, 0, m_titleItem->boundingRect().width(), m_titleItem->boundingRect().height() + 10);
    }
}

void KrossWordHeaderItem::paint(QPainter* painter,
                               const QStyleOptionGraphicsItem* option,
                               QWidget* widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);
}

void KrossWordHeaderItem::updateTheme(KrossWord* krossWord)
{
    QColor color = krossWord->theme()->fontColor();

    if (m_titleItem) {
        m_titleItem->setDefaultTextColor(color);
    }

    if (m_copyrightItem) {
        m_copyrightItem->setDefaultTextColor(color);
    }
}

void KrossWordHeaderItem::setContent(KrossWord *krossWord)
{
    if (krossWord->getTitle().isEmpty()) {
        if (m_titleItem) {
            scene()->removeItem(m_titleItem);
            delete m_titleItem;
            m_titleItem = NULL;
        }
    } else {
        if (!m_titleItem) {
            m_titleItem = new QGraphicsTextItem(this); //CHECK
        }

        QFont font = QFontDatabase::systemFont(QFontDatabase::TitleFont);
        font.setPointSize(12);
        m_titleItem->setFont(font);
        m_titleItem->setDefaultTextColor(krossWord->theme()->fontColor());
        m_titleItem->setTextWidth(krossWord->boundingRect().width()); // max width

        m_titleItem->setHtml(QString(i18n("<strong>%1</strong> by %2")).arg(krossWord->getTitle()).arg(krossWord->getAuthors()));
    }

    if (krossWord->getCopyright().isEmpty()) {
        if (m_copyrightItem) {
            scene()->removeItem(m_copyrightItem);
            delete m_copyrightItem;
            m_copyrightItem = NULL;
        }
    } else {
        if (!m_copyrightItem) {
            m_copyrightItem = new QGraphicsTextItem(this); //CHECK
        }

        QFont font = QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont);
        m_copyrightItem->setFont(font);
        m_copyrightItem->setDefaultTextColor(krossWord->theme()->fontColor());
        m_copyrightItem->setTextWidth(krossWord->boundingRect().width());

        m_copyrightItem->setPlainText(krossWord->getCopyright());
    }

    //prepareGeometryChange();
    setContentPos(krossWord, krossWord->width(), krossWord->height());
}

void KrossWordHeaderItem::setContentPos(KrossWord *krossWord, int columns, int rows)
{
    Q_UNUSED(columns);
    Q_UNUSED(rows);

    qreal y = 0;
    if (m_titleItem) {
        //m_titleItem->setPos((krossWord->boundingRect().width() - m_titleItem->boundingRect().width())/2, 0);
        m_titleItem->setPos(krossWord->boundingRect().left(), 0);
        y = m_titleItem->boundingRect().height();
    }

    if (m_copyrightItem) {
        //m_authorsItem->setPos(krossWord->boundingRect().width() - m_authorsItem->boundingRect().width(), y);
        m_copyrightItem->setPos(m_titleItem->boundingRect().left(), y);
    }
}

} // Crossword namespace
