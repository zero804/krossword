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

#ifndef KROSSWORDPUZREADER_HEADER
#define KROSSWORDPUZREADER_HEADER

#include <QDataStream>
#include <QPair>

class QBuffer;
class QString;
class KrossWord;
class QIODevice;

// TODO: Correct checksums generation, there's some error with zero-endings of strings...
/** This class can read and write AcrossLite's PUZ crossword puzzle files. */
class KrossWordPuzStream : public QDataStream
{
public:
    KrossWordPuzStream();

    static const char *FILE_MAGIC;
    static const qint64 OFFSET_FILE_MAGIC = 0x2;
    static const qint64 OFFSET_VERSION = 0x18;
    static const qint64 OFFSET_GRID_SIZE = 0x2c;
    static const qint64 OFFSET_SOLUTION = 0x34;

    struct KrossWordData {
        qint8 width, height;
        QByteArray solution, state;
        QByteArray title, authors, copyright, notes;
        QList< QByteArray > clues;

        KrossWordData() {
        };

        KrossWordData(qint8 width, qint8 height,
                      const QByteArray &solution, const QByteArray &state,
                      const QByteArray &title, const QByteArray &authors,
                      const QByteArray &copyright, const QByteArray &notes,
                      const QList< QByteArray > &clues) {
            this->width = width;
            this->height = height;
            this->solution = solution;
            this->state = state;
            this->title = title;
            this->authors = authors;
            this->copyright = copyright;
            this->notes = notes;
            this->clues = clues;
        };

        KrossWordData(KrossWord *krossWord);
    };

    struct PuzChecksums {
        quint16 main, cib/*, solution, state, part*/;
        QList< qint8 > masked;

        PuzChecksums(quint16 main, quint16 cib, /*quint16 solution, quint16 state,
        quint16 part,*/ const QList< qint8 > &masked) {
            this->main = main;
            this->cib = cib;
//      this->solution = solution;
//      this->state = state;
//      this->part = part;
            this->masked = masked;
        };

        PuzChecksums() {
        };
    };

    bool read(QIODevice *device, KrossWordData *krossWordData,
              PuzChecksums *checksums);
    bool read(QIODevice *device, KrossWord *krossWord);
    bool write(QIODevice *device, KrossWord *krossWord);

private:
    struct ClueInfo {
        uint gridIndex;
        uint number;
        QString clue;

        ClueInfo(uint gridIndex, uint number, QString clue) {
            this->gridIndex = gridIndex;
            this->number = number;
            this->clue = clue;
        };
    };

    PuzChecksums generateChecksums(QIODevice* buffer, KrossWordData data) const;
    bool writeDataTo(QDataStream &ds, const QByteArray &data, int len) const;

    quint16 checkSumRegion(const char *base, int len, quint16 checkSum) const;

    /** In .puz-files clues are not mapped to specific cells directly. Instead
    * the cells must be derived from the shape of the crossword. That is what
    * this method does.
    * It stores lists of ClueInfo's in @p acrossClues and @p downClues.
    * @param krossWordData A crossword data object.
    * @param acrossClues A list of ClueInfo's, one for each across (horizontal)
    * clue.
    * @param downClues A list of ClueInfo's, one for each down (vertical) clue.
    * @returns False on error. */
    bool mapClues(const KrossWordData &krossWordData,
                  QList<ClueInfo> &acrossClues, QList<ClueInfo> &downClues) const;
    QPair<uint, uint> indexToCoords(uint index, uint width) const;
    uint coordsToIndex(uint x, uint y, uint width) const;
    bool cellNeedsAcrossNumber(qint8 x, qint8 y, qint8 width,
                               const QByteArray &puzzleSolution) const;
    bool cellNeedsDownNumber(qint8 x, qint8 y, qint8 width,
                             const QByteArray &puzzleSolution) const;
    /** Reads chars until a '\0' is read.
    * @return All chars that were read. */
    QByteArray readZeroTerminatedString();
};

#endif // Multiple inclusion guard
