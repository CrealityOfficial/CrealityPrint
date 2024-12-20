#include "qtuser3d/module/rawogl.h"
#include <QtGui/QOffscreenSurface>
#include <QtCore/QDebug>

namespace qtuser_3d
{
	RawOGL::RawOGL(QObject* parent)
		:QObject(parent)
		, m_renderContext(nullptr)
		, m_surface(nullptr)
	{
		m_renderContext = new QOpenGLContext(this);
		m_surface = new QOffscreenSurface(nullptr, this);
		m_surface->create();
	}

	RawOGL::~RawOGL()
	{
		qDebug() << "Raw OGL ~~ ";
	}

	void RawOGL::init(QOpenGLContext* context)
	{
		m_renderContext = context;
		m_surface->setFormat(context->format());

		initializeOpenGLFunctions();
	}

	QOpenGLContext* RawOGL::sharedContext()
	{
		return m_renderContext;
	}

	QOffscreenSurface* RawOGL::surface()
	{
		return m_surface;
	}
}