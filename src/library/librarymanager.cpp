#include "librarymanager.h"

#include "io/krosswordxmlreader.h"
#include "krossword.h" // TO REMOVE FOR A I/O MANAGER

#include <klocalizedstring.h>

#include <QDebug>
#include <QCryptographicHash>

QByteArray calculate_file_hash(QCryptographicHash& function, const QString& url)
{
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

FileSystemModel::FileSystemModel(QObject *parent) : QFileSystemModel(parent)
{
    setReadOnly(false); // so the files (crosswords) can be moved...
    setNameFilters(QStringList() << "*.kwpz");
    setNameFilterDisables(false); //hidden (not just disable) the unwanted files
    connect(this, SIGNAL(directoryLoaded(QString)), this, SLOT(loadThumbnails(QString)));
}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    //we need to customize just the files column, not the dates one
    if(index.column() == 0) {
        QString libraryItem = QFileSystemModel::data(index, QFileSystemModel::FilePathRole).toString();
        QFileInfo fi(libraryItem);

        if(fi.isFile()) {
            QString errorString;
            KrossWordXmlReader::KrossWordInfo info = KrossWordXmlReader::readInfo(KUrl(libraryItem), &errorString);
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

void FileSystemModel::loadThumbnails(QString path)
{
    QModelIndexList fileIndexList = match(index(path), QFileSystemModel::FileNameRole, "*.kwpz", -1, Qt::MatchWildcard | Qt::MatchRecursive);

    QModelIndex fileIndex;
    KFileItemList fileItemList;
    foreach (fileIndex, fileIndexList) {
        if(fileIndex.data(QFileSystemModel::FilePathRole).toString().startsWith(path + "/"))
            fileItemList.append(KFileItem(KFileItem::Unknown, KFileItem::Unknown, KUrl("file://" + fileIndex.data(QFileSystemModel::FilePathRole).toString()), true));
    }

    if(fileItemList.count() != 0) {
        m_previewJob = new KIO::PreviewJob(fileItemList, 64, 64, 0, 1, false, true, 0);
        m_previewJob->setAutoDelete(true);
        connect(m_previewJob, SIGNAL(gotPreview(KFileItem, QPixmap)), this, SLOT(previewJobGotPreview(KFileItem, QPixmap)));
        connect(m_previewJob, SIGNAL(failed(KFileItem)), this, SLOT(previewJobFailed(KFileItem)));
        m_previewJob->start();
    }
}

void FileSystemModel::previewJobGotPreview(const KFileItem &fi, const QPixmap &pix)
{
    QString fileName = fi.url().path();
    QModelIndex idx = index(fileName);
    if (!idx.isValid()) {
        qDebug() << "Item for preview image not found";
    } else {
        m_thumbs.insert(fileName, QIcon(pix));
        emit dataChanged(idx, idx); // to trigger the update via FileSystemModel::data
    }

}

void FileSystemModel::previewJobFailed(const KFileItem &fi)
{
    qDebug() << fi.url();
}

bool FileSystemModel::isInLibrary(const QString &path) const
{
    bool found = false;
    QString libraryPath = rootDirectory().path();

    if(path.startsWith(libraryPath + "/")) {
        found = true;
    } else {
        QCryptographicHash hashFunction(QCryptographicHash::Md5);
        QByteArray fileHash = calculate_file_hash(hashFunction, path);

        QModelIndexList fileIndexList = match(index(libraryPath), QFileSystemModel::FileNameRole, "*.kwpz", -1, Qt::MatchWildcard | Qt::MatchRecursive);
        foreach (QModelIndex index, fileIndexList) {
            QByteArray hash = calculate_file_hash(hashFunction, index.data(QFileSystemModel::FileNameRole).toString());

            if(fileHash == hash) {
                found = true;
                break;
            }
        }
    }

    return found;
}

FileSystemModel::E_ERROR_TYPE FileSystemModel::addCrossword(const QUrl &url, QString &outAddedCrosswordFilename, const QString &folder)
{
    QString libraryDir;
    if (folder.isEmpty() || !folder.startsWith(rootDirectory().path())) {
        libraryDir = rootDirectory().path();
    } else {
        libraryDir = folder;
    }

    Crossword::KrossWord krossWord;
    QString errorString;

    if (!krossWord.read(KUrl(url), &errorString/*, this*/)) {
        qDebug() << "addCrossword() reading error:" << errorString;
        outAddedCrosswordFilename = QString();
        return E_ERROR_TYPE::ReadError;
    } else {
        QString name = krossWord.title().trimmed();
        if (name.isEmpty())
            name = "untitled";

        if (!libraryDir.endsWith(QDir::separator()))
            libraryDir.append(QDir::separator());

        name.prepend(libraryDir);

        QString tmpFileName = name;
        int i = 1;
        while (QFile::exists(tmpFileName + ".kwpz"))
            tmpFileName = name + '_' + QString::number(i++);

        QString saveFileName = tmpFileName + ".kwpz";

        if (!krossWord.write(saveFileName, &errorString, Crossword::KrossWord::Normal, Crossword::KrossWord::KrossWordPuzzleCompressedXmlFile)) {
            qDebug() << "addCrossword() writing error:" << errorString;
            outAddedCrosswordFilename = QString();
            return E_ERROR_TYPE::WriteError;
        } else {
            outAddedCrosswordFilename = saveFileName;
            return E_ERROR_TYPE::Succeeded;
        }
    }
}
