#include "WebRTCDecoder.h"
#include <QDebug>
#include <QImage>
#include <QPixmap>
#include <thread>
#include <QtConcurrent>
#include <yangstream/YangSynBuffer.h>
WebRTCDecoder* WebRTCDecoder::g_pSingleton = new (std::nothrow) WebRTCDecoder();
WebRTCDecoder::WebRTCDecoder()
{
    m_context = new YangContext();
    m_context->init();

    m_context->synMgr.session->playBuffer = (YangSynBuffer*)yang_calloc(sizeof(YangSynBuffer), 1);//new YangSynBuffer();
    yang_create_synBuffer(m_context->synMgr.session->playBuffer);

    m_context->avinfo.sys.mediaServer = Yang_Server_P2p;//Yang_Server_Srs/Yang_Server_Zlm
    m_context->avinfo.rtc.rtcSocketProtocol = Yang_Socket_Protocol_Udp;//

    m_context->avinfo.rtc.rtcLocalPort = 10000 + yang_random() % 15000;
    memset(m_context->avinfo.rtc.localIp, 0, sizeof(m_context->avinfo.rtc.localIp));
    yang_getLocalInfo(m_context->avinfo.sys.familyType, m_context->avinfo.rtc.localIp);
    m_context->avinfo.rtc.enableDatachannel = yangfalse;
    m_context->avinfo.rtc.iceCandidateType = YangIceHost;
    m_context->avinfo.rtc.turnSocketProtocol = Yang_Socket_Protocol_Udp;

    m_context->avinfo.rtc.enableAudioBuffer = yangtrue; //use audio buffer
    m_context->avinfo.audio.enableAudioFec = yangfalse; //srs not use audio fec
    m_player = YangPlayerHandle::createPlayerHandle(m_context, this);
}
WebRTCDecoder::~WebRTCDecoder()
{
    //stopplay();
}
WebRTCDecoder* WebRTCDecoder::GetInstance()
{
    return g_pSingleton;
}
void WebRTCDecoder::stopplay()
{
    m_isStop = true;
    m_playFutrue.waitForFinished();
    int waitCount = 100;
    while(m_status == CONNECTTING)
    {
        QThread::msleep(20);
        waitCount--;
        if(waitCount<=0)
        {
            break;
        }
    }
    if (m_player) m_player->stopPlay();

    qDebug()<<"hemiao:stopplay";
}

void WebRTCDecoder::startPlay(const QString& strUrl)
{
    qDebug()<<"hemiao:startplay";
    int waitCount = 100;
    while(m_status == CONNECTTING)
    {
        QThread::msleep(20);
        waitCount--;
        if(waitCount<=0)
        {
            break;
        }
        }
    m_status = CONNECTTING;
    m_url = strUrl;
    m_isStop = false;
    //QString url = "http://172.23.208.238:8000/call/demo";
    m_context->synMgr.session->playBuffer->resetVideoClock(m_context->synMgr.session->playBuffer->session);
    int32_t err = m_player->playRtc(0, m_url.toLatin1().data());
    if (!err)
    {
        m_playFutrue = QtConcurrent::run([this]() {
            //QThread::msleep(1000);
            bool bFirstFrame = true;
            while (!this->isStop())
            {
                //this->getRenderData();
                bool bValidFrame = this->receiveFrame();
                if(bFirstFrame && bValidFrame)
                {
                    emit videoFrameInfo(this->width(),this->height(),this->format());
                    bFirstFrame = false;
                }
                QThread::msleep(3);
            }
            return;
            });
    }
}


int WebRTCDecoder::width() 
{
    return m_width;
}
int WebRTCDecoder::height(){
    return m_height;
    }
int WebRTCDecoder::format() {
    return (int)QVideoFrame::Format_YUV420P;
    }
bool WebRTCDecoder::receiveFrame(){
     uint8_t* t_vb = m_context->synMgr.session->playBuffer->getVideoRef(m_context->synMgr.session->playBuffer->session, &m_frame);
    if (t_vb)
    {
        YangSynBuffer* sync_buffer = m_context->synMgr.session->playBuffer;

        m_width = sync_buffer->width(sync_buffer->session);
        m_height = sync_buffer->height(sync_buffer->session);
        if(m_width<=0)
        {
            return false;
        }
        QVideoFrame f((int)m_frame.nb, QSize(m_width, m_height), m_width, QVideoFrame::Format_YUV420P);
        if (f.map(QAbstractVideoBuffer::WriteOnly)) {
            memcpy(f.bits(), t_vb, m_frame.nb);
            f.setStartTime(0);
            f.unmap();
            emit newFrameAvailable(f);
        }
        return true;
    }
    return false;
    
}
void WebRTCDecoder::success()
{
    m_status = CONNECTED;
   qDebug()<<"success";
}
void WebRTCDecoder::failure(int32_t errcode)
{
    m_status = STOPPED;
    emit RtcConnectFailure(errcode);
}
void WebRTCDecoder::connectFailure(int errcode) {

}