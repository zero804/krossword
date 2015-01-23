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

#include "templatemodel.h"

#include <QFileSystemModel>
#include <QDebug>

TemplateModel::TemplateModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    m_fileSystemModel = new QFileSystemModel;
    m_fileSystemModel->setNameFilterDisables(false);
    m_fileSystemModel->setNameFilters(QStringList() << "*.kwpz" << "*.kwp");
    setSourceModel(m_fileSystemModel);
}

TemplateModel::~TemplateModel()
{
    delete m_fileSystemModel;
}

void TemplateModel::setRootPath(const QString& rootPath)
{
    m_fileSystemModel->setRootPath(rootPath);
}

QString TemplateModel::filePath(const QModelIndex& index) const
{
    return m_fileSystemModel->filePath(mapToSource(index));
}

void TemplateModel::setFilter(QDir::Filter filter)
{
    m_fileSystemModel->setFilter(filter);
}

QModelIndex TemplateModel::indexForPath(const QString& path,
        int column) const
{
    return mapFromSource(m_fileSystemModel->index(path, column));
}

QModelIndex TemplateModel::mapFromSource(
    const QModelIndex& sourceIndex) const
{
    QModelIndex rootIndex = m_fileSystemModel->index(m_fileSystemModel->rootPath());
    if (sourceIndex == rootIndex)
        return QModelIndex();
    else
        return QSortFilterProxyModel::mapFromSource(sourceIndex);
}

QModelIndex TemplateModel::mapToSource(
    const QModelIndex& proxyIndex) const
{
    QModelIndex rootIndex = m_fileSystemModel->index(m_fileSystemModel->rootPath());
    if (!proxyIndex.isValid())
        return rootIndex;
    else
        return QSortFilterProxyModel::mapToSource(proxyIndex);
}
