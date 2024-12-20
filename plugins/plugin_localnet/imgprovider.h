#ifndef _ImgProvider_H
#define _ImgProvider_H

#include <QQuickImageProvider>

class ImgProvider : public QObject, 
	public QQuickImageProvider
{
public:
	ImgProvider(QObject* parent = nullptr);
	virtual ~ImgProvider();

public:
	void setImage(const QImage& pimg);
	void setImage(const QImage& pimg, QString printerID);
	QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize);

private:
	QImage img;
	QImage defaultIcon;
	QMap<QString, QImage> imageSourceMap;
};

#endif // !_ImgProvider_H