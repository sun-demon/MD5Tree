#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThreadPool>
#include <QAtomicInt>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

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

    void onHashReady(const QString &fileName, const QString &hash, int threadId);
    void onTaskFinished();

private:
    void updateButtonsState();
    void startScanning();

    Ui::MainWindow *ui;
    QThreadPool m_pool;

    QAtomicInt m_stopFlag{0};
    QAtomicInt m_processed{0};
    int m_total = 0;

    bool m_isRunning = false;
};

#endif // MAINWINDOW_H
