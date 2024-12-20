#ifndef DRAWOBJECT_H
#define DRAWOBJECT_H

#include <QObject>
#include <QImage>
#include <QPainterPath>
class DrawObject : public QObject
{
    Q_OBJECT
public:
    explicit DrawObject(QObject *parent = nullptr);
    ~DrawObject();
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    enum SHAPETYPE{BMP = 0,PATH = 1};
signals:
    void nameChanged();
public slots:
    void setName(QString name);
    void setQmlObject(QObject* obj);
    void setImageData(QImage &data);
    void setOriginImageData(QImage &data);
    void setBigImageData(QImage& data);
    void setDstImageData(QImage& data);
    void setImgType(QString type);
    void setPath(QPainterPath path);
    QImage imageData();
    QImage originImageData();
    QImage bigImageData();
    QImage dstImageData();
    QString name();
    QString type();
    QObject* qmlObject();
    QPainterPath path();
private:
    QString m_name;
    QString m_type;
    QObject* m_qmlobject;
    QImage m_ImageData;
    QImage m_originImageData;
    QImage m_bigImageData;
    QImage m_dstImageData;
    QPainterPath m_path;
};

#endif // DRAWOBJECT_H
