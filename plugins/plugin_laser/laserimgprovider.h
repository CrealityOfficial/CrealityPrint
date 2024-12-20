#ifndef LASERIMGPROVIDER_H
#define LASERIMGPROVIDER_H

#include <QObject>
#include <QQuickImageProvider>
#include "drawobject.h"
class LaserImgProvider : public QQuickImageProvider
{
    //Q_OBJECT
public:
    explicit LaserImgProvider();
    ~LaserImgProvider();

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize);
    void setObject(const QString& id, DrawObject* drawObj);
    QString providerUrl() {return m_providerUrl;}
    void setProviderUrl(QString url) { m_providerUrl = url;}
signals:

public slots:

private:
    QMap<QString,DrawObject*> m_drawobjectlist;
    QString m_providerUrl;
};

#endif // LASERIMGPROVIDER_H
