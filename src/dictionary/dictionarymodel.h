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

#ifndef DICTIONARYMODEL_H
#define DICTIONARYMODEL_H

#include <QSqlTableModel>

class DictionaryModel : public QSqlTableModel
{
    Q_OBJECT

public:
    explicit DictionaryModel(QObject* parent = 0, QSqlDatabase db = QSqlDatabase());

    void setLimit(int lowerLimit, int upperLimit);
    void removeLimit();

protected:
    virtual QString selectStatement() const;

private:
    int m_lowerLimit;
    int m_upperLimit;
};

#endif // DICTIONARYMODEL_H
