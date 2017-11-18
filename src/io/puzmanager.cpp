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

#include "puzmanager.h"

#include <QIODevice>
#include <QDataStream>
#include <QBuffer>

#include <QDebug>

const char *PuzManager::FILE_MAGIC = "ACROSS&DOWN";

QString getStringFromGrid(QList<QByteArray> &grid, int x, int y, Qt::Orientation orientation)
{
    QString result;
    char letter = grid.at(y).at(x);
    if (orientation == Qt::Horizontal) {
        int pos = x;
        while (pos < grid.at(y).length()) {
            letter = grid.at(y).at(pos);
            if (letter == '.') {
                break;
            }
            result.append(letter);
            pos++;
        }
    } else if (orientation == Qt::Vertical) {
        int pos = y;
        while (pos < grid.length()) {
            letter = grid.at(pos).at(x);
            if (letter == '.') {
                break;
            }
            result.append(letter);
            pos++;
        }
    }

    return result;
}

//----------------------------------------------

bool PuzManager::readData(QIODevice *device, PuzManager::PuzChecksums *checksums)
{
    Q_ASSERT(device);

    bool closeAfterRead;
    if ((closeAfterRead = !device->isOpen()) && !device->open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream dataStream;
    dataStream.setDevice(device);
    dataStream.setByteOrder(QDataStream::LittleEndian);

    int gridStringLength;
    qint16 clueNumber;
    QByteArray fileMagic;

    if (checksums) {
        dataStream >> checksums->main;
    } else if (!device->seek(OFFSET_FILE_MAGIC)) {
        if (closeAfterRead) {
            device->close();
        }
        return false;
    }

    // Read and check file magic
    fileMagic.reserve(12);
    if (dataStream.readRawData(fileMagic.data(), 12) < 12) {
        if (closeAfterRead) {
            device->close();
        }
        return false;
    }
    if (QString(fileMagic) == (FILE_MAGIC)) {
        qDebug() << QString("Wrong file magic '%1', should be '%2'").arg(fileMagic.data()).arg(FILE_MAGIC);
        if (closeAfterRead) {
            device->close();
        }
        return false;
    }

    if (checksums) {
        dataStream >> checksums->cib;

        qint8 masked;
        for (int i = 0; i < 8; ++i) {
            dataStream >> masked;
            checksums->masked << masked;
        }
    } else if (!device->seek(OFFSET_VERSION)) {
        if (closeAfterRead) {
            device->close();
        }
        return false;
    }

    // Read and check version
    char *version = new char[4];
    if (dataStream.readRawData(version, 4) < 4) {
        delete[] version;
        if (closeAfterRead) {
            device->close();
        }
        return false;
    }
    if (!QString(version).startsWith(QLatin1String("1.2"))) {
        qDebug() << "Unsupported PUZ-version:" << version << "Should be 1.2";
    }
    delete[] version;

    // Read crossword grid size
    if (!device->seek(OFFSET_GRID_SIZE)) {
        if (closeAfterRead) {
            device->close();
        }
        return false;
    }
    dataStream >> m_puzData.width;
    dataStream >> m_puzData.height;

    // Read number of clues
    dataStream >> clueNumber;

    // Read puzzle solution string
    if (!device->seek(OFFSET_SOLUTION)) {
        if (closeAfterRead) {
            device->close();
        }
        return false;
    }
    gridStringLength = m_puzData.width * m_puzData.height;
    char *solution = new char[gridStringLength];
    if (dataStream.readRawData(solution, gridStringLength) != gridStringLength) {
        delete[] solution;
        if (closeAfterRead) {
            device->close();
        }
        return false;
    }
    m_puzData.solution = solution;
    delete[] solution;

    // Read puzzle state string
    char *state = new char[gridStringLength];
    if (dataStream.readRawData(state, gridStringLength) != gridStringLength) {
        delete[]  state;
        if (closeAfterRead) {
            device->close();
        }
        return false;
    }
    m_puzData.state = state;
    delete[] state;

    // Read header information
    m_puzData.title = readZeroTerminatedString(dataStream).trimmed();
    m_puzData.authors = readZeroTerminatedString(dataStream).trimmed();
    m_puzData.copyright = readZeroTerminatedString(dataStream).trimmed();

    int pos;
    if (m_puzData.title.isEmpty() && (pos = QString(m_puzData.authors).indexOf(QRegExp("by", Qt::CaseInsensitive))) != -1) {
        m_puzData.title = m_puzData.authors.left(pos).trimmed();
        m_puzData.authors = m_puzData.authors.mid(pos).trimmed();

        qDebug() << "Extracted title from author field:" << m_puzData.title
                 << "author is now" << m_puzData.authors;
    }

    // Read clues
    for (qint16 i = 0; i < clueNumber; ++i) {
        m_puzData.clues << readZeroTerminatedString(dataStream);

    }

    // Read notes
    m_puzData.notes = readZeroTerminatedString(dataStream);

    if (closeAfterRead) {
        device->close();
    }

    return true;
}

PuzManager::PuzManager(QIODevice *device)
    : CrosswordIO(device)
{

}

bool PuzManager::read(CrosswordData &crossData)
{
    Q_ASSERT(m_device);
    Q_ASSERT(m_device->isReadable());

    setErrorString(QString());

    PuzChecksums checksums;
    if (!readData(m_device, &checksums)) {
        return false;
    }
    m_device->close();

    PuzChecksums generatedChecksums = generateChecksums(m_device);
    qDebug() << "main" << checksums.main << "=?=" << generatedChecksums.main;
    qDebug() << "cib" << checksums.cib << "=?=" << generatedChecksums.cib;
    for (int i = 0; i < 8; ++i) {
        qDebug() << "masked" << i << checksums.masked[i] << "=?=" << generatedChecksums.masked[i];
    }

    crossData.width = int(m_puzData.width);
    crossData.height = int(m_puzData.height);
    crossData.type = "American"; // PUZ is always American type
    crossData.title = QString::fromLatin1(m_puzData.title);
    crossData.authors = QString::fromLatin1(m_puzData.authors);
    crossData.copyright = QString::fromLatin1(m_puzData.copyright);
    crossData.notes = QString::fromLatin1(m_puzData.notes);
    //crossData.images - no support in PUZ
    //crossData.lettersConfidence - no support in PUZ
    //crossData.markedLetters - no support in PUZ
    //crossData.undoData - no support in PUZ
    return mapClues(crossData.clues);
}

bool PuzManager::writeDataTo(QDataStream &dataStream, const QByteArray &data, int len) const
{
    return dataStream.writeRawData(data, len) == len;
}

bool PuzManager::write(const CrosswordData &crossdata)
{
    Q_ASSERT(m_device);
    Q_ASSERT(m_device->isWritable());

    setErrorString(QString());

    if (crossdata.width > 255 || crossdata.height > 255) {
        qDebug() << "Maximal size of crosswords to be saved in the PUZ-format version 1.2/1.3 is 255x255.";
        return false;
    }

    m_puzData.width = qint8(crossdata.width);
    m_puzData.height = qint8(crossdata.height);
    // crossdata.type ignored, PUZ is always American type
    // CHECK: best to check the input type?
    m_puzData.title = crossdata.title.toLatin1();
    m_puzData.authors = crossdata.authors.toLatin1();
    m_puzData.copyright = crossdata.copyright.toLatin1();
    m_puzData.notes = crossdata.notes.toLatin1();
    m_puzData.solution = QByteArray(crossdata.width * crossdata.height, '.'); // initialize filling with black cells
    m_puzData.state = QByteArray(crossdata.width * crossdata.height, '.'); // initialize filling with black cells
    if (!prepareDataForWrite(crossdata)) {
        return false;
    }

    bool closeAfterWrite;
    if ((closeAfterWrite = !m_device->isOpen()) && !m_device->open(QIODevice::WriteOnly)) {
        return false;
    }

    QDataStream dataStream;
    dataStream.setDevice(m_device);
    dataStream.setByteOrder(QDataStream::LittleEndian);

    QBuffer data;
    data.open(QBuffer::ReadWrite);
    QDataStream ds(&data);
    ds.setByteOrder(QDataStream::LittleEndian);

    if (!data.seek(OFFSET_FILE_MAGIC)) {     // Skip bytes: Main checksum
        if (closeAfterWrite) m_device->close();
        return false;
    }
    if (ds.writeRawData("ACROSS&DOWN\0", 12) != 12) {     // Magic string
        if (closeAfterWrite) m_device->close();
        return false;
    }

    if (!data.seek(OFFSET_VERSION)) {     // Skip bytes: CIB checksum, masked checksums
        if (closeAfterWrite) m_device->close();
        return false;
    }
    if (ds.writeRawData("1.2\0", 4) != 4) {     // Version
        if (closeAfterWrite) m_device->close();
        return false;
    }

    if (!data.seek(OFFSET_GRID_SIZE)) {     // Skip bytes: Masked checksums
        if (closeAfterWrite) m_device->close();
        return false;
    }
    ds << (qint8)m_puzData.width;
    ds << (qint8)m_puzData.height;
    ds << (qint16)m_puzData.clues.count();
    ds << (qint8)1;
    ds << (qint8)0;
    ds << (qint8)0;
    ds << (qint8)0;

    if (!data.seek(OFFSET_SOLUTION)) {
        if (closeAfterWrite) m_device->close();
        return false;
    }
    int gridStringLength = m_puzData.width * m_puzData.height;
    if (ds.writeRawData(m_puzData.solution, gridStringLength) != gridStringLength) {
        if (closeAfterWrite) m_device->close();
        return false;
    }
    if (ds.writeRawData(m_puzData.state, gridStringLength) != gridStringLength) {
        if (closeAfterWrite) m_device->close();
        return false;
    }

    if (!writeDataTo(ds, m_puzData.title, m_puzData.title.length() + 1)) {
        if (closeAfterWrite) m_device->close();
        return false;
    }
    if (!writeDataTo(ds, m_puzData.authors, m_puzData.authors.length() + 1)) {
        if (closeAfterWrite) m_device->close();
        return false;
    }
    if (!writeDataTo(ds, m_puzData.copyright, m_puzData.copyright.length() + 1)) {
        if (closeAfterWrite) m_device->close();
        return false;
    }

    for (qint16 i = 0; i < m_puzData.clues.count(); ++i) {
        if (!writeDataTo(ds, m_puzData.clues[i], m_puzData.clues[i].length() + 1)) {
            if (closeAfterWrite) m_device->close();
            return false;
        }
    }

    if (!writeDataTo(ds, m_puzData.notes, m_puzData.notes.length() + 1)) {
        if (closeAfterWrite) m_device->close();
        return false;
    }

    data.close();

    // Get checksums
    PuzChecksums puzChecksums = generateChecksums(&data);

    // Write checksums to buffer
    data.open(QIODevice::ReadWrite);
    ds << puzChecksums.main;
    if (!data.seek(0xe)) {     // Skip magic string
        if (closeAfterWrite) m_device->close();
        return false;
    }
    ds << puzChecksums.cib;
    foreach(const qint8 & checksumMasked, puzChecksums.masked) {
        ds << checksumMasked;
    }
    data.close();

    // Write everything buffered
    if (!dataStream.writeRawData(data.buffer(), data.size())) {
        if (closeAfterWrite) m_device->close();
        return false;
    }

    if (closeAfterWrite)
        m_device->close();
    return true;
}

PuzManager::PuzChecksums PuzManager::generateChecksums(QIODevice* buffer) const
{
    PuzChecksums checksums;

    buffer->open(QIODevice::ReadOnly);
    buffer->seek(0x2c);
    checksums.cib = checkSumRegion(buffer->read(8), 8, 0);
    buffer->close();

    // TODO: checksum AND checksumPart are wrong... (something to do with \0-termination?)
    int gridStringLength = m_puzData.width * m_puzData.height;
    checksums.main = checkSumRegion(m_puzData.solution, gridStringLength, checksums.cib);
    checksums.main = checkSumRegion(m_puzData.state, gridStringLength, checksums.main);
    checksums.main = checkSumRegion(m_puzData.title, m_puzData.title.length() + 1, checksums.main);   // zero-terminated
    checksums.main = checkSumRegion(m_puzData.authors, m_puzData.authors.length() + 1, checksums.main);   // zero-terminated
    checksums.main = checkSumRegion(m_puzData.copyright, m_puzData.copyright.length() + 1, checksums.main);   // zero-terminated
    quint16 solution = checkSumRegion(m_puzData.solution, gridStringLength, 0);
    quint16 state = checkSumRegion(m_puzData.state, gridStringLength, 0);
    quint16 part = checkSumRegion(m_puzData.title, m_puzData.title.length() + 1, 0);   // zero-terminated
    part = checkSumRegion(m_puzData.authors, m_puzData.authors.length() + 1, part);   // zero-terminated
    part = checkSumRegion(m_puzData.copyright, m_puzData.copyright.length() + 1, part);   // zero-terminated
    QByteArray test;
    for (int i = 0; i < m_puzData.clues.count(); i++) {
        QByteArray clue = m_puzData.clues[i];
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

quint16 PuzManager::checkSumRegion(const char *base, int len, quint16 checkSum) const
{
    for (int i = 0; i < len; i++) {
        if (checkSum & 0x0001) {
            checkSum = (checkSum >> 1) + 0x8000;
        } else {
            checkSum = checkSum >> 1;
        }
        checkSum += *(base + i);
    }

    return checkSum;
}

bool PuzManager::mapClues(QList<ClueInfo> &clues)
{
    QList<QByteArray> solutionGrid;
    for (int row = 0; row < m_puzData.solution.length(); row += m_puzData.width) {
        solutionGrid.append(m_puzData.solution.mid(row, m_puzData.width));
    }

    QList<QByteArray> stateGrid;
    for (int row = 0; row < m_puzData.state.length(); row += m_puzData.width) {
        stateGrid.append(m_puzData.state.mid(row, m_puzData.width));
    }

    // CHECK: assigning clue number can be done in Krossword...
    uint curClueNumber = 0;
    uint curClueIndex = 0;

    //  Iterate through all cells
    for (qint8 y = 0; y < m_puzData.height; ++y) {
        for (qint8 x = 0; x < m_puzData.width; ++x) {
            const uint index = Coords(x, y).toIndex(m_puzData.width);
            if (m_puzData.solution[index] == '.') { // black cell
                continue;
            }

            bool assignedNumber = false;
            if (cellNeedsAcrossNumber(x, y, m_puzData.width, m_puzData.solution)) {
                if (curClueIndex >= (uint)m_puzData.clues.count()) {
                    qDebug() << "Error in reading PUZ";
                    qDebug() << "Too few clues:" << m_puzData.clues.count() << "Tried index" << curClueIndex;
                    return false;
                }
                clues.append(ClueInfo(index,
                                      curClueNumber,
                                      ClueOrientation::Horizontal,
                                      Crossword::AnswerOffset::OnClueCell,
                                      QString::fromLatin1(m_puzData.clues[curClueIndex++]),
                                      getStringFromGrid(solutionGrid, x, y, Qt::Horizontal),
                                      getStringFromGrid(stateGrid, x, y, Qt::Horizontal)));
                assignedNumber = true;
            }

            if (cellNeedsDownNumber(x, y, m_puzData.width, m_puzData.solution)) {
                if (curClueIndex >= (uint)m_puzData.clues.count()) {
                    qDebug() << "Error in reading PUZ";
                    qDebug() << "Too few clues:" << m_puzData.clues.count() << "Tried index" << curClueIndex;
                    return false;
                }
                clues.append(ClueInfo(index,
                                      curClueNumber,
                                      ClueOrientation::Vertical,
                                      Crossword::AnswerOffset::OnClueCell,
                                      QString::fromLatin1(m_puzData.clues[curClueIndex++]),
                                      getStringFromGrid(solutionGrid, x, y, Qt::Vertical),
                                      getStringFromGrid(stateGrid, x, y, Qt::Vertical)));
                assignedNumber = true;
            }

            if (assignedNumber) {
                ++curClueNumber;
            }
        } // for x
    } // for y

    return true;
}

bool PuzManager::prepareDataForWrite(const CrosswordData &crossData)
{
    for (uint i = 0; i < (m_puzData.width * m_puzData.height); i++) {
        foreach (const ClueInfo &clueInfo, crossData.clues) {
            if (clueInfo.gridIndex == i) {
                if (clueInfo.orientation == ClueOrientation::Horizontal) {
                    m_puzData.clues.append(clueInfo.clue.toLatin1());

                    m_puzData.solution.replace(i, clueInfo.solution.length(), clueInfo.solution.toLatin1());
                    m_puzData.state.replace(i, clueInfo.answer.length(), clueInfo.answer.toLatin1());
                } else if (clueInfo.orientation == ClueOrientation::Vertical) {
                    m_puzData.clues.append(clueInfo.clue.toLatin1());

                    // solution and state(answers) filling has to be done with down clues too because it's possible
                    // to have letters not included in an horizontal clue (and vice versa)
                    int verticalIndex = i;
                    for (int pos = 0; pos < clueInfo.solution.length(); pos++) {
                        // solution and answer have the same lenght so we can use the same 'for' cicle
                        m_puzData.solution[verticalIndex] = clueInfo.solution.at(pos).toLatin1();
                        m_puzData.state[verticalIndex] = clueInfo.answer.at(pos).toLatin1();
                        verticalIndex += crossData.width;
                    }
                }
            }
        }
    }

    return true; //CHECK: currently useless
}

//  Returns true if the cell at (x, y) gets an "across" clue number.
bool PuzManager::cellNeedsAcrossNumber(qint8 x, qint8 y, qint8 width, const QByteArray &puzzleSolution) const
{
    // Check that there is a blank to the left of us
    if (x == 0 || puzzleSolution[Coords(x - 1, y).toIndex(width)] == '.') {
        // Check that there is space (at least two cells) for a word here
        if (x + 1 < width && puzzleSolution[Coords(x + 1, y).toIndex(width)] != '.') {
            return true;
        }
    }
    return false;
}

//  Returns true if the cell at (x, y) gets an "down" clue number.
bool PuzManager::cellNeedsDownNumber(qint8 x, qint8 y, qint8 width, const QByteArray &puzzleSolution) const
{
    // Check that there is a blank to the left of us
    if (y == 0 || puzzleSolution[Coords(x, y - 1).toIndex(width)] == '.') {
        // Check that there is space (at least two cells) for a word here
        if (y + 1 < width && puzzleSolution[Coords(x, y + 1).toIndex(width)] != '.') {
            return true;
        }
    }
    return false;
}

QByteArray PuzManager::readZeroTerminatedString(QDataStream &dataStream)
{
    QByteArray string;
    char *buffer = new char[1];

    while (dataStream.readRawData(buffer, 1) == 1 && buffer[0] != '\0') {
        string.append(buffer[0]);
    }

    delete [] buffer;
    return string;
}
