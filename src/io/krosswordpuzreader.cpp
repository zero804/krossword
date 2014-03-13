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

#include "krosswordpuzreader.h"

#include "krossword.h"
#include "animator.h"
#include "cells/cluecell.h"
#include "cells/lettercell.h"

#include <QIODevice>
#include <QStringList>
#include <qtextcodec.h>
#include <qbuffer.h>

#include <KDebug>

const char *KrossWordPuzStream::FILE_MAGIC = "ACROSS&DOWN";

KrossWordPuzStream::KrossWordPuzStream()
{

}


bool KrossWordPuzStream::read(QIODevice *device,
                              KrossWordPuzStream::KrossWordData *krossWordData,
                              KrossWordPuzStream::PuzChecksums *checksums)
{
    Q_ASSERT(device);
    Q_ASSERT(krossWordData);

    bool closeAfterRead;
    if ((closeAfterRead = !device->isOpen()) && !device->open(QIODevice::ReadOnly))
        return false;
    setDevice(device);
    setByteOrder(LittleEndian);

    int gridStringLength;
    qint16 clueNumber;
    QByteArray fileMagic;

    if (checksums)
        *this >> checksums->main;
    else if (!device->seek(OFFSET_FILE_MAGIC)) {
        if (closeAfterRead) device->close();
        return false;
    }

    // Read and check file magic
    fileMagic.reserve(12);
    if (readRawData(fileMagic.data(), 12) < 12) {
        if (closeAfterRead) device->close();
        return false;
    }
    if (QString(fileMagic) == (FILE_MAGIC)) {
        kDebug() << QString("Wrong file magic '%1', should be '%2'").arg(fileMagic.data()).arg(FILE_MAGIC);
        if (closeAfterRead) device->close();
        return false;
    }

    if (checksums) {
        *this >> checksums->cib;

        qint8 masked;
        for (int i = 0; i < 8; ++i) {
            *this >> masked;
            checksums->masked << masked;
        }
    } else if (!device->seek(OFFSET_VERSION)) {
        if (closeAfterRead) device->close();
        return false;
    }

    // Read and check version
    char *version = new char[4];
    if (readRawData(version, 4) < 4) {
        delete[] version;
        if (closeAfterRead) device->close();
        return false;
    }
    if (!QString(version).startsWith(QLatin1String("1.2"))) {
        kDebug() << "Unsupported PUZ-version:" << version << "Should be 1.2";
//  delete[] version;
//  if ( closeAfterRead ) device->close();
//  return false;
    }
    delete[] version;

    // Read crossword grid size
    if (!device->seek(OFFSET_GRID_SIZE)) {
        if (closeAfterRead) device->close();
        return false;
    }
    *this >> krossWordData->width;
    *this >> krossWordData->height;

    // Read number of clues
    *this >> clueNumber;

    // Read puzzle solution string
    if (!device->seek(OFFSET_SOLUTION)) {
        if (closeAfterRead) device->close();
        return false;
    }
    gridStringLength = krossWordData->width * krossWordData->height;
    char *solution = new char[gridStringLength];
    if (readRawData(solution, gridStringLength) != gridStringLength) {
        delete[] solution;
        if (closeAfterRead) device->close();
        return false;
    }
    krossWordData->solution = solution;
    delete[] solution;

    // Read puzzle state string
    char *state = new char[gridStringLength];
    if (readRawData(state, gridStringLength) != gridStringLength) {
        delete[]  state;
        if (closeAfterRead) device->close();
        return false;
    }
    krossWordData->state = state;
    delete[] state;

    // Read header information
    krossWordData->title = readZeroTerminatedString().trimmed();
    krossWordData->authors = readZeroTerminatedString().trimmed();
    krossWordData->copyright = readZeroTerminatedString().trimmed();

    int pos;
    if (krossWordData->title.isEmpty() &&
            (pos = QString(krossWordData->authors).indexOf(QRegExp("by", Qt::CaseInsensitive))) != -1) {
        krossWordData->title = krossWordData->authors.left(pos).trimmed();
        krossWordData->authors = krossWordData->authors.mid(pos).trimmed();

        kDebug() << "Extracted title from author field:" << krossWordData->title
                 << "author is now" << krossWordData->authors;
    }

    // Read clues
    for (qint16 i = 0; i < clueNumber; ++i)
        krossWordData->clues << readZeroTerminatedString();

    // Read notes
    krossWordData->notes = readZeroTerminatedString();

    if (closeAfterRead)
        device->close();

    return true;
}

