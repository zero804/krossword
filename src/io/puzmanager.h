/*
*   Copyright 2010 Friedrich Pülz <fpuelz@gmx.de>
*   Copyright 2017 Giacomo Barazzetti <giacomosrv@gmail.com>
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

#ifndef PUZMANAGER_HEADER
#define PUZMANAGER_HEADER

#include "crosswordio.h"

#include <QDebug>

QString getStringFromGrid(QList<QByteArray> &grid, int x, int y, Qt::Orientation orientation);

class QDataStream;

// TODO: Correct checksums generation, there's some error with zero-endings of strings...
/** This class can read (and write) AcrossLite's PUZ crossword puzzle files. */
class PuzManager : public CrosswordIO //CHECK: KrossWordPuzStream
{
public:
    PuzManager(QIODevice *device);

    bool read(CrosswordData &crossData) override;
    bool write(const CrosswordData &crossdata) override;

private:
    static const char *FILE_MAGIC;
    static const qint64 OFFSET_FILE_MAGIC = 0x2;
    static const qint64 OFFSET_VERSION = 0x18;
    static const qint64 OFFSET_GRID_SIZE = 0x2c;
    static const qint64 OFFSET_SOLUTION = 0x34;

    // as internal PUZ-oriented representation
    struct PuzData {
        qint8 width, height;
        QByteArray title, authors, copyright, notes;
        QList<QByteArray> clues;
        QByteArray solution, state; // left->right and up->down: letters, '.' for black cells and '-' for missing letters in state(user answers)
    };

    PuzData m_puzData;

    struct PuzChecksums {
        quint16 main, cib/*, solution, state, part*/;
        QList< qint8 > masked;
    };

    bool readData(QIODevice *device, PuzChecksums *checksums);

    /** In .puz-files clues are not mapped to specific cells directly. Instead
    * the cells must be derived from the shape of the crossword. That is what
    * this method does.
    * It stores lists of ClueInfo's in @p acrossClues and @p downClues.
    * @returns False on error. */
    bool mapClues(QList<ClueInfo> &clues);

    /** Reads chars until a '\0' is read. @return All chars that were read. */
    QByteArray readZeroTerminatedString(QDataStream &dataStream);

    PuzChecksums generateChecksums(QIODevice* buffer) const;
    quint16 checkSumRegion(const char *base, int len, quint16 checkSum) const;

    bool writeDataTo(QDataStream &dataStream, const QByteArray &data, int len) const;

    /** Convert CrossData.acrossClues and CrossData.downClue in
     * PuzData.clues, PuzData.solution and PuzData.state */
    bool prepareDataForWrite(const CrosswordData &crossData);

    bool cellNeedsAcrossNumber(qint8 x, qint8 y) const;
    bool cellNeedsDownNumber(qint8 x, qint8 y) const;
};

#endif
