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

#ifndef KROSSWORDXMLWRITER_HEADER
#define KROSSWORDXMLWRITER_HEADER

#include <QXmlStreamWriter>
#include <krossword.h>

namespace Crossword
{
class ImageCell;
class SolutionLetterCell;
class ClueCell;
class KrossWord;
}
using namespace Crossword;

class KrossWordXmlWriter : public QXmlStreamWriter
{
public:
    KrossWordXmlWriter() { };

    bool writeCompressed( QIODevice *device, KrossWord *krossWord,
                          KrossWord::WriteMode writeMode = KrossWord::Normal,
                          const QByteArray &undoData = QByteArray() );
    bool write( QIODevice *device, KrossWord *krossWord,
                KrossWord::WriteMode writeMode = KrossWord::Normal,
                const QByteArray &undoData = QByteArray() );

    QString errorString() const {
        return m_errorString;
    };

private:
    void writeKrossWord( KrossWord *krossWord,
                         KrossWord::WriteMode writeMode = KrossWord::Normal,
                         const QByteArray &undoData = QByteArray() );
    void writeClue( ClueCell *clue,
                    KrossWord::WriteMode writeMode = KrossWord::Normal );
    void writeImage( ImageCell *image,
                     KrossWord::WriteMode writeMode = KrossWord::Normal );
    void writeSolutionLetter( SolutionLetterCell *solutionLetter );

    QString m_errorString;
};

#endif // Multiple inclusion guard
