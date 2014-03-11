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

#ifndef CLUEMODEL_H
#define CLUEMODEL_H

#include <QStandardItemModel>
#include <KDebug>

namespace Crossword {
  class ClueCell;
}
using namespace Crossword;

class ClueItem : public QObject, public QStandardItem {
    Q_OBJECT
    
    public:
	ClueItem( const QIcon &icon, ClueCell *clueCell );

	virtual QVariant data( int role = Qt::UserRole + 1 ) const;
	virtual void setData( const QVariant& value, int role = Qt::UserRole + 1 );
	QStandardItem *parent() const { return QStandardItem::parent(); };

	ClueCell *clueCell() const { return m_clueCell; };

    signals:
	void changeClueTextRequest( ClueCell *clue, const QString &newClueText );

    private:
	ClueCell *m_clueCell;
};

class ClueModel : public QStandardItemModel {
    Q_OBJECT
    
    public:
	ClueModel( QObject* parent = 0 );

	QStandardItem *horizontalCluesItem() const { return m_itemHorizontal; };
	QStandardItem *verticalCluesItem() const { return m_itemVertical; };

	ClueItem *clueItem( ClueCell *clueCell ) const;
	QStandardItem *answerItem( ClueCell *clueCell ) const;
	ClueItem *clueItemFromIndex( const QModelIndex &index ) const;

	void addClue( ClueCell *clueCell );
	void removeClue( ClueCell *clueCell );

	void clear();

    signals:
	void changeClueTextRequest( ClueCell *clue, const QString &newClueText );
	
    protected slots:
	void updateClueOrientation( ClueCell *clue, Qt::Orientation newOrientation );
	void updateClueText( ClueCell *clue, const QString &newText );
	void changeClueTextRequested( ClueCell *clue, const QString &newClueText ) {
	    emit changeClueTextRequest( clue, newClueText ); };

    private:
	QStandardItem *m_itemHorizontal;
	QStandardItem *m_itemVertical;
	QString m_iconSad;
};

#endif // CLUEMODEL_H
