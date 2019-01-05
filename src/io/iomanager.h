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

#ifndef IOMANAGER_H
#define IOMANAGER_H

#include "crosswordio.h"

enum FileFormat {
    UnknowFormat,
    KwpFormat,
    KwpzFormat,
    PuzFormat
};

class IOManager : public CrosswordIO
{
public:
    IOManager(QFile *file);

    bool read(CrosswordData &crossData) override;
    bool write(const CrosswordData &crossData) override; // CHECK: specify mimetype? (optional)

private:
    FileFormat m_fileFormat;
};

#endif // IOMANAGER_H