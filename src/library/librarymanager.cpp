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

#include "librarymanager.h"

#include "io/krosswordxmlreader.h"
#include "krossword.h" // TO REMOVE FOR A I/O MANAGER

#include <QDebug>
#include <QCryptographicHash>

#include <klocalizedstring.h> // temporary for i18nc

QByteArray computeFileHash(const QString& url)
{
    QCryptographicHash function(QCryptographicHash::Md5);
    QFile f(url);
    f.open(QIODevice::ReadOnly);
    if (f.isOpen()) {
        function.addData(f.readAll());
        QByteArray hash = function.result();
        f.close();
        function.reset();
        return hash;
    }
    return QByteArray();
}

LibraryManager::LibraryManager(QObject *parent) : QFileSystemModel(parent)
{
    setReadOnly(false); // so the files (crosswords) can be moved...
    setNameFilters(QStringList() << "*.kwpz");
    setNameFilterDisables(false); //hidden (not just disable) the unwanted files

    connect(this, SIGNAL(directoryLoaded(QString)), this, SLOT(loadThumbnailsSlot(QString)));
    connect(this, SIGNAL(directoryLoaded(QString)), this, SLOT(computeCrosswordsHashSlot(QString)));
}

QVariant LibraryManager::data(const QModelIndex &index, int role) const
{
    //we need to customize just the files column, not the dates one
    if(index.column() == 0) {
        QString libraryItem = QFileSystemModel::data(index, QFileSystemModel::FilePathRole).toString();
        QFileInfo fi(libraryItem);

        if(fi.isFile()) {
            QString errorString;
            KrossWordXmlReader::KrossWordInfo info = KrossWordXmlReader::readInfo(QUrl::fromLocalFile(libraryItem), &errorString);
            if (!info.isValid()) {
                qDebug() << "Error reading crossword info from library file" << errorString;
            }

            QString title = info.title.isEmpty() ? fi.fileName().remove(QRegExp("\\." + fi.suffix() + '$', Qt::CaseInsensitive)) : info.title;

            QString itemText = QString("<b>%1</b><br>%2 %3x%4<br>%5 %6 - %7")
                               .arg(title)
                               .arg(i18nc("The title for sizes of crosswords in the library tree view", "Size:"))
                               .arg(info.width)
                               .arg(info.height)
                               .arg(i18nc("The title for authors of crosswords in the library tree view", "Author(s):"))
                               .arg(info.authors)
                               .arg(info.copyright);

            switch (role) {
            case Qt::DisplayRole:
                return itemText;
                break;
            case Qt::DecorationRole:
                if (m_thumbs.contains(libraryItem))
                    return m_thumbs.value(libraryItem);
                break;
            //case Qt::UserRole: //remember userRole + 1/2/3 are already used by qfilesystemmodel
            //    break;
            default:
                return QFileSystemModel::data(index, role);
            }
        }
    }

    return QFileSystemModel::data(index, role);
}

void LibraryManager::onDirectoryLoaded(const QString &path)
{
    Q_UNUSED(path);
    m_function();
}

void LibraryManager::setOnDirectoryLoadedFunction(const std::function<void(void)> &func)
{
    if (m_function) {
        clearOnDirectoryLoadedFunction();
    }

    m_function = func;
    connect(this, SIGNAL(directoryLoaded(QString)), this, SLOT(onDirectoryLoaded(QString)));
}

void LibraryManager::clearOnDirectoryLoadedFunction()
{
    m_function = nullptr;
    disconnect(this, SIGNAL(directoryLoaded(QString)), this, SLOT(onDirectoryLoaded(QString)));
}

void LibraryManager::loadThumbnailsSlot(const QString &path)
{
    QModelIndexList fileIndexList = match(index(path), QFileSystemModel::FileNameRole, "*.kwpz", -1, Qt::MatchWildcard | Qt::MatchRecursive);

    QModelIndex fileIndex;
    KFileItemList fileItemList;
    foreach (fileIndex, fileIndexList) {
        if (fileIndex.data(QFileSystemModel::FilePathRole).toString().startsWith(path + "/")) {
            QUrl url = fileIndex.data(QFileSystemModel::FilePathRole).toUrl();
            url.setScheme("file");
            fileItemList.append(KFileItem(url));
        }
    }

    if (!fileItemList.empty()) {
        QStringList plugin("crosswordthumbnail");     
        m_previewJob = new KIO::PreviewJob(fileItemList, QSize(64, 64), &plugin);
        connect(m_previewJob, SIGNAL(gotPreview(KFileItem, QPixmap)), this, SLOT(previewJobGotPreview(KFileItem, QPixmap)));
        connect(m_previewJob, SIGNAL(failed(KFileItem)), this, SLOT(previewJobFailed(KFileItem)));
        m_previewJob->start();
    }
}

