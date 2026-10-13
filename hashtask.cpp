#include "hashtask.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QThread>

HashTask::HashTask(const QString &filePath, QAtomicInt *stopFlag)
    : m_filePath(filePath)
    , m_stopFlag(stopFlag)
{
    setAutoDelete(true);
}

void HashTask::run()
{
    if (m_stopFlag->loadRelaxed() != 0) {
        emit taskFinished();
        return;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit resultReady(QFileInfo(m_filePath).fileName(), "ERROR", 0);
        emit taskFinished();
        return;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);

    constexpr qint64 chunkSize = 4096;
    char buffer[chunkSize];

    while (!file.atEnd()) {
        if (m_stopFlag->loadRelaxed() != 0) {
            emit taskFinished();
            return;
        }
        const qint64 read = file.read(buffer, chunkSize);
        if (read <= 0) break;
        hash.addData(buffer, read);
    }

    const QString md5 = hash.result().toHex();
    const int threadId = reinterpret_cast<quintptr>(QThread::currentThreadId());

    emit resultReady(QFileInfo(m_filePath).fileName(), md5, threadId);
    emit taskFinished();
}
