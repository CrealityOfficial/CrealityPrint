#ifndef PLAYER_FFMPEG_VIDEO_DECODER_H_
#define PLAYER_FFMPEG_VIDEO_DECODER_H_

extern "C"
{
#define __STDC_CONSTANT_MACROS

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include<libavcodec/avcodec.h>
#include<libswresample/swresample.h>
}

#include <QContiguousCache>
#include <QThread>
#include <QObject>
#include <QImage>
#include "interface/cxffmpeg/framesource.h"
class VideoDecoder : public FrameSource
{
    Q_OBJECT
public:
    void stopplay();

    void startPlay(const QString& strUrl);
    int width() override;
    int height() override;
    int format() override;
    void receiveFrame(); 
signals:
    void videoFrameInfo(int width,int height,int format);
    void videoFrameDataFinish(QString url);

private:
    AVFormatContext* m_fmtCtx = nullptr;
    AVCodecContext* m_videoCodecCtx = nullptr;
    AVStream* m_videoStream = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    int m_videoStreamIndex = 0;

    AVPixelFormat m_pixFmt;
    int m_width=1280, m_height=720;
    bool isStop = false;
};

class VideoDecoderController : public QObject
{
    Q_OBJECT
public:
    VideoDecoderController(QObject* parent = nullptr);
    ~VideoDecoderController();

    void startThread(const QString& serverAddress);
    void stopThread();
    VideoDecoder* findDecoder(const QString& serverAddress) { return m_decoders[serverAddress];}

signals:
    void videoFrameInfo(int width,int height,int format);
public slots:
    void videoFrameDataFinish(QString url);
    void onVideoFrameInfo(int width,int height,int format);

private:
    std::map<QString, VideoDecoder*> m_decoders;
};

#endif // ! PLAYER_FFMPEG_VIDEO_DECODER_H_
