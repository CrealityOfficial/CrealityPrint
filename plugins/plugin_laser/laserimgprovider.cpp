#include "laserimgprovider.h"

LaserImgProvider::LaserImgProvider() 
    :QQuickImageProvider(QQmlImageProviderBase::Image)
{

}

LaserImgProvider::~LaserImgProvider()
{

}

QImage LaserImgProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    if(m_drawobjectlist.contains(id))
    {
        return this->m_drawobjectlist.value(id)->imageData();
    }else{
        return QImage();
    }

}
void LaserImgProvider::setObject(const QString& id, DrawObject* drawObj)
{
    m_drawobjectlist.insert(id,drawObj);
}
