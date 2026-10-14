#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "hashtask.h"

#include <QHeaderView>
#include <QAbstractItemView>
#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileSystemModel>
#include <QFileInfo>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedLayout>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>

FileResultsModel::FileResultsModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int FileResultsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_results.size();
}

int FileResultsModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 3;
}

QVariant FileResultsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_results.size())
        return {};

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == 1)
            return int(Qt::AlignCenter);
        if (index.column() == 2)
            return int(Qt::AlignRight | Qt::AlignVCenter);
    }

    if (role != Qt::DisplayRole)
        return {};

    const Result &result = m_results.at(index.row());
    switch (index.column()) {
    case 0: return result.fileName;
    case 1: return result.hash;
    case 2: return result.threadId;
    default: return {};
    }
}

QVariant FileResultsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section) {
    case 0: return "Имя файла";
    case 1: return "MD5";
    case 2: return "ID потока";
    default: return {};
    }
}

int FileResultsModel::appendPending(const QString &fileName)
{
    const int row = m_results.size();
    beginInsertRows(QModelIndex(), row, row);
    m_results.append({fileName, "Хеширование...", QString()});
    endInsertRows();
    return row;
}

void FileResultsModel::updateResult(int row, const QString &hash, const QString &threadId)
{
    if (row < 0 || row >= m_results.size())
        return;

    Result &result = m_results[row];
    result.hash = hash;
    result.threadId = threadId;
    emit dataChanged(index(row, 1), index(row, 2));
}

void FileResultsModel::markCanceled(int row)
{
    if (row < 0 || row >= m_results.size())
        return;

    m_results[row].hash = "Отменено";
    emit dataChanged(index(row, 1), index(row, 1));
}

void FileResultsModel::clear()
{
    beginResetModel();
    m_results.clear();
    endResetModel();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_pool.setMaxThreadCount(2);

    m_resultsModel = new FileResultsModel(this);
    ui->resultsTable->setModel(m_resultsModel);

    QHeaderView *header = ui->resultsTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->resultsTable->setColumnWidth(1, 250);
    ui->resultsTable->setColumnWidth(2, 80);

    // cross-platform monospace font
    QFont monoFont = ui->resultsTable->font();
    monoFont.setStyleHint(QFont::Monospace);
    ui->resultsTable->setFont(monoFont);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setFixedWidth(340);
    m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_statusLabel->setText("Готово");
    ui->statusbar->addWidget(m_statusLabel);

    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(350);
    connect(m_statusTimer, &QTimer::timeout, this, [this]() {
        if (m_isScanning || m_stopRequested) {
            m_animationStep = (m_animationStep + 1) % 4;
            updateStatusLabel();
        }
    });

    connect(ui->browseButton, &QPushButton::clicked,
            this, &MainWindow::onBrowseClicked);
    connect(ui->startButton, &QPushButton::clicked,
            this, &MainWindow::onStartClicked);
    connect(ui->stopButton, &QPushButton::clicked,
            this, &MainWindow::onStopClicked);

    m_directoryDialog = new QDialog(this);
    m_directoryDialog->setWindowTitle("Выберите директорию");
    m_directoryDialog->resize(560, 420);

    m_directoryModel = new QFileSystemModel(m_directoryDialog);
    m_directoryModel->setFilter(QDir::AllEntries | QDir::Drives | QDir::NoDotAndDotDot);

    m_directoryTree = new QTreeView(m_directoryDialog);
    m_directoryTree->setModel(m_directoryModel);
    m_directoryTree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_directoryTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_directoryTree->setHeaderHidden(true);
    for (int column = 1; column < m_directoryModel->columnCount(); ++column)
        m_directoryTree->hideColumn(column);

    auto *treeContainer = new QWidget(m_directoryDialog);
    auto *treeStack = new QStackedLayout(treeContainer);
    treeStack->setStackingMode(QStackedLayout::StackAll);
    treeStack->addWidget(m_directoryTree);

    m_directoryLoadingOverlay = new QWidget(treeContainer);
    m_directoryLoadingOverlay->setStyleSheet(
        "background-color: rgba(255, 255, 255, 210);");
    auto *loadingLayout = new QVBoxLayout(m_directoryLoadingOverlay);
    loadingLayout->addStretch();
    auto *loadingLabel = new QLabel("Загрузка дисков и папок...", m_directoryLoadingOverlay);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLayout->addWidget(loadingLabel);
    auto *loadingSpinner = new QProgressBar(m_directoryLoadingOverlay);
    loadingSpinner->setRange(0, 0);
    loadingSpinner->setTextVisible(false);
    loadingSpinner->setFixedWidth(180);
    loadingLayout->addWidget(loadingSpinner, 0, Qt::AlignHCenter);
    loadingLayout->addStretch();
    treeStack->addWidget(m_directoryLoadingOverlay);
    m_directoryLoadingOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, m_directoryDialog);
    QPushButton *selectButton = buttons->button(QDialogButtonBox::Ok);
    selectButton->setText("Выбрать");
    selectButton->setEnabled(false);
    buttons->button(QDialogButtonBox::Cancel)->setText("Отмена");

    auto *directoryLayout = new QVBoxLayout(m_directoryDialog);
    directoryLayout->addWidget(treeContainer);
    directoryLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted,
            m_directoryDialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected,
            m_directoryDialog, &QDialog::reject);
    connect(m_directoryTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this, selectButton](const QModelIndex &current) {
                selectButton->setEnabled(current.isValid()
                                          && m_directoryModel->isDir(current));
            });
    connect(m_directoryDialog, &QDialog::accepted, this, [this]() {
        const QModelIndex index = m_directoryTree->currentIndex();
        if (!index.isValid() || !m_directoryModel->isDir(index))
            return;

        ui->pathLineEdit->setText(m_directoryModel->filePath(index));
        updateButtonsState();
    });
    connect(m_directoryModel, &QFileSystemModel::directoryLoaded,
            this, [this](const QString &path) {
                if (QDir::cleanPath(path) == QDir::cleanPath(m_directoryModel->rootPath())) {
                    m_directoryRootLoaded = true;
                    m_directoryLoadingOverlay->hide();
                }
            });

    m_directoryLoadingOverlay->show();
    const QModelIndex rootIndex = m_directoryModel->setRootPath(QString());
    m_directoryTree->setRootIndex(rootIndex);

    updateButtonsState();
}

