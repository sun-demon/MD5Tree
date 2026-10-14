#ifndef HASHTASK_H
#define HASHTASK_H

#include <QObject>
#include <QRunnable>
#include <QAtomicInt>
#include <QString>
#include <QStringList>

class HashTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    HashTask(const QString &filePath, int row, QAtomicInt *stopFlag);

    void run() override;

signals:
    void resultReady(int row, const QString &hash, const QString &threadId);
    void bytesProcessed(qint64 bytes);
    void taskFinished(int row, bool completed);

private:
    QString m_filePath;
    int m_row;
    QAtomicInt *m_stopFlag;
};

class FileScanTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    FileScanTask(const QString &directory, QAtomicInt *stopFlag);

    void run() override;

signals:
    void scanProgress(int directories, int files, qint64 bytes);
    void scanFinished(const QStringList &files, int directories,
                      qint64 bytes, bool canceled);

private:
    QString m_directory;
    QAtomicInt *m_stopFlag;
};

#endif // HASHTASK_H
