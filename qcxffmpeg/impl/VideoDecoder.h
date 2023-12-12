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

class VideoDecoder : public QObject
{
    Q_OBJECT
public:
    void stopplay();

    void startPlay(const QString& strUrl);
    
signals:
    void videoFrameDataReady(QString url, QImage data);
    void videoFrameDataFinish(QString url);

private:
    AVFormatContext* m_fmtCtx = nullptr;
    AVCodecContext* m_videoCodecCtx = nullptr;
    AVStream* m_videoStream = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    int m_videoStreamIndex = 0;

    AVPixelFormat m_pixFmt;
    int m_width, m_height;
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

signals:

    void videoFrameDataReady(QString url, QImage data);

public slots:
    void videoFrameDataFinish(QString url);
    void onVideoFrameDataReady(QString url, QImage data);

private:
    std::map<QString, VideoDecoder*> m_decoders;
};

#endif // ! PLAYER_FFMPEG_VIDEO_DECODER_H_