MainWindow::~MainWindow()
{
    m_stopFlag.storeRelaxed(1);
    m_pool.waitForDone();
    delete ui;
}

void MainWindow::updateButtonsState()
{
    const bool hasDir = !ui->pathLineEdit->text().isEmpty();

    ui->browseButton->setEnabled(!m_isRunning || m_stopRequested);
    ui->startButton->setEnabled(hasDir && !m_isRunning);
    ui->stopButton->setEnabled(m_isRunning && !m_stopRequested);
}

void MainWindow::onBrowseClicked()
{
    if (m_directoryDialog && m_directoryDialog->isVisible()) {
        m_directoryDialog->raise();
        m_directoryDialog->activateWindow();
        return;
    }

    if (!m_directoryRootLoaded)
        m_directoryLoadingOverlay->show();
    m_directoryDialog->show();
    m_directoryDialog->raise();
    m_directoryDialog->activateWindow();
}

void MainWindow::onStartClicked()
{
    const QString dir = ui->pathLineEdit->text();
    if (dir.isEmpty()) return;

    m_scanRoot = QDir(dir).absolutePath();
    m_resultsModel->clear();
    m_stopFlag.storeRelaxed(0);
    m_processed.storeRelaxed(0);
    m_nextFile = 0;
    m_activeTasks = 0;
    m_pendingFiles.clear();
    m_directoriesFound = 0;
    m_filesFound = 0;
    m_totalBytes = 0;
    m_processedBytes = 0;
    m_completedFiles = 0;
    m_total = 0;
    m_isRunning = true;
    m_stopRequested = false;
    m_isScanning = true;
    updateButtonsState();
    ui->statusbar->setBusy(true);
    ui->statusbar->setToolTip(QString("Найдено папок: %1, файлов: %2")
                                  .arg(m_directoriesFound).arg(m_filesFound));
    m_animationStep = 0;
    m_statusTimer->start();
    updateStatusLabel();

    auto *task = new FileScanTask(dir, &m_stopFlag);
    connect(task, &FileScanTask::scanProgress,
            this, &MainWindow::onScanProgress);
    connect(task, &FileScanTask::scanFinished,
            this, &MainWindow::onScanFinished);
    m_pool.start(task);
}

void MainWindow::onScanProgress(int directories, int files, qint64 bytes)
{
    m_directoriesFound = directories;
    m_filesFound = files;
    m_totalBytes = bytes;
    updateStatusLabel();
}