bool KrossWordPuzStream::read(QIODevice* device, KrossWord* krossWord)
{
    Q_ASSERT(krossWord);

    // Read from device
    PuzChecksums checksums;
    KrossWordData krossWordData;
    if (!read(device, &krossWordData, &checksums))
        return false;

    PuzChecksums generatedChecksums = generateChecksums(device, krossWordData);
    kDebug() << "main" << checksums.main << "=?=" << generatedChecksums.main;
    kDebug() << "cib" << checksums.cib << "=?=" << generatedChecksums.cib;
    for (int i = 0; i < 8; ++i)
        kDebug() << "masked" << i << checksums.masked[i] << "=?=" << generatedChecksums.masked[i];

    QList<ClueInfo> acrossClues, downClues;
    bool mappingCluesOk = mapClues(krossWordData, acrossClues, downClues);

    krossWord->setTitle(krossWordData.title);
    krossWord->setAuthors(krossWordData.authors);
    krossWord->setCopyright(krossWordData.copyright);
    krossWord->setNotes(krossWordData.notes);

    krossWord->removeAllCells();
    // Crosswords in *.puz-files are always american style
    krossWord->createNew(American, QSize(krossWordData.width, krossWordData.height));

    // Don't emit currentAnswerChanged signals
    bool wasBlockingSignals = krossWord->blockSignals(true);
#if QT_VERSION >= 0x040600
    krossWord->animator()->setEnabled(false);
#endif

    ClueCell *clue;
    foreach(ClueInfo clueInfo, acrossClues) {
        QPair<uint, uint> coords = indexToCoords(clueInfo.gridIndex, krossWordData.width);
        QString answer, currentAnswer;
        for (int x = coords.first; x < krossWordData.width; ++x) {
            uint index = coordsToIndex(x, coords.second, krossWordData.width);
            char ch = krossWordData.solution[ index ];
            if (ch == '.')
                break;
            else {
                answer += ch;

                char chState = krossWordData.state[ index ];
                if (chState == '.') {
                    kDebug() << "The solution and state strings in the PUZ-file aren't compatible.";
                    kDebug() << "The solution string has no empty cell at"
                             << QString("(%1, %2)").arg(x).arg(coords.second) << "but the state string has";
                    currentAnswer += ' ';
                } else if (chState == '-') {
                    currentAnswer += ' ';
//   } else if ( !LetterCell::isLetterAllowed(chState) ) {/*
                } else if (!krossWord->crosswordTypeInfo().isCharacterLegal(chState)) {
                    kDebug() << "The state string contains a not allowed letter" << chState;
                    currentAnswer += ' ';
                } else
                    currentAnswer += chState;
            }
        }

        // Put question cells to empty cells if possible (TODO: half question cells? parted diagonally..)
//  QPair<uint, uint> coordsLeft( coords.first - 1, coords.second );
//  bool isPlaceForQuestionCell = /*coords.second == 0 ||*/
//   coords.first != 0 && puzzleSolution[ coordsToIndex(coordsLeft.first, coordsLeft.second, width) ] == '.';

        krossWord->insertClue(Coord(coords.first, coords.second), Qt::Horizontal,
                              OnClueCell, clueInfo.clue, answer,
                              LetterCellType, DontIgnoreErrors, true,
                              &clue);

        if (clue) {
            clue->setClueNumber(clueInfo.number);
            clue->setCurrentAnswer(currentAnswer, Unknown);
        }
    }

    foreach(ClueInfo clueInfo, downClues) {
        QPair<uint, uint> coords = indexToCoords(clueInfo.gridIndex, krossWordData.width);
        QString answer, currentAnswer;
        for (int y = coords.second; y < krossWordData.height; ++y) {
            uint index = coordsToIndex(coords.first, y, krossWordData.width);
            char ch = krossWordData.solution[ index ];
            if (ch == '.')
                break;
            else
                answer += ch;

            char chState = krossWordData.state[ index ];
            if (chState == '.') {
                kDebug() << "The solution and state strings in the PUZ-file aren't compatible.";
                kDebug() << "The solution string has no empty cell at"
                         << QString("(%1, %2)").arg(coords.first).arg(y) << "but the state string has";
                currentAnswer += ' ';
            } else if (chState == '-') {
                currentAnswer += ' ';
//   } else if ( !LetterCell::isLetterAllowed(chState) ) {
            } else if (!krossWord->crosswordTypeInfo().isCharacterLegal(chState)) {
                kDebug() << "The state string contains a not allowed letter" << chState;
                currentAnswer += ' ';
            } else
                currentAnswer += chState;
        }

        krossWord->insertClue(Coord(coords.first, coords.second), Qt::Vertical,
                              OnClueCell, clueInfo.clue/*curClueIndex*/, answer,
                              LetterCellType, DontIgnoreErrors, true,
                              &clue);
        if (clue) {
            clue->setClueNumber(clueInfo.number);
            clue->setCurrentAnswer(currentAnswer, Unknown);
        }
    }

#if QT_VERSION >= 0x040600
    krossWord->animator()->setEnabled(true);
#endif
    krossWord->blockSignals(wasBlockingSignals);

    return mappingCluesOk;
}

