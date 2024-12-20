#include "openglcontextsaver.h"
#include <QtCore/QDebug>

namespace qtuser_3d
{
	OpenGLContextSaver::OpenGLContextSaver(QOpenGLContext* context)
		: m_context(context)
		, m_surface(context ? static_cast<QOffscreenSurface*>(context->surface()) : nullptr)
	{

	}

	OpenGLContextSaver::~OpenGLContextSaver()
	{
		if (m_context)
			m_context->makeCurrent(m_surface);
	}

	QOpenGLContext* OpenGLContextSaver::context()
	{
		return m_context;
	}

	QOffscreenSurface* OpenGLContextSaver::surface()
	{
		return m_surface;
	}
}