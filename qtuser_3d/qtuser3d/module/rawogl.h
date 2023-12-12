#ifndef QTUSER_3D_RAWOGL_1602040546851_H
#define QTUSER_3D_RAWOGL_1602040546851_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtGui/QOpenGLExtraFunctions>

class QOffscreenSurface;
namespace qtuser_3d
{
	class QTUSER_3D_API RawOGL : public QObject , public QOpenGLExtraFunctions
	{
	public:
		RawOGL(QObject* parent = nullptr);
		virtual ~RawOGL();

		void init(QOpenGLContext* context);
		QOpenGLContext* sharedContext();
		QOffscreenSurface* surface();
	protected:
		QOpenGLContext* m_renderContext;
		QOffscreenSurface* m_surface;
	};
}

#endif // QTUSER_QML_RAWOGL_1602040546851_H