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

#ifndef CONVERTCROSSWORDDIALOG_H
#define CONVERTCROSSWORDDIALOG_H

#include "ui_convert_crossword_type.h"

using namespace Crossword;

/** A dialog to convert a crossword. */
class ConvertCrosswordDialog : public KDialog
{
    Q_OBJECT

public:
    explicit ConvertCrosswordDialog(KrossWord *krossWord,
                                    QWidget* parent = 0, Qt::WFlags flags = 0);

    CrosswordTypeInfo crosswordTypeInfo() {
        return m_convertTypeInfo;
    };

protected slots:
    void crosswordTypeChanged(int index);
    void crosswordTypeInfoChanged(const CrosswordTypeInfo &typeInfo);

private:
    KrossWord *m_krossWord;
    Ui::convert_crossword_type ui_convert_crossword_type;

    CrosswordTypeInfo m_convertTypeInfo;
    bool m_changedUserDefinedSettings;
    int m_previousConvertToTypeIndex;
};

#endif // CONVERTCROSSWORDDIALOG_H
