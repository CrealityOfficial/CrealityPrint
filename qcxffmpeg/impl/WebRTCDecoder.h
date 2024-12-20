#ifndef PLAYER_FFMPEG_WEBRTC_DECODER_H_
#define PLAYER_FFMPEG_WEBRTC_DECODER_H_
#include <QContiguousCache>
#include <QThread>
#include <QObject>
#include <QImage>
#include <QFuture> 
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif
extern "C"
{
#define __STDC_CONSTANT_MACROS
//#include "video/yangrecordthread.h"
#include "yangplayer/YangPlayerHandle.h"
#include "yangstream/YangStreamType.h"
//#include "yangplayer/YangPlayWidget.h"
#include <yangutil/yangavinfotype.h>
#include <yangutil/sys/YangSysMessageI.h>
#include <yangutil/sys/YangSocket.h>
#include <yangutil/sys/YangLog.h>
#include <yangutil/sys/YangMath.h>

}
#include "interface/cxffmpeg/framesource.h"
class WebRTCDecoder :  public FrameSource,public YangSysMessageI
{
    enum Status {
    STOPPED = 1,
    CONNECTTING = 2,
    CONNECTED = 3
    };
    Q_OBJECT
public:
    static WebRTCDecoder* GetInstance();
    void stopplay();
    void startPlay(const QString& strUrl);
    WebRTCDecoder();
    ~WebRTCDecoder();
    void success();
    void failure(int32_t errcode);
    bool isStop() { return m_isStop; }
    int width() override;
    int height() override;
    int format() override;
    bool receiveFrame(); 
signals:
    void videoFrameInfo(int width,int height,int format);
    void videoFrameDataFinish(QString url);
    void RtcConnectFailure(int errcode);
private slots:
    void connectFailure(int errcode);
private:
    bool m_isStop = false;
    Status m_status = STOPPED;
    YangPlayerHandle* m_player;
    YangFrame m_frame;
    static WebRTCDecoder *g_pSingleton;
protected:
    YangContext* m_context;
    QString m_url;
    QFuture<void> m_playFutrue;
    int m_width=1920;
    int m_height=1080;
};

#endif // ! PLAYER_FFMPEG_WEBRTC_DECODER_H_