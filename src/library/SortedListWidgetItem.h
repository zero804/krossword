#ifndef SORTEDLISTWIDGETITEM_H
#define SORTEDLISTWIDGETITEM_H

#include <QListWidgetItem>

class SortedListWidgetItem : public QListWidgetItem
{
public:
    SortedListWidgetItem(const QString& display)
        : QListWidgetItem(display)
    { }

    bool operator<(const QListWidgetItem &other) const {
        return data(Qt::UserRole + 1) < other.data(Qt::UserRole + 1);
    }
};

#endif // SORTEDLISTWIDGET_H
