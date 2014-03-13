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

#ifndef CROSSWORDPREVIEWWIDGET_H
#define CROSSWORDPREVIEWWIDGET_H

#include <QLabel>
#include <kfileitem.h>

namespace KIO
{
class PreviewJob;
}

class CrosswordPreviewWidget : public QLabel
{
    Q_OBJECT

public:
    CrosswordPreviewWidget(QWidget* parent = 0);

    void showPreview(const QString &fileName,
                     const QString &mimeType =
                         "application/x-krosswordpuzzle-compressed");
    QSize previewSize() const {
        return m_previewSize;
    };
    void setPreviewSize(const QSize &previewSize) {
        m_previewSize = previewSize;
    };

protected slots:
    void previewJobGotPreview(const KFileItem &fileItem, const QPixmap &pixmap);
    void previewJobFailed(const KFileItem &fileItem);

private:
    KIO::PreviewJob *m_previewJob;
    QSize m_previewSize;
};

#endif // CROSSWORDPREVIEWWIDGET_H
