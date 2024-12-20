#ifndef FRAMEPROVIDER_H
#define FRAMEPROVIDER_H
//#include <QtCore/QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

class FrameProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractVideoSurface *videoSurface READ videoSurface WRITE setVideoSurface)


public:
    QAbstractVideoSurface* videoSurface() const { return m_surface; }

private:
    QAbstractVideoSurface *m_surface = NULL;
    QVideoSurfaceFormat m_format;

public:


    void setVideoSurface(QAbstractVideoSurface *surface)
    {
        if (m_surface && m_surface != surface  && m_surface->isActive()) {
            m_surface->stop();
        }

        m_surface = surface;

        if (m_surface && m_format.isValid())
        {
            m_format = m_surface->nearestFormat(m_format);
            m_surface->start(m_format);

        }
    }

    void setFormat(int width, int heigth, int ft)
    {
        QSize size(width, heigth);
        QVideoSurfaceFormat format(size, (QVideoFrame::PixelFormat)ft);
        m_format = format;

        if (m_surface) 
        {
            if (m_surface->isActive())
            {
                m_surface->stop();
            }
            m_format = m_surface->nearestFormat(m_format);
            m_surface->start(m_format);
        }
    }

public slots:
    void onNewVideoContentReceived(const QVideoFrame &frame)
    {
        if (m_surface)
            m_surface->present(frame);
    }
};
#endif