void MainWindow::onScanFinished(const QStringList &files, int directories,
                                qint64 bytes, bool canceled)
{
    m_isScanning = false;
    m_directoriesFound = directories;
    m_filesFound = files.size();
    m_totalBytes = bytes;
    if (canceled || m_stopFlag.loadRelaxed() != 0) {
        finishRun("Поиск остановлен");
        return;
    }

    m_total = files.size();
    if (m_total == 0) {
        finishRun("Файлы не найдены");
        return;
    }

    m_processed.storeRelaxed(0);
    m_processedBytes = 0;
    m_completedFiles = 0;
    m_pendingFiles = files;
    m_nextFile = 0;
    m_activeTasks = 0;
    ui->statusbar->setBusy(false);
    ui->statusbar->setProgress(0, m_totalBytes > 0 ? m_totalBytes : m_total);
    ui->statusbar->setToolTip(QString("%1 файлов, %2 байт")
                                  .arg(m_total).arg(m_totalBytes));
    updateStatusLabel();
    scheduleHashTasks();
}

void MainWindow::scheduleHashTasks()
{
    while (!m_stopRequested
           && m_nextFile < m_pendingFiles.size()
           && m_activeTasks < m_pool.maxThreadCount()) {
        const QString path = m_pendingFiles.at(m_nextFile++);
        const QString relativePath = QDir(m_scanRoot).relativeFilePath(path);
        const int row = m_resultsModel->appendPending(relativePath);
        auto *task = new HashTask(path, row, &m_stopFlag);
        connect(task, &HashTask::resultReady, this, &MainWindow::onHashReady);
        connect(task, &HashTask::bytesProcessed, this, &MainWindow::onHashProgress);
        connect(task, &HashTask::taskFinished, this, &MainWindow::onTaskFinished);
        ++m_activeTasks;
        m_pool.start(task);
    }
}

void MainWindow::onStopClicked()
{
    m_stopFlag.storeRelaxed(1);
    m_stopRequested = true;
    updateButtonsState();
    updateStatusLabel();
}

void MainWindow::finishRun(const QString &status)
{
    m_isRunning = false;
    m_stopRequested = false;
    m_isScanning = false;
    m_statusTimer->stop();
    m_pendingFiles.clear();
    ui->statusbar->clearProgress();
    ui->statusbar->setToolTip(QString());
    updateButtonsState();
    m_statusLabel->setText(status);
}

void MainWindow::onHashReady(int row, const QString &hash, const QString &threadId)
{
    m_resultsModel->updateResult(row, hash, threadId);
}

void MainWindow::onHashProgress(qint64 bytes)
{
    m_processedBytes = qMin(m_totalBytes, m_processedBytes + bytes);
    if (m_totalBytes > 0) {
        ui->statusbar->setProgress(m_processedBytes, m_totalBytes);
    } else if (m_total > 0) {
        ui->statusbar->setProgress(m_completedFiles, m_total);
    }
}

void MainWindow::onTaskFinished(int row, bool completed)
{
    if (completed) {
        ++m_completedFiles;
        m_processed.fetchAndAddRelaxed(1);
    } else {
        m_resultsModel->markCanceled(row);
    }
    --m_activeTasks;
    if (m_totalBytes == 0 && m_total > 0) {
        ui->statusbar->setProgress(m_completedFiles, m_total);
    }
    updateStatusLabel();

    if (m_stopRequested) {
        if (m_activeTasks == 0)
            finishRun("Остановлено");
        return;
    }

    scheduleHashTasks();
    if (m_processed.loadRelaxed() >= m_total && m_activeTasks == 0) {
        ui->statusbar->setProgress(m_totalBytes, m_totalBytes > 0 ? m_totalBytes : m_total);
        finishRun("Готово");
    }
}

void MainWindow::updateStatusLabel()
{
    const QString dots(m_animationStep, QLatin1Char('.'));
    if (m_isScanning) {
        m_statusLabel->setText(
            m_stopRequested
                ? QString("Остановка поиска%1").arg(dots)
                : QString("Поиск: %1 папок, %2 файлов%3")
                      .arg(m_directoriesFound).arg(m_filesFound).arg(dots));
        ui->statusbar->setToolTip(QString("Найдено папок: %1, файлов: %2")
                                      .arg(m_directoriesFound).arg(m_filesFound));
    } else if (m_stopRequested) {
        m_statusLabel->setText(
            QString("Остановка: %1 / %2 файлов%3")
                .arg(m_completedFiles).arg(m_total).arg(dots));
    } else if (m_isRunning) {
        m_statusLabel->setText(QString("Обработано: %1 / %2 файлов")
                                   .arg(m_completedFiles).arg(m_total));
        ui->statusbar->setToolTip(
            QString("Хеширование: %1 из %2 байт")
                .arg(m_processedBytes).arg(m_totalBytes));
    }
}
