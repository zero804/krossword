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

#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include <kdialog.h>

class QLabel;
class QGridLayout;
namespace Crossword
{
class KrossWord;
}
using namespace Crossword;

/** A dialog to show statistics of a crossword. */
class StatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatisticsDialog(KrossWord *krossWord, QWidget* parent = 0);

private:
    void setup();
    QLabel *label(const QString &text, bool title = false);
    void addStatisticsValue(QGridLayout *layout, const QString &title,
                            int count, int totalCount,
                            const QString &toolTip = QString());

    KrossWord *m_krossWord;
};

#endif // STATISTICSDIALOG_H
