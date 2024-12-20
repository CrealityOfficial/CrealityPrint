#include "qtuser3d/framegraph/colorpicker.h"
#include "qtuser3d/framegraph/texturerendertarget.h"
#include <QtWidgets/QApplication>
#include <QtCore/QDebug>
#include <QCoreApplication>
#include <QElapsedTimer>

namespace qtuser_3d
{
	NamedReply::NamedReply(namedReplyFunc func, QObject* parent)
		: QObject(parent)
		, callback(func)
	{
	}

	NamedReply::~NamedReply()
	{
	}

	void NamedReply::invoke()
	{
		if (callback)
			callback(reply->image());
	}

	ColorPicker::ColorPicker(Qt3DCore::QNode* parent)
		:QFrameGraphNode(parent)
		, m_updateTimer(nullptr)
		, m_useDelay(true)
		, m_textureRenderTarget(nullptr)
		, m_capturing(false)
		, m_captureReply(nullptr)
	{
#ifdef _DEBUG
		m_debugName = QString("%1/pickTest.bmp").arg(QCoreApplication::applicationDirPath());
		qDebug() << QString("ColorPicker::ColorPicker %1").arg(m_debugName);
#endif
	
		m_renderTargetSelector = new Qt3DRender::QRenderTargetSelector(this);

		m_clearBuffer = new Qt3DRender::QClearBuffers(m_renderTargetSelector);
		m_clearBuffer->setClearColor(QColor(255, 255, 255, 255));
		m_clearBuffer->setBuffers(Qt3DRender::QClearBuffers::BufferType::ColorDepthBuffer);
		
		m_renderPassFilter = new Qt3DRender::QRenderPassFilter(m_clearBuffer);
		m_filterKey = new Qt3DRender::QFilterKey(m_renderPassFilter);
		m_filterKey->setName("pick");
		m_filterKey->setValue(0);
		m_renderPassFilter->addMatch(m_filterKey);


		m_renderPassFilter2 = new Qt3DRender::QRenderPassFilter(m_renderTargetSelector);
		m_filterKey2 = new Qt3DRender::QFilterKey(m_renderPassFilter2);
		m_filterKey2->setName("pick2nd");
		m_filterKey2->setValue(0);
		m_renderPassFilter2->addMatch(m_filterKey2);

		m_cameraSelector = new Qt3DRender::QCameraSelector();
		m_camera = new Qt3DRender::QCamera(m_cameraSelector);
		m_cameraSelector->setCamera(m_camera);

		{
			m_cameraSelector2 = new Qt3DRender::QCameraSelector(m_renderPassFilter2);
			m_cameraSelector2->setCamera(m_camera);
			m_renderCapture = new Qt3DRender::QRenderCapture(m_cameraSelector2);
		}

		//m_textureRenderTarget = new TextureRenderTarget();
		//m_renderTargetSelector->setTarget(m_textureRenderTarget);

		m_updateTimer = new QTimer(this);
		m_delayTimer = new QTimer(this);
		m_delayTimer->setSingleShot(true);
		connect(m_updateTimer, SIGNAL(timeout()), this, SIGNAL(signalUpdate()));
		connect(m_delayTimer, SIGNAL(timeout()), this, SLOT(delayCapture()));

		m_unUseTimer.setSingleShot(true);
		connect(&m_unUseTimer, &QTimer::timeout, this, &ColorPicker::unUse);
	}

	ColorPicker::~ColorPicker()
	{
		if (!m_cameraSelector->parent())
		{
			delete m_cameraSelector;
			m_cameraSelector = nullptr;
		}
	}

	void ColorPicker::resize(const QSize& size)
	{
		//qDebug() << "ColorPicker::resize " << size;
		QSize scaleSize = size * m_renderTargetRatio;

		m_colorPickerImage = QImage(scaleSize, QImage::Format_RGBA8888);
		m_colorPickerImage.fill(0xFFFFFFFF);

		if (m_textureRenderTarget)
			m_textureRenderTarget->resize(scaleSize);
	}

