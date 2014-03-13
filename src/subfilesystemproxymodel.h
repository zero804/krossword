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

#ifndef SUBFILESYSTEMPROXYMODEL_H
#define SUBFILESYSTEMPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QDir>

class QFileSystemModel;
class SubFileSystemProxyModel : public QSortFilterProxyModel
{
public:
    SubFileSystemProxyModel(QObject* parent = 0);
    virtual ~SubFileSystemProxyModel();

    void setRootPath(const QString &rootPath);
    QString filePath(const QModelIndex &index) const;
    void setFilter(QDir::Filter filter);
    QModelIndex indexForPath(const QString &path, int column = 0) const;

    virtual QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;
    virtual QModelIndex mapToSource(const QModelIndex& proxyIndex) const;

private:
    QFileSystemModel *m_fileSystemModel;
};

#endif // SUBFILESYSTEMPROXYMODEL_H
