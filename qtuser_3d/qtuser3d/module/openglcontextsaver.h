#ifndef QTUSER_CORE_OPENGLCONTEXTSAVER_1651303374025_H
#define QTUSER_CORE_OPENGLCONTEXTSAVER_1651303374025_H
#include "qtuser3d/qtuser3dexport.h"
#include <QtGui/QOpenGLContext>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLExtraFunctions>

namespace qtuser_3d
{
	class QTUSER_3D_API OpenGLContextSaver
	{
	public:
		OpenGLContextSaver(QOpenGLContext* context = QOpenGLContext::currentContext());
		~OpenGLContextSaver();

		QOpenGLContext* context();
		QOffscreenSurface* surface();
	private:
		QOpenGLContext* m_context;
		QOffscreenSurface* m_surface;
	};
}

#endif // QTUSER_CORE_OPENGLCONTEXTSAVER_1651303374025_H