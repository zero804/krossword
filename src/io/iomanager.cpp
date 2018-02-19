/*
*   Copyright 2017-2018 Giacomo Barazzetti <giacomosrv@gmail.com>
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

#include "iomanager.h"

#include "kwpmanager.h"
#include "kwpzmanager.h"
#include "puzmanager.h"

#include <QMimeDatabase>

IOManager::IOManager(QFile *file)
    : CrosswordIO(file)
{
    QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(file->fileName(), QMimeDatabase::MatchDefault);
    if (mimeType.inherits("application/x-crossword")) {
        m_fileFormat = PuzFormat;
    } else if (mimeType.inherits("application/x-krosswordpuzzle")) {
        m_fileFormat = KwpFormat;
    } else if (mimeType.inherits("application/x-krosswordpuzzle-compressed")) {
        m_fileFormat = KwpzFormat;
    } else {
        m_fileFormat = UnknowFormat;
        setErrorString(i18n("Unknow crossword format."));
    }
}

bool IOManager::read(CrosswordData &crossData)
{
    bool readOk = false;

    if (m_fileFormat == PuzFormat) {
        PuzManager puzManager(m_device);
        readOk = puzManager.read(crossData);
        if (!readOk) {
            setErrorString(i18n("Error reading AcrossLite's .puz-format."));
            return false;
        }
    } else if (m_fileFormat == KwpFormat) {
        KwpManager kwpManager(m_device);
        readOk = kwpManager.read(crossData);
        if (!readOk) {
            setErrorString(kwpManager.errorString());
            return false;
        }
    } else if (m_fileFormat == KwpzFormat) {
        KwpzManager kwpzManager(m_device);
        readOk = kwpzManager.read(crossData);
        if (!readOk) {
            setErrorString(kwpzManager.errorString());
            return false;
        }
    } else if (m_fileFormat == UnknowFormat) {
        return false;
    }

    return readOk;
}

bool IOManager::write(const CrosswordData &crossData)
{
    bool writeOk = false;

    if (m_fileFormat == KwpFormat) {
        KwpManager kwpManager(m_device);
        writeOk = kwpManager.write(crossData);
        if (!writeOk) {
            setErrorString(i18n("Error writing crossword: %1", kwpManager.errorString()));
            return false;
        }
    } else if (m_fileFormat == KwpzFormat) {
        KwpzManager kwpzManager(m_device);
        writeOk = kwpzManager.write(crossData);
        if (!writeOk) {
            setErrorString(i18n("Error writing compressed crossword: %1", kwpzManager.errorString()));
            return false;
        }
    } else if (m_fileFormat == PuzFormat) { // CHECK: we don't want to export in puz format...
        PuzManager puzManager(m_device);
        writeOk = puzManager.write(crossData);
        if (!writeOk) {
            setErrorString(i18n("Error writing AcrossLite's .puz-format."));
            return false;
        }
    } else if (m_fileFormat == UnknowFormat) {
        return false;
    }

    return writeOk;
}
