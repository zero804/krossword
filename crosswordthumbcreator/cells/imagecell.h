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

#ifndef IMAGECELL_H
#define IMAGECELL_H

#include "spannedcell.h"
#include <KUrl>

class ImageCell : public SpannedCell {
    public:
	ImageCell( KrossWord* krossWord, const Coord& coordTopLeft,
		   int horizontalCellSpan, int verticalCellSpan, const KUrl &url/*,
		   QGraphicsScene* scene*/ );

	/** For qgraphicsitem_cast. */
	enum { Type = UserType + 8 };
	virtual int type() const { return Type; };

	virtual void drawBackgroundForPrinting(
		QPainter *p, const QStyleOptionGraphicsItem *options );

	KUrl url() const { return m_url; };

    private:
	KUrl m_url;
	QImage m_image;
};

#endif // IMAGECELL_H
