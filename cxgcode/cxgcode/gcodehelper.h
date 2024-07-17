#ifndef CXGCODE_CXSW_GCODEHELPER_1603698936550_H
#define CXGCODE_CXSW_GCODEHELPER_1603698936550_H
#include "cxgcode/interface.h"
#include <QtCore/QString>
#include <QtGui/QImage>

#include <vector>
#include "ccglobal/tracer.h"

namespace cxsw
{
	CXGCODE_API bool cxSaveGCode(const QString& fileName, const QString& previewImageString,
		const QStringList& layerString, const QString& prefixString, const QString& tailCode);

	CXGCODE_API void image2String(const QImage& image, int width, int height, bool clear, QString& outString);
	CXGCODE_API QImage* resizeModule(QImage* previewImage);

	CXGCODE_API void getImageStr(QString& imageStr, QImage* Image, int layers, QString sPreImgFormat, float layerHeight = 0., bool multiFormat = false,
		const QString& path = QString(""));

	CXGCODE_API int getLineStart(QImage* Image);

	CXGCODE_API int getLineEnd(QImage* Image);
}

#endif // CXGCODE_CXSW_GCODEHELPER_1603698936550_H