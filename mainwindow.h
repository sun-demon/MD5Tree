#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThreadPool>
#include <QAtomicInt>
#include <QStringList>
#include <QAbstractTableModel>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QDialog;
class QFileSystemModel;
class QLabel;
class QTimer;
class QTreeView;
class QWidget;

class FileResultsModel : public QAbstractTableModel
{
public:
    explicit FileResultsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    int appendPending(const QString &fileName);
    void updateResult(int row, const QString &hash, const QString &threadId);
    void markCanceled(int row);
    void clear();

private:
    struct Result {
        QString fileName;
        QString hash;
        QString threadId;
    };

    QVector<Result> m_results;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onBrowseClicked();
    void onStartClicked();
    void onStopClicked();

    void onScanProgress(int directories, int files, qint64 bytes);
    void onScanFinished(const QStringList &files, int directories,
                        qint64 bytes, bool canceled);
    void onHashReady(int row, const QString &hash, const QString &threadId);
    void onHashProgress(qint64 bytes);
    void onTaskFinished(int row, bool completed);

private:
    void updateButtonsState();
    void finishRun(const QString &status);
    void scheduleHashTasks();
    void updateStatusLabel();

    Ui::MainWindow *ui;
    FileResultsModel *m_resultsModel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTimer *m_statusTimer = nullptr;
    QDialog *m_directoryDialog = nullptr;
    QFileSystemModel *m_directoryModel = nullptr;
    QTreeView *m_directoryTree = nullptr;
    QWidget *m_directoryLoadingOverlay = nullptr;
    bool m_directoryRootLoaded = false;
    QThreadPool m_pool;

    QAtomicInt m_stopFlag{0};
    QAtomicInt m_processed{0};
    int m_total = 0;
    int m_nextFile = 0;
    int m_activeTasks = 0;
    int m_directoriesFound = 0;
    int m_filesFound = 0;
    int m_completedFiles = 0;
    int m_animationStep = 0;
    qint64 m_totalBytes = 0;
    qint64 m_processedBytes = 0;
    QStringList m_pendingFiles;
    QString m_scanRoot;

    bool m_isRunning = false;
    bool m_stopRequested = false;
    bool m_isScanning = false;
};

#endif // MAINWINDOW_H
