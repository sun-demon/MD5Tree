#ifndef HASHTASK_H
#define HASHTASK_H

#include <QObject>
#include <QRunnable>
#include <QString>

class HashTask : public QObject, public QRunnable
{
    Q_OBJECT
public:
    HashTask(const QString &filePath, QAtomicInt *stopFlag);

    void run() override;

signals:
    void resultReady(const QString &fileName, const QString &hash, int threadId);
    void taskFinished();

private:
    QString m_filePath;
    QAtomicInt *m_stopFlag;
};

#endif // HASHTASK_H
