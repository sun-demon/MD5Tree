#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "hashtask.h"

#include <QHeaderView>
#include <QAbstractItemView>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // update table column widths
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

    connect(ui->browseButton, &QPushButton::clicked,
            this, &MainWindow::onBrowseClicked);
    connect(ui->startButton, &QPushButton::clicked,
            this, &MainWindow::onStartClicked);
    connect(ui->stopButton, &QPushButton::clicked,
            this, &MainWindow::onStopClicked);

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

    ui->browseButton->setEnabled(!m_isRunning);
    ui->startButton->setEnabled(hasDir && !m_isRunning);
    ui->stopButton->setEnabled(m_isRunning);
}

void MainWindow::onBrowseClicked()
{
    const QString startDir = ui->pathLineEdit->text().isEmpty()
                                 ? QDir::homePath()
                                 : ui->pathLineEdit->text();

    const QString dir = QFileDialog::getExistingDirectory(this, "Выберите директорию", startDir);

    if (!dir.isEmpty()) {
        ui->pathLineEdit->setText(dir);
        updateButtonsState();
    }
}

void MainWindow::onStartClicked()
{
    const QString dir = ui->pathLineEdit->text();
    if (dir.isEmpty()) return;

    ui->resultsTable->setRowCount(0);
    m_processed.storeRelaxed(0);
    m_stopFlag.storeRelaxed(0);

    QStringList files;
    QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        files << it.next();

    m_total = files.size();
    if (m_total == 0) {
        ui->statusbar->showMessage("Файлы не найдены");
        return;
    }

    m_isRunning = true;
    updateButtonsState();
    ui->statusbar->showMessage(QString("Обработка %1 файлов...").arg(m_total));

    for (const QString &path : files) {
        auto *task = new HashTask(path, &m_stopFlag);
        connect(task, &HashTask::resultReady, this, &MainWindow::onHashReady);
        connect(task, &HashTask::taskFinished, this, &MainWindow::onTaskFinished);
        m_pool.start(task);
    }
}

void MainWindow::onStopClicked()
{
    m_stopFlag.storeRelaxed(1);
    m_pool.clear();
    ui->statusbar->showMessage("Остановлено");
}

void MainWindow::onHashReady(const QString &fileName, const QString &hash, int threadId)
{
    const int row = ui->resultsTable->rowCount();
    ui->resultsTable->insertRow(row);

    auto *nameItem = new QTableWidgetItem(fileName);
    auto *hashItem = new QTableWidgetItem(hash);
    hashItem->setTextAlignment(Qt::AlignCenter);

    auto *idItem = new QTableWidgetItem(QString::number(threadId));
    idItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    ui->resultsTable->setItem(row, 0, nameItem);
    ui->resultsTable->setItem(row, 1, hashItem);
    ui->resultsTable->setItem(row, 2, idItem);
}

void MainWindow::onTaskFinished()
{
    const int done = m_processed.fetchAndAddRelaxed(1) + 1;
    ui->statusbar->showMessage(QString("Обработано %1 из %2").arg(done).arg(m_total));

    if (done >= m_total) {
        m_isRunning = false;
        updateButtonsState();
        ui->statusbar->showMessage(QString("Готово: %1 файлов").arg(m_total));
    }
}
