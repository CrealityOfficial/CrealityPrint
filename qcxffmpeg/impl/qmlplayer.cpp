#include "cxffmpeg/qmlplayer.h"
#include <QPainter>
#include <QTimer>
#include <QAudioOutput>
#include "VideoDecoder.h"
#include "WebRTCDecoder.h"
#include "cxffmpeg/videoplayer.h"
QMLPlayer::QMLPlayer(QQuickItem *parent)
    :QQuickPaintedItem(parent)
{
    m_decoderController = new VideoDecoderController(this);
    connect(m_decoderController, &VideoDecoderController::videoFrameInfo, this, &QMLPlayer::onVideoFrameInfo);
    connect(m_decoderController, &VideoDecoderController::videoFrameDataFinish, this, &QMLPlayer::onVideoFrameDataFinish);
    m_linkState = false;
    m_timer = new QTimer(this);
    
    
}
void QMLPlayer::setQmlEngine(QObject *engine)
{
    m_engine = qmlEngine(this);
    if(m_player==nullptr)
        m_player = new VideoPlayer(m_engine,this);
    }
QMLPlayer::~QMLPlayer()
{
    if (m_decoderController)
    {
        delete m_decoderController;
    }
    if (m_webrtc_decoder)
    {
        //delete m_webrtc_decoder;
    }
    m_timer->stop();
}

void QMLPlayer::paint(QPainter *painter)
{
    return;
   
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
    
    //m_timer->setInterval(40);
    //connect(m_timer, SIGNAL(timeout()), this, SLOT(update()));
    //m_timer->start();
#ifdef DEBUG_WEBRTC
    urlStr = "webrtc://172.23.208.238:8000/call/demo";
#endif
    if (urlStr.indexOf("webrtc_local") > 0)
    {
        
        if (!m_webrtc_decoder)
        {
            m_webrtc_decoder = WebRTCDecoder::GetInstance();
            connect(m_webrtc_decoder, &WebRTCDecoder::videoFrameInfo, this, &QMLPlayer::onVideoFrameInfo);
        }
        
        //urlStr = urlStr.replace("webrtc", "http");
        setUrl(urlStr);
        m_webrtc_decoder->startPlay(urlStr);
    }
    else {
        setUrl(urlStr);
        m_decoderController->startThread(urlStr);
        
    }
    
}

void QMLPlayer::stop()
{
    qDebug() << "QMLPlayer::stop()";
    //m_decoderController->stopplay();
    m_decoderController->stopThread();
    if (m_webrtc_decoder)
    {
        m_webrtc_decoder->stopplay();
    }
    //m_image = QImage();
}

void QMLPlayer::onVideoFrameInfo(int width,int height,int format)
{
    if (m_url.indexOf("webrtc_local") > 0)
    {
        m_player->setSource(m_webrtc_decoder);
    }else{
        m_player->setSource(m_decoderController->findDecoder(m_url));
    }
    
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