	//noted:  Only one render capture result is produced per requestCapture call even if the frame graph has multiple leaf nodes.
	void ColorPicker::requestCapture()
	{
#ifdef TEST_TEST
		qDebug() << "ColorPicker::requestCapture " << m_useDelay;
#endif
		m_createImageFinished = false;
		m_capturing = true;
		use();
		if(m_useDelay)
			m_delayTimer->start(100);
		else
		{
			if (m_captureReply)
			{
				delete m_captureReply;
				m_captureReply = nullptr;
			} 

			m_captureReply = m_renderCapture->requestCapture();
			connect(m_captureReply, &Qt3DRender::QRenderCaptureReply::completed, this, &ColorPicker::captureCompleted);

			m_updateTimer->start(40);
		}
	}
	
	RenderCaptor* ColorPicker::requestCapture(Qt3DRender::QCamera* camera, QObject* receiver, RenderCaptor::ReceiverHandleReplyFunc func)
	{
		RenderCaptor* captor = new RenderCaptor(m_renderPassFilter);
		captor->capture(camera, receiver, func);
		return captor;
	}

	void ColorPicker::requestSyncCapture(int timeout)
	{
		m_createImageFinished = false;
		m_capturing = true;
		m_delayTimer->stop();
		if (m_captureReply)
		{
			delete m_captureReply;
			m_captureReply = nullptr;
		}

		use();
		m_captureReply = m_renderCapture->requestCapture();
		connect(m_captureReply, &Qt3DRender::QRenderCaptureReply::completed, this, &ColorPicker::captureCompleted);
		m_updateTimer->start(40);
		
		QElapsedTimer timeoutTimer;
		timeoutTimer.start();
		while (m_capturing && timeoutTimer.elapsed() < timeout)
		{
			QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		}
	}

	void ColorPicker::requestNamedCapture(namedReplyFunc callback)
	{
		if (!callback)
			return;

		QTimer::singleShot(100, [this, callback]() {
			NamedReply* reply = new NamedReply(callback, this);
			reply->reply.reset(m_renderCapture->requestCapture());
			connect(reply->reply.data(), &Qt3DRender::QRenderCaptureReply::completed, this, &ColorPicker::namedCaptureCompleted);
			m_namedReplies.append(reply);
			m_updateTimer->start(40);
			});
	}

	void ColorPicker::setUseDelay(bool delay)
	{
		m_useDelay = delay;
	}

	void ColorPicker::delayCapture()
	{
#if 1
		qDebug() << "ColorPicker::delayCapture";
#endif

		if (m_captureReply)
		{
			delete m_captureReply;
			m_captureReply = nullptr;
		}

		use();
		m_captureReply = m_renderCapture->requestCapture();
		connect(m_captureReply, &Qt3DRender::QRenderCaptureReply::completed, this, &ColorPicker::captureCompleted);

		m_updateTimer->start(40);
		m_delayTimer->stop();
	}

	void ColorPicker::captureCompleted()
	{
		emit frameCaptureComplete();
#if 1
		qDebug() << "ColorPicker::captureCompleted";
#endif
		m_colorPickerImage = m_captureReply->image();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
		//m_captureReply.reset();
#else
		//m_captureReply.reset();
#endif

        m_updateTimer->stop();
		if (m_requestCallback)
		{
			m_requestCallback(m_colorPickerImage);
		}
		else
		{
            m_createImageFinished = true;
#if _DEBUG
        m_colorPickerImage.save(m_debugName);
#endif

            if (m_pickerFunc) m_pickerFunc(this);
		}

		m_capturing = false;
		// unUse();
		m_unUseTimer.start(500);
		emit updateColorPickerFinished();
	}

	void ColorPicker::namedCaptureCompleted()
	{
		QObject* s = sender();
		NamedReply* reply = nullptr;

		for (NamedReply* r : m_namedReplies)
		{
			if (r->reply.data() == s)
			{
				reply = r;
				break;
			}
		}

		if (reply)
			reply->invoke();

		m_namedReplies.removeOne(reply);
		if (reply)
			reply->deleteLater();

		if(m_namedReplies.size() == 0 && !m_captureReply)
			m_updateTimer->stop();
	}

	void ColorPicker::setFilterKey(const QString& name, int value)
	{
		//qDebug() << "ColorPicker::setFilterKey " << name << " = " << value;
		m_filterKey->setName(name);
		m_filterKey->setValue(value);
	}

	void ColorPicker::sourceMayChanged()
	{
		requestCapture();
	}

