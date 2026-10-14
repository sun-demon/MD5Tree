#include "hashtask.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <algorithm>

HashTask::HashTask(const QString &filePath, int row, QAtomicInt *stopFlag)
    : m_filePath(filePath)
    , m_row(row)
    , m_stopFlag(stopFlag)
{
    setAutoDelete(true);
}

void HashTask::run()
{
    if (m_stopFlag->loadRelaxed() != 0) {
        emit taskFinished(m_row, false);
        return;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit resultReady(m_row, "ERROR", 0);
        emit taskFinished(m_row, true);
        return;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);

    constexpr qint64 chunkSize = 256 * 1024;
    char buffer[chunkSize];
    qint64 unreportedBytes = 0;
    QElapsedTimer progressTimer;
    progressTimer.start();

    while (!file.atEnd()) {
        if (m_stopFlag->loadRelaxed() != 0) {
            if (unreportedBytes > 0)
                emit bytesProcessed(unreportedBytes);
            emit taskFinished(m_row, false);
            return;
        }
        const qint64 read = file.read(buffer, chunkSize);
        if (read <= 0) break;
        hash.addData(buffer, read);
        unreportedBytes += read;
        if (unreportedBytes >= 4 * 1024 * 1024 || progressTimer.elapsed() >= 100) {
            emit bytesProcessed(unreportedBytes);
            unreportedBytes = 0;
            progressTimer.restart();
        }
    }

    if (unreportedBytes > 0)
        emit bytesProcessed(unreportedBytes);
    const QString md5 = hash.result().toHex();
    const QString threadId = QString::number(
        reinterpret_cast<quintptr>(QThread::currentThreadId()));

    emit resultReady(m_row, md5, threadId);
    emit taskFinished(m_row, true);
}

FileScanTask::FileScanTask(const QString &directory, QAtomicInt *stopFlag)
    : m_directory(directory)
    , m_stopFlag(stopFlag)
{
    setAutoDelete(true);
}

void FileScanTask::run()
{
    QStringList files;
    int directories = 0;
    qint64 bytes = 0;
    QElapsedTimer progressTimer;
    progressTimer.start();
    QDirIterator it(m_directory, QDir::AllEntries | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (m_stopFlag->loadRelaxed() == 0 && it.hasNext()) {
        const QString path = it.next();
        const QFileInfo info(path);
        if (info.isDir()) {
            ++directories;
        } else {
            files << path;
            bytes += qMax<qint64>(0, info.size());
        }

        if (progressTimer.elapsed() >= 100) {
            emit scanProgress(directories, files.size(), bytes);
            progressTimer.restart();
        }
    }

    const bool canceled = m_stopFlag->loadRelaxed() != 0;
    if (canceled) {
        files.clear();
    } else {
        std::sort(files.begin(), files.end(), [](const QString &left, const QString &right) {
            const int insensitive = QString::compare(left, right, Qt::CaseInsensitive);
            return insensitive == 0 ? left < right : insensitive < 0;
        });
    }

    emit scanProgress(directories, files.size(), bytes);
    emit scanFinished(files, directories, bytes, canceled);
}
