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
    }
}

bool IOManager::read(CrosswordData &crossData)
{
    CrosswordIO *manager = nullptr;
    if (m_fileFormat == PuzFormat) {
        manager = new PuzManager(m_device);
    } else if (m_fileFormat == KwpFormat) {
        manager = new KwpManager(m_device);
    } else if (m_fileFormat == KwpzFormat) {
        manager = new KwpzManager(m_device);
    } else if (m_fileFormat == UnknowFormat) {
        setErrorString(i18n("Unknown crossword format."));
        return false;
    }

    Q_ASSERT(manager != nullptr);

    bool readOk = manager->read(crossData);
    if (!readOk) {
        setErrorString(i18n("Crossword reading error: %1", manager->errorString()));
    }

    return readOk;
}

bool IOManager::write(const CrosswordData &crossData)
{
    CrosswordIO *manager = nullptr;
    if (m_fileFormat == KwpFormat) {
        manager = new KwpManager(m_device);
    } else if (m_fileFormat == KwpzFormat) {
        manager = new KwpzManager(m_device);
    } else if (m_fileFormat == PuzFormat) {
        setErrorString(i18n("PUZ writing isn't supported."));
        return false;
    } else if (m_fileFormat == UnknowFormat) {
        setErrorString(i18n("Unknown crossword format."));
        return false;
    }

    Q_ASSERT(manager != nullptr);

    bool writeOk = manager->write(crossData);
    if (!writeOk) {
        setErrorString(i18n("Crossword writing error: %1", manager->errorString()));
    }

    return writeOk;
}
