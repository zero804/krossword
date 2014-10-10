#ifndef LIBRARYMANAGER_H
#define LIBRARYMANAGER_H

#include <QFileSystemModel>

#include <KIO/PreviewJob>

class FileSystemModel : public QFileSystemModel
{
    Q_OBJECT

public:
    enum E_ERROR_TYPE {
        Succeeded = 0,
        ReadError,
        WriteError
    };

public:
    explicit FileSystemModel(QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    bool isInLibrary(const QString &path) const;
    E_ERROR_TYPE addCrossword(const QUrl &url, QString &outAddedCrosswordFilename, const QString &folder = QString());

private:
    QHash<QString, QIcon> m_thumbs;
    KIO::PreviewJob *m_previewJob;

protected slots:
    void loadThumbnails(QString path);
    void previewJobGotPreview(const KFileItem &fi, const QPixmap &pix);
    void previewJobFailed(const KFileItem &fi);

};

#endif // LIBRARYMANAGER_H
