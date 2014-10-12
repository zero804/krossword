/*
* Copyright 2014 Andrea Barazzetti <andreadevsrv@gmail.com>
* Copyright 2014 Giacomo Barazzetti <giacomosrv@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LIBRARYMANAGER_H
#define LIBRARYMANAGER_H

#include <QFileSystemModel>

#include <KIO/PreviewJob>

class LibraryManager : public QFileSystemModel
{
    Q_OBJECT

public:
    enum E_ERROR_TYPE {
        Succeeded = 0,
        ReadError,
        WriteError
    };

public:
    explicit LibraryManager(QObject *parent = 0);

    bool isInLibrary(const QString &path) const;
    bool newFolder(const QString &folderName);
    E_ERROR_TYPE addCrossword(const QUrl &url, QString &outAddedCrosswordFilename, const QString &folder = QString());

    bool remove(const QModelIndex &index);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QFileInfoList getCrosswordsFilePath() const;
    QFileInfoList getFoldersPath() const;

private:
    QHash<QString, QIcon> m_thumbs;
    QHash<QString, QByteArray> m_crosswordsHash;
    KIO::PreviewJob *m_previewJob;

protected slots:
    void loadThumbnailsSlot(QString path);
    void previewJobGotPreview(const KFileItem &fi, const QPixmap &pix);
    void previewJobFailed(const KFileItem &fi);
    void computeCrosswordsHashSlot(const QString& path);

};

#endif // LIBRARYMANAGER_H
