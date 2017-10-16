#ifndef KROSSWORDHEADERITEM_H
#define KROSSWORDHEADERITEM_H

#include <QGraphicsObject>

namespace Crossword
{

class KrossWord;

class KrossWordHeaderItem : public QGraphicsObject
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    KrossWordHeaderItem(QGraphicsItem *parent = 0);

    void setContent(KrossWord *krossWord);
    void updateTheme(KrossWord *krossWord);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

public slots:
    void crosswordResized(KrossWord *krossWord, int columns, int rows);

private:
    QGraphicsTextItem *m_titleItem;
    QGraphicsTextItem *m_authorsItem;
};

}

#endif // KROSSWORDHEADERITEM_H
