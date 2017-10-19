#ifndef HEADERITEM_H
#define HEADERITEM_H

#include <QGraphicsObject>

namespace Crossword
{

class KrossWord;

class HeaderItem : public QGraphicsObject
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    HeaderItem(QGraphicsItem *parent = 0);

    void setContent(KrossWord *krossWord);
    void updateTheme(KrossWord *krossWord);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

public slots:
    void setContentPos(KrossWord *krossWord, int columns, int rows);

private:
    QGraphicsTextItem *m_titleItem;
    QGraphicsTextItem *m_copyrightItem;
};

}

#endif // HEADERITEM_H
