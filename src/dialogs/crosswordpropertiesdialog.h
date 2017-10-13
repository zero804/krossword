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

#ifndef CROSSWORDPROPERTIESDIALOG_H
#define CROSSWORDPROPERTIESDIALOG_H

#include "ui_properties.h"

using namespace Crossword;

/** A dialog to move all cells of a crossword. */
class CrosswordPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CrosswordPropertiesDialog(KrossWord *krossWord, QWidget* parent = 0, Qt::WindowFlags flags = 0);

    int columns() const;
    int rows() const;
    KrossWord::ResizeAnchor anchor() const;
    QString title() const;
    QString author() const;
    QString copyright() const;
    QString notes() const;

signals:
    void conversionRequested(const CrosswordTypeInfo &targetTypeInfo);

protected slots:
    void rowsChanged(int rows);
    void columnsChanged(int columns);
    void convertClicked();
    void sizeChanged(int columns, int rows);
    void resizeAnchorChanged(int id);
    void resetSizeClicked();

private:
    enum ArrowCharIndex {
        ArrowNW = 0, ArrowN = 1, ArrowNE = 2,
        ArrowW = 3, ArrowNone = 4, ArrowE = 5,
        ArrowSW = 6, ArrowS = 7, ArrowSE = 8
    };

    static const QList< QChar > ArrowChars;

    void setAnchorIcons(KrossWord::ResizeAnchor anchor);
    void updateInfoText(KrossWord::ResizeAnchor anchor);
    void updateInfoText(int columns, int rows);
    void updateInfoText(KrossWord::ResizeAnchor anchor, int columns, int rows);

    KrossWord *m_krossWord;
    Ui::properties ui_properties;
    QHash< int, KrossWord::ResizeAnchor > m_anchorIdToAnchor;

};

#endif // CROSSWORDPROPERTIESDIALOG_H
