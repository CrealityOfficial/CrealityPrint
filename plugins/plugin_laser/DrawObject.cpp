#include "drawobject.h"

DrawObject::DrawObject(QObject *parent) : QObject(parent)
{

}
DrawObject::~DrawObject()
{

}
void DrawObject::setName(QString name)
{
    m_name = name;
}
QString DrawObject::name()
{
    return m_name;
}
QString DrawObject::type()
{
    return m_type;
}
void DrawObject::setQmlObject(QObject* obj)
{
    m_qmlobject = obj;
}
QObject* DrawObject::qmlObject()
{
    return m_qmlobject;
}
void DrawObject::setImageData(QImage &data)
{
    m_ImageData = data;
}
void DrawObject::setOriginImageData(QImage &data)
{
    m_originImageData = data;
    m_ImageData = m_originImageData;
}

void DrawObject::setBigImageData(QImage& data)
{
    m_bigImageData = data;
    m_dstImageData = data;
}

void DrawObject::setDstImageData(QImage& data)
{
    m_dstImageData = data;
}

void DrawObject::setImgType(QString type)
{
    m_type = type;
}
void DrawObject::setPath(QPainterPath path)
{
    m_path = path;
}

QImage DrawObject::imageData()
{
    return m_ImageData;
}

QImage DrawObject::originImageData()
{
    return m_originImageData;
}

QImage DrawObject::bigImageData()
{
    return m_bigImageData;
}

QImage DrawObject::dstImageData()
{
    return m_dstImageData;
}

QPainterPath DrawObject::path()
{
    return m_path;
}
