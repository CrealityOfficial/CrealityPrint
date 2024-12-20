#include "rectlineentity.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"

namespace qtuser_3d {

	RectLineEntity::RectLineEntity(Qt3DCore::QNode* parent)
		:qtuser_3d::LineExEntity(parent)
		, m_camera(nullptr)
	{
		setLineWidth(2);
	}

	RectLineEntity::~RectLineEntity()
	{
	}

	void RectLineEntity::setCamera(ScreenCamera* camera)
	{
		m_camera = camera;
	}

	void RectLineEntity::setRectOfScreen(const QRect& rect)
	{
		if (m_camera == nullptr)
		{
			qDebug() <<"camera must be set before";
			return;
		}

		m_rect = rect;

		QPoint origin = rect.topLeft();
		QSize size = rect.size();

		QVector<QVector3D> positions;

		positions.push_back(makeWorldPositionFromScreen(m_camera, origin));
		positions.push_back(makeWorldPositionFromScreen(m_camera, origin+QPoint(size.width(), 0.0)));
		positions.push_back(makeWorldPositionFromScreen(m_camera, rect.bottomRight()));
		positions.push_back(makeWorldPositionFromScreen(m_camera, origin+QPoint(0.0, size.height())));
		
		updateGeometry(positions, true);
	}


	QVector3D RectLineEntity::makeWorldPositionFromScreen(ScreenCamera* screenCamera, const QPoint& pos)
	{
		//qtuser_3d::ScreenCamera* screenCamera = visCamera();
		QSize size = screenCamera->size();
		QMatrix4x4 p = screenCamera->projectionMatrix();
		QMatrix4x4 v = screenCamera->viewMatrix();

		int x = pos.x();
		int y = pos.y();

		float X = float(x) / size.width() * 2.0 - 1.0;
		float Y = (1.0 - float(y) / size.height()) * 2.0 - 1.0;

		QVector4D ndcP(X, Y, 0.0, 1.0);

		QMatrix4x4 inv_vp = (p * v).inverted();

		QVector4D worldP = inv_vp * ndcP;
		worldP /= worldP.w();
		return QVector3D(worldP);
	}
}