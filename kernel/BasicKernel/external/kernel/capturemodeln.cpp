#include "capturemodeln.h"

#include <mutex>

#include "qtuser3d/framegraph/texturerendertarget.h"
#include "qtuser3d/framegraph/colorpicker.h"

class GeneralModelImageProvider : public QQuickImageProvider {

public:
  explicit GeneralModelImageProvider()
			: QQuickImageProvider(QQuickImageProvider::ImageType::Image) {}

  virtual ~GeneralModelImageProvider() = default;

public:
  QImage requestImage(const QString& id, QSize* size, const QSize& requested_size) override {
		std::unique_lock<std::mutex> lock{ mutex_ };
		*size = image_.size();
		if (requested_size.isValid()) {
			return image_.scaled(requested_size);
		}
		return image_;
	}

public:
	void updateImage(const QImage& image) {
		std::unique_lock<std::mutex> lock{ mutex_ };
		image_ = image;
	}

private:
	std::mutex mutex_;
	QImage image_;
};





CaptureModelN::CaptureModelN(QObject* parent)
	: QObject(parent)
	, m_renderTarget(nullptr)
	, m_colorPicker(nullptr)
{
	m_renderTarget = new qtuser_3d::TextureRenderTarget();

	m_colorPicker = new qtuser_3d::ColorPicker();
	m_colorPicker->useSelfCameraSelector(true);
	m_colorPicker->setEnabled(true);
	m_colorPicker->setClearColor(QColor(0, 0, 0, 0));
	m_colorPicker->setTextureRenderTarget(m_renderTarget);
	m_colorPicker->setFilterKey("modelzproject", 0);
}

CaptureModelN::~CaptureModelN()
{

}

void CaptureModelN::setResolution(const QSize& res)
{
	m_renderTarget->resize(res);
}

void CaptureModelN::setRequestCallback(qtuser_3d::requestCallFunc func)
{
	m_colorPicker->setRequestCallback([this, func](QImage& image) {
		Q_EMIT imageUpdated(image);
		func(image);
	});
}

void CaptureModelN::focusBox(const qtuser_3d::Box3D& box)
{
	QVector3D center = box.center();
	QVector3D boxSize = box.size();

	Qt3DRender::QCamera* pickerCamera = m_colorPicker->camera();
	pickerCamera->setViewCenter(QVector3D(center.x(), center.y(), 0.0f));
	pickerCamera->setPosition(QVector3D(center.x(), center.y(), box.max.z()));
	pickerCamera->setUpVector(QVector3D(0.0f, 1.0f, 0.0f));

	pickerCamera->setFarPlane(10000.0f);
	pickerCamera->setNearPlane(1.0f);
	pickerCamera->setProjectionType(Qt3DRender::QCameraLens::OrthographicProjection);
	pickerCamera->setBottom(-boxSize.y() / 2.0f);
	pickerCamera->setTop(boxSize.y() / 2.0f);
	pickerCamera->setLeft(-boxSize.x() / 2.0f);
	pickerCamera->setRight(boxSize.x() / 2.0f);
}

void CaptureModelN::attachToGraphNode(Qt3DRender::QFrameGraphNode* frameGraph)
{
	m_colorPicker->setParent(frameGraph);
}

void CaptureModelN::detachFromGraphNode()
{
	m_colorPicker->setParent((Qt3DCore::QNode*)nullptr);
}

void CaptureModelN::startCapture()
{
	m_colorPicker->requestCapture();
}

QQuickImageProvider* CaptureModelN::createImageProvider() {
	auto* provider = new GeneralModelImageProvider;
	connect(this, &CaptureModelN::imageUpdated, [provider](const QImage& image) {
		provider->updateImage(image);
	});
	return provider;
}