KrossWordPuzStream::KrossWordData::KrossWordData(KrossWord* krossWord,
        KrossWord::WriteMode writeMode)
{
    Q_ASSERT(krossWord);

    width = krossWord->width();
    height = krossWord->height();
    title = krossWord->title().toLatin1();
    authors = krossWord->authors().toLatin1();
    copyright = krossWord->copyright().toLatin1();
    notes = krossWord->notes().toLatin1();

    // Get crossword solution and state strings
    int gridStringLength = width * height;
    solution.reserve(gridStringLength);
    state.reserve(gridStringLength);
    for (uint y = 0; y < krossWord->height(); ++y) {
        for (uint x = 0; x < krossWord->width(); ++x) {
            KrossWordCell *cell = krossWord->at(Coord(x, y));
            if (cell->isLetterCell()) {
                LetterCell *letter = (LetterCell*)cell;

                if (writeMode == KrossWord::Template)
                    solution.append('-');
                else
                    solution.append(letter->correctLetter().toLatin1());

                if (letter->isEmpty() || writeMode == KrossWord::Template)
                    state.append('-');
                else
                    state.append(letter->currentLetter().toLatin1());

                ClueCell *clueHorz, *clueVert;
                clueHorz = letter->clue(Qt::Horizontal);
                clueVert = letter->clue(Qt::Vertical);

                if (writeMode == KrossWord::Template) {
                    if (clueHorz && clueHorz->firstLetter() == letter)
                        clues << "";
                    if (clueVert && clueVert->firstLetter() == letter)
                        clues << "";
                } else {
                    if (clueHorz && clueHorz->firstLetter() == letter)
                        clues << clueHorz->clue().toLatin1();
                    if (clueVert && clueVert->firstLetter() == letter)
                        clues << clueVert->clue().toLatin1();
                }
            } else { // No clue or solution letter cells in PUZ format
                solution.append('.');
                state.append('.');
            }
        }
    }
}

bool KrossWordPuzStream::writeDataTo(QDataStream &ds,
                                     const QByteArray &data, int len) const
{
    return ds.writeRawData(data, len) == len;
}

