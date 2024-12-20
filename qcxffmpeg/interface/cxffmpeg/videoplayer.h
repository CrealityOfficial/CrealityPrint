
#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H
#include "framesource.h"
#include "frameprovider.h"
#include <QQmlEngine>
#include <QQmlContext>

class VideoPlayer : public QObject
{
    Q_OBJECT

public:
    VideoPlayer(QQmlEngine* engine,QObject *parent){
        m_engine = engine;
    }
    void setSource(FrameSource *source) {
        if(m_provider==nullptr)
        {
            m_provider = new FrameProvider();
            m_engine->rootContext()->setContextProperty("frameProvider", m_provider);
        }
        if(m_source)
        {
            QObject::disconnect(m_source, SIGNAL(newFrameAvailable(const QVideoFrame &)), m_provider, SLOT(onNewVideoContentReceived(const QVideoFrame &)));
        }
        m_source = source;
        // Set the correct format for the video surface (Make sure your selected format is supported by the surface)
        m_provider->setFormat(source->width(),source->height(), source->format());

        // Connect your frame source with the provider
        QObject::connect(source, SIGNAL(newFrameAvailable(const QVideoFrame &)), m_provider, SLOT(onNewVideoContentReceived(const QVideoFrame &)));

        
    }
private:
    QQmlEngine* m_engine;
    FrameSource* m_source=nullptr;
    FrameProvider* m_provider=nullptr;
};
#endif