	void ColorPicker::setClearColor(const QColor& color)
	{
		//qDebug() << "ColorPicker::setClearColor " << color;
		m_clearBuffer->setClearColor(color);
	}

	void ColorPicker::setAllFilterKey(const QString& name, int value)
	{
		m_filterKey->setName(name);
		m_filterKey->setValue(value);

		m_filterKey2->setName(name);
		m_filterKey2->setValue(value);

	}

	bool ColorPicker::isPrepared()
	{
		return !m_capturing;
	}
	
	bool ColorPicker::pick(const QPoint& point, int* faceID)
	{
		if (m_capturing)
			return false;

		return pick(point.x(), point.y(), faceID);
	}

	bool ColorPicker::pick(int x, int y, int* faceID)
	{
		x *= m_renderTargetRatio;
		y *= m_renderTargetRatio;

		if (y >= m_colorPickerImage.height())
			return false;

		QRgb pixelval = m_colorPickerImage.pixel(x, y);

		if (pixelval == 0xFFFFFFFF)
			return false;

		unsigned r = qRed(pixelval);
		unsigned g = qGreen(pixelval);
		unsigned b = qBlue(pixelval);
		unsigned a = qAlpha(pixelval);
#if 0
		qDebug() << " r " << r << " g " << g << " b " << b << " " << a;
#endif
		if (faceID) *faceID = (int)(16777216 * a + 65536 * b + 256 * g + r);
		return true;
	}

	void ColorPicker::setDebugName(const QString& name)
	{
#ifdef _DEBUG
		m_debugName = name;
#endif
	}

    bool ColorPicker::getImageFinished()
    {
		//return &Qt3DRender::QRenderCaptureReply::isComplete;
		return m_createImageFinished;
    }

    void ColorPicker::setPickerFunc(selfPickerFunc func)
	{
		m_pickerFunc = func;
	}

	void ColorPicker::setRequestCallback(requestCallFunc func)
	{
		m_requestCallback = func;
	}

	void ColorPicker::useSelfCameraSelector(bool use)
	{
		//qDebug() << "ColorPicker::useSelfCameraSelector " << use;
		if (use)
		{
			m_cameraSelector->setParent(m_renderPassFilter);
		}
		else
		{
			m_cameraSelector->setParent((Qt3DCore::QNode*)nullptr);
		}
	}

	Qt3DRender::QCamera* ColorPicker::camera()
	{
		return m_camera;
	}

	void ColorPicker::useSelfCamera()
	{
		m_cameraSelector->setCamera(m_camera);
		m_cameraSelector2->setCamera(m_camera);
	}

	void ColorPicker::useCamera(Qt3DRender::QCamera* cam)
	{
		m_cameraSelector->setCamera(cam);
		m_cameraSelector2->setCamera(cam);
	}

	Qt3DRender::QTexture2D* ColorPicker::colorTexture()
	{
		if(m_textureRenderTarget) 
			return m_textureRenderTarget->colorTexture();
		return nullptr;
	}

	void ColorPicker::setTextureRenderTarget(TextureRenderTarget* textureRenderTarget)
	{
		//qDebug() << "ColorPicker::setTextureRenderTarget " << m_textureRenderTarget;
		m_textureRenderTarget = textureRenderTarget;
		if(m_textureRenderTarget)
			m_renderTargetSelector->setTarget(m_textureRenderTarget);
	}

	void ColorPicker::useSelfTarget()
	{
		if (!m_textureRenderTarget)
		{
			setTextureRenderTarget(new qtuser_3d::TextureRenderTarget(m_renderTargetSelector));
		}
	}
	
	void ColorPicker::use()
	{
		m_unUseTimer.stop();
		if (m_filterKey->name() == "pick")
			return;
		//set the filterName same as the  "pick" renderPass of the modelEffect
		m_filterKey->setName("pick");
	}

	void ColorPicker::unUse()
	{
		//set the filterName different from the  "pick" renderPass of the modelEffect
		if (m_filterKey->name() == "-ignore-ColorPicker-")
			return;
			
		m_filterKey->setName("-ignore-ColorPicker-");
	}

	float ColorPicker::renderTargetRatio()
	{
		return m_renderTargetRatio;
	}
}