bool KrossWordPuzStream::write(QIODevice* device, KrossWord* krossWord,
                               KrossWord::WriteMode writeMode)
{
    Q_ASSERT(device);
    Q_ASSERT(krossWord);

    if (krossWord->width() > 255 || krossWord->height() > 255) {
        kDebug() << "Maximal size of crosswords to be saved in the PUZ-format "
                 "version 1.2/1.3 is 255x255.";
        return false;
    }

    bool closeAfterWrite;
    if ((closeAfterWrite = !device->isOpen()) && !device->open(QIODevice::WriteOnly))
        return false;
    setDevice(device);
    setByteOrder(LittleEndian);

    KrossWordData krossWordData(krossWord, writeMode);
    QBuffer data;
    data.open(QBuffer::ReadWrite);
    QDataStream ds(&data);
    ds.setByteOrder(LittleEndian);

    if (!data.seek(OFFSET_FILE_MAGIC)) {     // Skip bytes: Main checksum
        if (closeAfterWrite) device->close();
        return false;
    }
    if (ds.writeRawData("ACROSS&DOWN\0", 12) != 12) {     // Magic string
        if (closeAfterWrite) device->close();
        return false;
    }

    if (!data.seek(OFFSET_VERSION)) {     // Skip bytes: CIB checksum, masked checksums
        if (closeAfterWrite) device->close();
        return false;
    }
    if (ds.writeRawData("1.2\0", 4) != 4) {     // Version
        if (closeAfterWrite) device->close();
        return false;
    }

    if (!data.seek(OFFSET_GRID_SIZE)) {     // Skip bytes: Masked checksums
        if (closeAfterWrite) device->close();
        return false;
    }
    ds << (qint8)krossWordData.width;
    ds << (qint8)krossWordData.height;
    ds << (qint16)krossWordData.clues.count();
    ds << (qint8)1;
    ds << (qint8)0;
    ds << (qint8)0;
    ds << (qint8)0;

    if (!data.seek(OFFSET_SOLUTION)) {
        if (closeAfterWrite) device->close();
        return false;
    }
    int gridStringLength = krossWord->width() * krossWord->height();
    if (ds.writeRawData(krossWordData.solution, gridStringLength) != gridStringLength) {
        if (closeAfterWrite) device->close();
        return false;
    }
    if (ds.writeRawData(krossWordData.state, gridStringLength) != gridStringLength) {
        if (closeAfterWrite) device->close();
        return false;
    }

    if (!writeDataTo(ds, krossWordData.title, krossWordData.title.length() + 1)) {
        if (closeAfterWrite) device->close();
        return false;
    }
    if (!writeDataTo(ds, krossWordData.authors, krossWordData.authors.length() + 1)) {
        if (closeAfterWrite) device->close();
        return false;
    }
    if (!writeDataTo(ds, krossWordData.copyright, krossWordData.copyright.length() + 1)) {
        if (closeAfterWrite) device->close();
        return false;
    }

    for (qint16 i = 0; i < krossWordData.clues.count(); ++i) {
        if (!writeDataTo(ds, krossWordData.clues[i], krossWordData.clues[i].length() + 1)) {
            if (closeAfterWrite) device->close();
            return false;
        }
    }

    if (!writeDataTo(ds, krossWordData.notes, krossWordData.notes.length() + 1)) {
        if (closeAfterWrite) device->close();
        return false;
    }

    data.close();

    // Get checksums
    PuzChecksums puzChecksums = generateChecksums(&data, krossWordData);

    // Write checksums to buffer
    data.open(QIODevice::ReadWrite);
    ds << puzChecksums.main;
    if (!data.seek(0xe)) {     // Skip magic string
        if (closeAfterWrite) device->close();
        return false;
    }
    ds << puzChecksums.cib;
    foreach(const qint8 & checksumMasked, puzChecksums.masked)
    ds << checksumMasked;
    data.close();

    // Write everything buffered
    if (!writeRawData(data.buffer(), data.size())) {
        if (closeAfterWrite) device->close();
        return false;
    }

    if (closeAfterWrite)
        device->close();
    return true;
}

KrossWordPuzStream::PuzChecksums KrossWordPuzStream::generateChecksums(
    QIODevice* buffer, KrossWordData data) const
{
    PuzChecksums checksums;

    buffer->open(QIODevice::ReadOnly);
    buffer->seek(0x2c);
    checksums.cib = checkSumRegion(buffer->read(8), 8, 0);
    buffer->close();

    // TODO: checksum AND checksumPart are wrong... (something to do with \0-termination?)
    int gridStringLength = data.width * data.height;
    checksums.main = checkSumRegion(data.solution, gridStringLength, checksums.cib);
    checksums.main = checkSumRegion(data.state, gridStringLength, checksums.main);
    checksums.main = checkSumRegion(data.title, data.title.length() + 1, checksums.main);   // zero-terminated
    checksums.main = checkSumRegion(data.authors, data.authors.length() + 1, checksums.main);   // zero-terminated
    checksums.main = checkSumRegion(data.copyright, data.copyright.length() + 1, checksums.main);   // zero-terminated
    quint16 solution = checkSumRegion(data.solution, gridStringLength, 0);
    quint16 state = checkSumRegion(data.state, gridStringLength, 0);
    quint16 part = checkSumRegion(data.title, data.title.length() + 1, 0);   // zero-terminated
    part = checkSumRegion(data.authors, data.authors.length() + 1, part);   // zero-terminated
    part = checkSumRegion(data.copyright, data.copyright.length() + 1, part);   // zero-terminated
    QByteArray test;
    for (int i = 0; i < data.clues.count(); i++) {
        QByteArray clue = data.clues[i];
        checksums.main = checkSumRegion(clue, clue.length(), checksums.main);   // NOT zero-terminated
        part = checkSumRegion(clue, clue.length(), part);   // NOT zero-terminated
    }

    // These hex values in ASCII are the string "ICHEATED",
    // they are XOR-masked against the checksums
    checksums.masked.append((qint8)(0x49 ^ (checksums.cib & 0xFF)));
    checksums.masked.append((qint8)(0x43 ^ (solution & 0xFF)));
    checksums.masked.append((qint8)(0x48 ^ (state & 0xFF)));
    checksums.masked.append((qint8)(0x45 ^ (part & 0xFF)));
    checksums.masked.append((qint8)(0x41 ^ ((checksums.cib & 0xFF00) >> 8)));
    checksums.masked.append((qint8)(0x54 ^ ((solution & 0xFF00) >> 8)));
    checksums.masked.append((qint8)(0x45 ^ ((state & 0xFF00) >> 8)));
    checksums.masked.append((qint8)(0x44 ^ ((part & 0xFF00) >> 8)));

    return checksums;
}

