#include "imgprovider.h"
#include <QDebug>

ImgProvider::ImgProvider(QObject* parent)
	: QObject(parent)
	, QQuickImageProvider(QQuickImageProvider::Image)
{
	defaultIcon.load(":/UI/photo/crealityIcon.png");
}

ImgProvider::~ImgProvider()
{

}

void ImgProvider::setImage(const QImage& pimg)
{
	img = pimg;
}

void ImgProvider::setImage(const QImage& pimg, QString printerID)
{
	QString key = QString("icons/%1").arg(printerID);
	imageSourceMap[key] = pimg;
}

QImage ImgProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
	if (id.startsWith("icons"))
	{
		return (imageSourceMap.contains(id) ? imageSourceMap[id] : defaultIcon);
	}
	else
	{
		return this->img;
	}
}


