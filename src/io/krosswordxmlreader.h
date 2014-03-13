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
#include <KUrl>

namespace Crossword
{
class KrossWord;
}
using namespace Crossword;

class KrossWordXmlReader : public QXmlStreamReader
{
public:
    KrossWordXmlReader();

    struct KrossWordInfo {
        int width, height;
        QString type, title, authors, copyright, notes;

        KrossWordInfo() {
            this->width = this->height = -1; // make invalid initially
        };

        KrossWordInfo( const KrossWordInfo &other );

        KrossWordInfo( const QString &type, int width, qint8 height,
                       const QString &title, const QString &authors,
                       const QString &copyright, const QString &notes );

        bool isValid() const {
            return this->width > 0 && this->height > 0;
        };
    };

    /** Reads information about the crossword at the given @p url.
    * @returns A KrossWordInfo object with information about the crossword at
    * the given @p url. Use KrossWordInfo::isValid() to check for errors.
    * If isValid() returns false @p errorString will be set to a string
    * explaining the error (if @p errorString isn't NULL). */
    static KrossWordInfo readInfo( const KUrl &url, QString *errorString = NULL );

    bool readCompressed( QIODevice *device, KrossWord *krossWord,
                         QByteArray *undoData = NULL );
    bool readCompressedInfo( QIODevice *device, KrossWordInfo &krossWordInfo );

    bool read( QIODevice *device, KrossWord *krossWord,
               QByteArray *undoData = NULL );
    bool readInfo( QIODevice *device, KrossWordInfo &krossWordInfo );

private:
    void readUnknownElement();
    KrossWordInfo readKrossWordInfo();
    void readKrossWord( KrossWord *krossWord, QByteArray *undoData = NULL );
    void readClue( KrossWord *krossWord );
    void readImage( KrossWord *krossWord );
    void readSolutionLetter( KrossWord *krossWord );
    void readUserDefinedCrosswordSettings( KrossWord *krossWord );
};

#endif // Multiple inclusion guard