quint16 KrossWordPuzStream::checkSumRegion(const char *base, int len, quint16 checkSum) const
{
    for (int i = 0; i < len; i++) {
        if (checkSum & 0x0001)
            checkSum = (checkSum >> 1) + 0x8000;
        else
            checkSum = checkSum >> 1;
        checkSum += *(base + i);
    }

    return checkSum;
}

QPair< uint, uint > KrossWordPuzStream::indexToCoords(uint index, uint width) const
{
    return QPair< uint, uint >(index % width, index / width);
}

uint KrossWordPuzStream::coordsToIndex(uint x, uint y, uint width) const
{
    return x + y * width;
}

bool KrossWordPuzStream::mapClues(const KrossWordData &krossWordData,
                                  QList<ClueInfo> &acrossClues, QList<ClueInfo> &downClues) const
{
    uint curClueNumber = 0;
    uint curClueIndex = 0;

    //  Iterate through all cells
    for (qint8 y = 0; y < krossWordData.height; ++y) {
        for (qint8 x = 0; x < krossWordData.width; ++x) {
            if (krossWordData.solution[coordsToIndex(x, y, krossWordData.width)] == '.')
                continue;

            bool assignedNumber = false;
            if (cellNeedsAcrossNumber(x, y, krossWordData.width, krossWordData.solution)) {
                if (curClueIndex >= (uint)krossWordData.clues.count()) {
                    kDebug() << "Error in reading PUZ from device" << device();
                    kDebug() << "Too few clues:" << krossWordData.clues.count()
                             << "Tried index" << curClueIndex;
                    return false;
                }
                acrossClues.append(ClueInfo(coordsToIndex(x, y, krossWordData.width),
                                            curClueNumber, krossWordData.clues[curClueIndex++]));
                assignedNumber = true;
            }
            if (cellNeedsDownNumber(x, y, krossWordData.width, krossWordData.solution)) {
                if (curClueIndex >= (uint)krossWordData.clues.count()) {
                    kDebug() << "Error in reading PUZ from device" << device();
                    kDebug() << "Too few clues:" << krossWordData.clues.count()
                             << "Tried index" << curClueIndex;
                    return false;
                }
                downClues.append(ClueInfo(coordsToIndex(x, y, krossWordData.width),
                                          curClueNumber, krossWordData.clues[curClueIndex++]));
                assignedNumber = true;
            }

            if (assignedNumber)
                ++curClueNumber;
        } // for x
    } // for y

    return true;
}

//  Returns true if the cell at (x, y) gets an "across" clue number.
bool KrossWordPuzStream::cellNeedsAcrossNumber(qint8 x, qint8 y, qint8 width,
        const QByteArray &puzzleSolution) const
{
    // Check that there is a blank to the left of us
    if (x == 0 || puzzleSolution[coordsToIndex(x - 1, y, width)] == '.') {
        // Check that there is space (at least two cells) for a word here
        if (x + 1 < width && puzzleSolution[coordsToIndex(x + 1, y, width)] != '.')
            return true;
    }
    return false;
}

//  Returns true if the cell at (x, y) gets an "down" clue number.
bool KrossWordPuzStream::cellNeedsDownNumber(qint8 x, qint8 y, qint8 width,
        const QByteArray &puzzleSolution) const
{
    // Check that there is a blank to the left of us
    if (y == 0 || puzzleSolution[coordsToIndex(x, y - 1, width)] == '.') {
        // Check that there is space (at least two cells) for a word here
        if (y + 1 < width && puzzleSolution[coordsToIndex(x, y + 1, width)] != '.')
            return true;
    }
    return false;
}

QByteArray KrossWordPuzStream::readZeroTerminatedString()
{
    QByteArray string;
    char *buffer = new char[1];

    while (readRawData(buffer, 1) == 1 && buffer[0] != '\0') {
//  kDebug() << buffer[0];
        string.append(buffer[0]);
    }

    delete buffer;
    return string;
}






