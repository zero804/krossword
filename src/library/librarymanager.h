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

#include <functional>

#include <QFileSystemModel>

#include <KIO/PreviewJob>

/**
 * @brief This is the library model. It's essentially a wrapper of QFileSystemModel with some
 *        special methods and informations.
 */
class LibraryManager : public QFileSystemModel
{
    Q_OBJECT

public:
    enum E_ERROR_TYPE {
        /**
         * Return value for no error
         */
        Succeeded = 0,

        /**
         * Something went wrong when reading a file
         */
        ReadError,

        /**
         * Something went wrong when writing a file
         */
        WriteError
    };

public:
    explicit LibraryManager(QObject *parent = 0);

    /**
     * @param path is the filepath of a crossword
     * @return true if the crossword is in the library
     */
    bool isInLibrary(const QString &path) const;

    /**
     * @brief Make a new folder (!FIXME folder are category) in the library
     * @param folderName will be the name of the new folder
     * @return true if there is no other folder named folderName
     */
    bool newFolder(const QString &folderName);

    /**
     * @brief Add a crossword in the library (can be a new one or dropped from GUI)
     * @param url is the path where to find the resource
     * @param outAddedCrosswordFilename is the new crossword filename added to the library
     * @param folder the subfolder name where to copy it
     * @return E_ERROR_TYPE::Succeeded (all good), E_ERROR_TYPE::ReadError (bad resource), E_ERROR_TYPE::WriteError(cannot write there)
     */
    E_ERROR_TYPE addCrossword(const QUrl &url, QString &outCrosswordUrl, const QString &folder = QString());

    /**
     * @brief Removes crosswords and folders
     * @param index of item to remove
     * @return true if was able to remove the item
     */
    bool remove(const QModelIndex &index);

    /**
     * @brief Returns some extra info for our model
     * @see QFileSystemModel::data
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    /**
     * @return all crosswords in the library
     */
    QFileInfoList getCrosswordsFilePath() const;

    /**
     * @return all folders in the library
     */
    QFileInfoList getFoldersPath() const;

    /**
     * @brief set custom callback for directoryLoaded signal
     */
    void setOnDirectoryLoadedFunction(const std::function<void(void)> &func);

    /**
     * @brief clear custom callback for directoryLoaded signal
     */
    void clearOnDirectoryLoadedFunction();

private:
    QHash<QString, QIcon> m_thumbs;
    QHash<QString, QByteArray> m_crosswordsHash;
    KIO::PreviewJob *m_previewJob;

    std::function<void(void)> m_function;

private slots:
    void loadThumbnailsSlot(const QString &path);
    void previewJobGotPreview(const KFileItem &fi, const QPixmap &pix);
    void previewJobFailed(const KFileItem &fi);
    void computeCrosswordsHashSlot(const QString &path);
    void onDirectoryLoaded(const QString &path);

};

#endif // LIBRARYMANAGER_H
