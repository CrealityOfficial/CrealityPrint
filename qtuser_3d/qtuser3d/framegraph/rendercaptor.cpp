#include "qtuser3d/framegraph/rendercaptor.h"

namespace qtuser_3d
{
	RenderCaptor::RenderCaptor(Qt3DCore::QNode* parent)
	{
		m_cameraSelector = new Qt3DRender::QCameraSelector(parent);
		m_renderCapture = new Qt3DRender::QRenderCapture(m_cameraSelector);
	}

	RenderCaptor::~RenderCaptor()
	{
		m_cameraSelector->deleteLater();
		m_renderCapture->deleteLater();
	}

	void RenderCaptor::capture(Qt3DRender::QCamera* camera, QObject* receiver, ReceiverHandleReplyFunc func)
	{
		m_receiver = receiver;
		m_callback = func;
		m_camera.reset(camera);

		connect(receiver, &QObject::destroyed, this, [=]
		{
			m_receiver = NULL;
		});

		m_cameraSelector->setCamera(camera);
		m_reply.reset(m_renderCapture->requestCapture());
		connect(m_reply.data(), &Qt3DRender::QRenderCaptureReply::completed, this, &RenderCaptor::onCaptureCompleted);
	}

	void RenderCaptor::setEnabled(bool enabled)
	{
		m_cameraSelector->setEnabled(enabled);

		if (!enabled)
			deleteLater();
	}

	void RenderCaptor::onCaptureCompleted()
	{
		if (!m_cameraSelector->isEnabled())
			return;

		if (m_callback && m_receiver)
		{
			m_callback(m_receiver, m_reply->image());
		}

		if (m_disposable)
		{
			deleteLater();
		}

	}

}
