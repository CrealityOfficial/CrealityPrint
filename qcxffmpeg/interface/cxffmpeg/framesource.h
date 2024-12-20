#ifndef FRAMESOURCE_H
#define FRAMESOURCE_H
#include <QVideoFrame>
class FrameSource : public QObject
{
    Q_OBJECT
public:
        virtual int width()=0;
        virtual int height()=0;
        virtual int format()=0;
signals:
       void newFrameAvailable(const QVideoFrame &frame);
};
#endif // QMLPLAYER_H