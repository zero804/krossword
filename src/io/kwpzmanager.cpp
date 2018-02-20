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

#include "kwpzmanager.h"
#include "kwpmanager.h"

#include <KZip>
#include <QBuffer>
#include <QDebug>

KwpzManager::KwpzManager(QIODevice *device)
    : CrosswordIO(device)
{
}

bool KwpzManager::read(CrosswordData &crossData)
{
    Q_ASSERT(m_device);
    Q_ASSERT(m_device->isReadable());

    setErrorString(QString());

    // Read compressed XML from the given IO device
    KZip zip(m_device);
    zip.setCompression(KZip::DeflateCompression);
    if (!zip.open(QIODevice::ReadOnly)) {
        setErrorString(i18n("Couldn't open the ZIP archive for reading"));
        return false;
    }
    const KArchiveDirectory *archive = zip.directory();
    if (!archive) {
        setErrorString(i18n("Couldn't get the archive contents"));
        return false;
    }
    if (!archive->entries().contains("crossword.kwp")) {
        setErrorString(i18n("Not a valid *.kwpz-file! The crossword file wasn't found (crossword.kwp)."));
        return false;
    }
    const KArchiveEntry *crosswordEntry = archive->entry("crossword.kwp");
    if (!crosswordEntry->isFile()) {
        setErrorString(i18n("Not a valid *.kwpz-file! No file 'crossword.kwp' found, it's a directory."));
        return false;
    }
    KArchiveFile *crosswordFile = (KArchiveFile*)crosswordEntry;
    QIODevice *crosswordDevice = crosswordFile->createDevice();
    crosswordDevice->open(QIODevice::ReadOnly);

    // Read the crossword
    KwpManager kwpManager(crosswordDevice);
    bool readOk = kwpManager.read(crossData);
    setErrorString(kwpManager.errorString());

    crosswordDevice->close();
    delete crosswordDevice;

    if (!zip.close()) {
        setErrorString(i18n("Couldn't close the ZIP archive")); // CHECK: a bit too much?
        return false;
    }

    return readOk;
}

bool KwpzManager::write(const CrosswordData &crossdata)
{
    Q_ASSERT(m_device);
    Q_ASSERT(m_device->isWritable());

    setErrorString(QString());

    // Write XML to a buffer
    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    KwpManager kwpManager(&buffer);
    bool writeOk = kwpManager.write(crossdata);
    buffer.close();

    if (!writeOk) {
        setErrorString(kwpManager.errorString());
        return false;
    }

    // Write compressed XML to the given IO device
    KZip zip(m_device);
    zip.setCompression(KZip::DeflateCompression);
    if (!zip.open(QIODevice::WriteOnly)) {
        setErrorString(i18n("Couldn't open the ZIP archive for writing"));
        return false;
    }
    if (!zip.prepareWriting("crossword.kwp", "krosswordpuzzle", "krosswordpuzzle", buffer.size())) {
        setErrorString(i18n("Error writing to the compressed file"));
        return false;
    }
    if (!zip.writeData(buffer.data(), buffer.size())) {
        setErrorString(i18n("Error writing to the compressed file"));
        return false;
    }
    if (!zip.finishWriting(buffer.size())) {
        setErrorString(i18n("Error writing to the compressed file"));
        return false;
    }
    if (!zip.close()) {
        setErrorString(i18n("Couldn't close the ZIP archive"));
        return false;
    }

    return true;
}
