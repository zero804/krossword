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

#ifndef IMAGECELLWIDGET_H
#define IMAGECELLWIDGET_H

#include <QtGui/QWidget>
#include "ui_image_properties_dock.h"
#include "global.h"

using namespace Crossword;

class ImageCellWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageCellWidget(ImageCell *imageCell, QWidget* parent = 0);

    void setImageCell(ImageCell *imageCell);
    ImageCell *imageCell() const {
        return m_imageCell;
    };

protected slots:
    void browseClicked();
    void horizontalCellSpanChanged(int value);
    void verticalCellSpanChanged(int value);

private:
    Ui::image_properties_dock ui_image_properties_dock;
    ImageCell *m_imageCell;
};

#endif // IMAGECELLWIDGET_H
