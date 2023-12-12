#include "cxffmpeg/qmlplayer.h"
#include <QPainter>
#include <QTimer>
#include <QAudioOutput>
#include "VideoDecoder.h"

QMLPlayer::QMLPlayer(QQuickItem *parent)
    :QQuickPaintedItem(parent)
{
    m_decoderController = new VideoDecoderController(this);
    connect(m_decoderController, &VideoDecoderController::videoFrameDataReady, this, &QMLPlayer::onVideoFrameDataReady);
    connect(m_decoderController, &VideoDecoderController::videoFrameDataFinish, this, &QMLPlayer::onVideoFrameDataFinish);
    m_linkState = false;
    m_timer = new QTimer(this);
}

QMLPlayer::~QMLPlayer()
{
    if (m_decoderController)
    {
        delete m_decoderController;
    }
    m_timer->stop();
}

void QMLPlayer::paint(QPainter *painter)
{
    if (!m_image.isNull())
    {
        int imageH = m_image.height();
        int imageW = m_image.width();
        int screenH = this->height();
        int screenW = this->width();

        int scaledH = this->height();
        int scaledW = this->width();

        int offsetX = 0;
        int offsetY = 0;

        if (imageW > imageH)
        {
            scaledH = imageH * screenW / imageW;
            if (scaledH > screenH)
            {
                scaledH = screenH;
                scaledW = imageW * scaledH / imageH;

                offsetX = (screenW - scaledW) / 2;
                offsetY = 0;
            }
            else
            {
                offsetX = 0;
                offsetY = (screenH - scaledH) / 2;
            }
        }
        else
        {
            scaledW = imageW * scaledH / imageH;
            if (scaledW > screenW)
            {
                scaledW = screenW;
                scaledH = imageH * screenW / imageW;
                offsetX = 0;
                offsetY = (screenH - scaledH) / 2;
            }
            else
            {
                offsetX = (screenW - scaledW) / 2;
                offsetY = 0;
            }
        }


        QImage img = m_image.scaled(scaledW, scaledH);
        painter->drawImage(QPoint(offsetX, offsetY), img);
    }
    else
    {
        m_image = QImage(this->width(), this->height(), QImage::Format_RGB888);
        m_image.fill(QColor(0.0, 0.0, 0.0));
        QImage img = m_image.scaled(this->width(), this->height());
        painter->drawImage(QPoint(0, 0), img);
    }
}

void QMLPlayer::rowVideoData(QImage data)
{
    m_image = data;
}

QString QMLPlayer::getUrl() const
{
    return m_url;
}

void QMLPlayer::setUrl(const QString &value)
{
    m_url = value;
}

void QMLPlayer::start(QString urlStr)
{
    setUrl(urlStr);
    m_timer->setInterval(40);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(update()));
    m_timer->start();

    m_decoderController->startThread(urlStr);
}

void QMLPlayer::stop()
{
    qDebug() << "QMLPlayer::stop()";
    //m_decoderController->stopplay();
    m_image = QImage();
}

void QMLPlayer::onVideoFrameDataReady(QString url, QImage data)
{
    if (url != m_url)
    {
        return;
    }
    //qDebug() << "onVideoFrameDataReady data.width:"<< width << " data.height:"<< height;
    rowVideoData(data);
    m_linkState = true;

    emit sigVideoFrameDataReady();
}

void QMLPlayer::onVideoFrameDataFinish()
{
    for (int i = 0; i < 10; i++)
    {
        m_image.fill(QColor(0.0, 0.0, 0.0));
    }
}

bool QMLPlayer::getLinkState()
{
    return m_linkState;
}
