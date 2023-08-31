#ifndef CAPTURE_MODEL_N_202307251057_H
#define CAPTURE_MODEL_N_202307251057_H

#include <QtQuick/QQuickImageProvider>
#include <qframegraphnode.h>
//#include "qtuser3d/framegraph/colorpicker.h"
#include "qtuser3d/math/box3d.h"

namespace qtuser_3d
{
	class ColorPicker;
	class TextureRenderTarget;
	typedef std::function<void(QImage& image)> requestCallFunc;
}

class CaptureModelN : public QObject
{
	Q_OBJECT
public:
	CaptureModelN(QObject* parent = nullptr);
	virtual ~CaptureModelN();

	void setResolution(const QSize& res);
	void focusBox(const qtuser_3d::Box3D& box);
	void setRequestCallback(qtuser_3d::requestCallFunc func);

	void attachToGraphNode(Qt3DRender::QFrameGraphNode * frameGraph);
	void detachFromGraphNode();

	void startCapture();

	QQuickImageProvider* createImageProvider();

private:
	Q_SIGNAL void imageUpdated(const QImage& image);

private:
	qtuser_3d::ColorPicker* m_colorPicker;
	qtuser_3d::TextureRenderTarget* m_renderTarget;
};


#endif // !CAPTURE_MODEL_N_202307251057_H
