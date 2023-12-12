#ifndef QMLPLAYER_H
#define QMLPLAYER_H

#include <QQuickPaintedItem>
#include <QImage>
#include "cxffmpeg/interface.h"

class VideoDecoderController;
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

private:
    void paint(QPainter *painter) override;
    void rowVideoData(QImage data);

protected slots:
    void onVideoFrameDataReady(QString url, QImage data);
    void onVideoFrameDataFinish();

signals:
    void sigVideoFrameDataReady();

private:
    QImage m_image;
    QString m_url;
    VideoDecoderController* m_decoderController;
    bool m_linkState;
    QTimer* m_timer = nullptr;
};

#endif // QMLPLAYER_H
