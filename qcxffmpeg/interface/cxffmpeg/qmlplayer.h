#ifndef QMLPLAYER_H
#define QMLPLAYER_H

#include <QQuickPaintedItem>
#include <QImage>
#include "cxffmpeg/interface.h"
class WebRTCDecoder;
class VideoDecoderController;
class VideoPlayer;
class PLAYER_FFMPEG_API QMLPlayer : public QQuickPaintedItem
{
    Q_OBJECT
public:
    QMLPlayer(QQuickItem *parent = 0);
    ~QMLPlayer();

    QString getUrl() const;
    Q_INVOKABLE void setUrl(const QString &value);

    Q_INVOKABLE void start(QString urlStr);
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool getLinkState();
    Q_INVOKABLE void setQmlEngine(QObject *engine);

private:
    void paint(QPainter *painter) override;
    void rowVideoData(QImage data);

protected slots:
    void onVideoFrameInfo(int width,int height,int format);
    void onVideoFrameDataFinish();

signals:
    void sigVideoFrameDataReady();

private:
    QImage m_image;
    QString m_url;
    VideoDecoderController* m_decoderController;
    WebRTCDecoder *m_webrtc_decoder=nullptr;

    bool m_linkState;
    QTimer* m_timer = nullptr;
    QQmlEngine* m_engine= nullptr;
    VideoPlayer *m_player= nullptr;
};

#endif // QMLPLAYER_H
