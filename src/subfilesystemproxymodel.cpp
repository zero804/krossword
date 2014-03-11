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

#include "subfilesystemproxymodel.h"

#include <QFileSystemModel>
#include <QDebug>

SubFileSystemProxyModel::SubFileSystemProxyModel( QObject* parent )
	: QSortFilterProxyModel( parent ) {
  m_fileSystemModel = new QFileSystemModel;
  m_fileSystemModel->setNameFilterDisables( false );
  m_fileSystemModel->setNameFilters( QStringList() << "*.kwpz" << "*.kwp" );
  setSourceModel( m_fileSystemModel );
}

SubFileSystemProxyModel::~SubFileSystemProxyModel() {
  delete m_fileSystemModel;
}

void SubFileSystemProxyModel::setRootPath( const QString& rootPath ) {
  m_fileSystemModel->setRootPath( rootPath );
}

QString SubFileSystemProxyModel::filePath( const QModelIndex& index ) const {
  return m_fileSystemModel->filePath( mapToSource(index) );
}

void SubFileSystemProxyModel::setFilter( QDir::Filter filter ) {
  m_fileSystemModel->setFilter( filter );
}

QModelIndex SubFileSystemProxyModel::indexForPath( const QString& path,
						   int column ) const {
  return mapFromSource( m_fileSystemModel->index(path, column) );
}

QModelIndex SubFileSystemProxyModel::mapFromSource(
	    const QModelIndex& sourceIndex ) const {
  QModelIndex rootIndex = m_fileSystemModel->index( m_fileSystemModel->rootPath() );
  if ( sourceIndex == rootIndex )
    return QModelIndex();
  else
    return QSortFilterProxyModel::mapFromSource( sourceIndex );
}

QModelIndex SubFileSystemProxyModel::mapToSource(
	    const QModelIndex& proxyIndex ) const {
  QModelIndex rootIndex = m_fileSystemModel->index( m_fileSystemModel->rootPath() );
  if ( !proxyIndex.isValid() )
    return rootIndex;
  else
    return QSortFilterProxyModel::mapToSource( proxyIndex );
}
