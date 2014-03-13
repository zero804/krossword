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

#include "crosswordthumbnail.h"
#include "krossword.h"

#include <KUrl>

bool CrosswordThumbCreator::create( const QString& path, int width, int height, QImage& img )
{
    KrossWord krossWord;
    QString errorString;
    if ( !krossWord.read( KUrl( path ), &errorString ) ) {
        kDebug() << errorString;
        return false;
    }

    img = krossWord.toImage( QSize( width * 2, height * 2 ) );

    return true;
}

extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator() {
        return new CrosswordThumbCreator();
    }
};