void LibraryManager::computeCrosswordsHashSlot(const QString& path)
{
    Q_UNUSED(path);

    QFileInfoList crosswordsPath = getCrosswordsFilePath();

    foreach(QFileInfo path, crosswordsPath) {
        QByteArray hash = computeFileHash(path.filePath());

        m_crosswordsHash.insert(path.fileName(), hash);
    }

    disconnect(this, SIGNAL(directoryLoaded(QString)), this, SLOT(computeCrosswordsHashSlot(QString)));
}

void LibraryManager::previewJobGotPreview(const KFileItem &fi, const QPixmap &pix)
{
    QString fileName = fi.url().path();
    QModelIndex idx = index(fileName);
    if (!idx.isValid()) {
        qDebug() << "Item for preview image not found";
    } else {
        m_thumbs.insert(fileName, QIcon(pix));
        emit dataChanged(idx, idx); // to trigger the update via LibraryManager::data
    }
}

void LibraryManager::previewJobFailed(const KFileItem &fi)
{
    qDebug() << "Preview job failed for file" << fi.url().url(QUrl::PreferLocalFile);
}

bool LibraryManager::isInLibrary(const QString &path) const
{
    return m_crosswordsHash.values().contains(computeFileHash(path));
}

bool LibraryManager::newFolder(const QString &folderName)
{
    QDir dir(rootDirectory().path());
    if (dir.exists(folderName)) {
        return false;
    } else {
        dir.mkdir(folderName);
        return true;
    }
}

LibraryManager::E_ERROR_TYPE LibraryManager::addCrossword(const QUrl &url, QString &outCrosswordUrl, const QString &folder)
{
    QString filePath;
    if (folder.isEmpty() || !folder.startsWith(rootDirectory().path())) {
        filePath = rootDirectory().path();
    } else {
        filePath = folder;
    }

    if (!filePath.endsWith(QDir::separator())) {
        filePath.append(QDir::separator());
    }

    Crossword::KrossWord krossWord;
    QString errorString;

    if (!krossWord.read(url, &errorString/*, this*/)) {
        qDebug() << "addCrossword() reading error:" << errorString;
        outCrosswordUrl = QString();
        return E_ERROR_TYPE::ReadError;
    } else {
        QString fileName = krossWord.getTitle().trimmed();
        if (fileName.isEmpty()) {
            fileName = "untitled";
        }

        fileName.remove("#");
        fileName.remove("?");

        QString tmpFileName = fileName;
        int i = 1;
        while (QFile::exists(filePath + tmpFileName + ".kwpz")) {
            tmpFileName = fileName + '_' + QString::number(i++);
        }

        const QString fileUrl = filePath + tmpFileName + ".kwpz";

        if (!krossWord.write(fileUrl, &errorString, Crossword::KrossWord::Normal, Crossword::KrossWord::KrossWordPuzzleCompressedXmlFile)) {
            qDebug() << "addCrossword() writing error:" << errorString;
            outCrosswordUrl = QString();
            return E_ERROR_TYPE::WriteError;
        } else {
            outCrosswordUrl = fileUrl;
            m_crosswordsHash.insert(QUrl::fromLocalFile(fileUrl).path(), computeFileHash(fileUrl));
            return E_ERROR_TYPE::Succeeded;
        }
    }
}

QFileInfoList LibraryManager::getCrosswordsFilePath() const
{
    QFileInfoList crosswordPaths;
    QFileInfoList fiSubDirs = QDir(this->rootPath()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot) << QFileInfo(this->rootPath());

    foreach(QFileInfo fi, fiSubDirs) {
        QFileInfoList fifiles = QDir(fi.filePath()).entryInfoList(QDir::Files);
        foreach(QFileInfo file, fifiles) {
            crosswordPaths.append(file.filePath());
        }
    }

    return crosswordPaths;
}

QFileInfoList LibraryManager::getFoldersPath() const
{
    QFileInfoList folderPaths;
    folderPaths << QFileInfo(this->rootPath()) << QDir(this->rootPath()).entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);

    return folderPaths;
}

bool LibraryManager::remove(const QModelIndex& index)
{
    QStringList filenames;
    if (this->isDir(index)) {
        for(int i = 0; i < this->rowCount(index); ++i) {

            QModelIndex childIndex = this->index(i, 0, index);
            QString path = this->data(childIndex, QFileSystemModel::FileNameRole).toString();
            filenames << path;
        }
    } else {
        filenames << this->data(index, QFileSystemModel::FileNameRole).toString();
    }

    bool removed = QFileSystemModel::remove(index);

    if (removed) {
        foreach(QString filename, filenames) {
            m_crosswordsHash.remove(filename);
        }
    }

    return removed;
}
