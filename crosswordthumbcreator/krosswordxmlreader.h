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

#ifndef KROSSWORDXMLREADER_HEADER
#define KROSSWORDXMLREADER_HEADER

#include <QXmlStreamReader>

class KrossWord;

class KrossWordXmlReader : public QXmlStreamReader {
public:
    KrossWordXmlReader();

    struct KrossWordInfo {
	int width, height;
	QString title, authors, copyright, notes;

	KrossWordInfo() {
	};

	KrossWordInfo( const KrossWordInfo &other ) {
	    this->width = other.width;
	    this->height = other.height;
	    this->title = other.title;
	    this->authors = other.authors;
	    this->copyright = other.copyright;
	    this->notes = other.notes;
	};

	KrossWordInfo( int width, qint8 height,
		const QString &title, const QString &authors,
		const QString &copyright, const QString &notes ) {
	    this->width = width;
	    this->height = height;
	    this->title = title;
	    this->authors = authors;
	    this->copyright = copyright;
	    this->notes = notes;
	};
    };

    bool readCompressed( QIODevice *device, KrossWord *krossWord );
    bool readCompressedInfo( QIODevice *device, KrossWordInfo &krossWordInfo );
    bool read( QIODevice *device, KrossWord *krossWord );
    bool readInfo( QIODevice *device, KrossWordInfo &krossWordInfo );

private:
    void readUnknownElement();
    KrossWordInfo readKrossWordInfo();
    void readKrossWord( KrossWord *krossWord );
    void readClue( KrossWord *krossWord );
    void readImage( KrossWord *krossWord );
    void readSolutionLetter( KrossWord *krossWord );
};

#endif // Multiple inclusion guard
