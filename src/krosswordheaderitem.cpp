#include "krosswordheaderitem.h"
#include "krossword.h"
#include "krosswordtheme.h"

#include <QGraphicsScene>
#include <QFontDatabase>

namespace Crossword
{

KrossWordHeaderItem::KrossWordHeaderItem(QGraphicsItem* parent)
    : QGraphicsObject(parent),
    m_titleItem(0), m_authorsItem(0)
{

}

QRectF KrossWordHeaderItem::boundingRect() const
{
    if (!m_titleItem)
        return QRectF(0, 0, 0, 0);
    else if (m_authorsItem) {
        return QRectF(0, 0, qMax(m_titleItem->boundingRect().width(),
                                 m_authorsItem->boundingRect().width()),
                      m_titleItem->boundingRect().height() +
                      m_authorsItem->boundingRect().height() + 10);
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

    if (m_authorsItem) {
        m_authorsItem->setDefaultTextColor(color);
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

        QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
        font.setBold(true);
        font.setPointSize(20);
        m_titleItem->setFont(font);
        m_titleItem->setDefaultTextColor(krossWord->theme()->fontColor());
        m_titleItem->setTextWidth(krossWord->boundingRect().width()); // max width

        m_titleItem->setPlainText(krossWord->getTitle());
    }

    if (krossWord->getAuthors().isEmpty()) {
        if (m_authorsItem) {
            scene()->removeItem(m_authorsItem);
            delete m_authorsItem;
            m_authorsItem = NULL;
        }
    } else {
        if (!m_authorsItem) {
            m_authorsItem = new QGraphicsTextItem(this); //CHECK
        }

        QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
        font.setPointSize(12);
        m_authorsItem->setFont(font);
        m_authorsItem->setDefaultTextColor(krossWord->theme()->fontColor());
        m_authorsItem->setTextWidth(krossWord->boundingRect().width());

        if (krossWord->getCopyright().isEmpty()) {
            m_authorsItem->setPlainText(krossWord->getAuthors());
        } else {
            m_authorsItem->setHtml(QString("<p style=\"text-align:right;\">%1<br>%2</p>").arg(krossWord->getAuthors()).arg(krossWord->getCopyright()));
        }
    }

    //prepareGeometryChange();
    crosswordResized(krossWord, krossWord->width(), krossWord->height());
}

void KrossWordHeaderItem::crosswordResized(KrossWord *krossWord, int columns, int rows)
{
    Q_UNUSED(columns);
    Q_UNUSED(rows);

    qreal y = 0;
    if (m_titleItem) {
        //m_titleItem->setPos((krossWord->boundingRect().width() - m_titleItem->boundingRect().width())/2, 0);
        m_titleItem->setPos(krossWord->boundingRect().left(), 0);
        y = m_titleItem->boundingRect().height();
    }

    if (m_authorsItem) {
        //m_authorsItem->setPos(krossWord->boundingRect().width() - m_authorsItem->boundingRect().width(), y);
        m_authorsItem->setPos(m_titleItem->boundingRect().left(), y);
    }
}

} // Crossword namespace
