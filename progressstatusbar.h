#ifndef PROGRESSSTATUSBAR_H
#define PROGRESSSTATUSBAR_H

#include <QPaintEvent>
#include <QPainter>
#include <QStatusBar>
#include <QTimer>

class ProgressStatusBar : public QStatusBar
{
public:
    explicit ProgressStatusBar(QWidget *parent = nullptr)
        : QStatusBar(parent)
    {
        m_busyTimer.setInterval(30);
        connect(&m_busyTimer, &QTimer::timeout, this, [this]() {
            m_busyPosition = (m_busyPosition + 2) % 140;
            update();
        });
    }

    void setBusy(bool busy)
    {
        m_busy = busy;
        if (busy)
            m_busyTimer.start();
        else
            m_busyTimer.stop();
        update();
    }

    void setProgress(qint64 value, qint64 maximum)
    {
        m_busy = false;
        m_busyTimer.stop();
        m_progressValue = qBound<qint64>(0, value, qMax<qint64>(maximum, 0));
        m_progressMaximum = qMax<qint64>(maximum, 0);
        update();
    }

    void clearProgress()
    {
        m_busy = false;
        m_busyTimer.stop();
        m_progressMaximum = 0;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QStatusBar::paintEvent(event);

        QPainter painter(this);
        const QRect track(0, height() - 3, width(), 3);
        painter.fillRect(track, QColor(120, 120, 120, 55));

        if (m_busy) {
            const int segmentWidth = qMax(40, width() / 5);
            const int travel = width() + segmentWidth;
            const int x = (m_busyPosition * travel / 140) - segmentWidth;
            painter.fillRect(QRect(x, track.top(), segmentWidth, track.height()),
                             QColor(35, 125, 210, 210));
        } else if (m_progressMaximum > 0) {
            const int progressWidth = static_cast<int>(
                (static_cast<long double>(width()) * m_progressValue)
                / m_progressMaximum);
            painter.fillRect(QRect(0, track.top(), progressWidth, track.height()),
                             QColor(35, 125, 210, 210));
        } else {
            painter.fillRect(track, palette().color(QPalette::Window));
        }
    }

private:
    QTimer m_busyTimer;
    qint64 m_progressValue = 0;
    qint64 m_progressMaximum = 0;
    int m_busyPosition = 0;
    bool m_busy = false;
};

#endif // PROGRESSSTATUSBAR_H
