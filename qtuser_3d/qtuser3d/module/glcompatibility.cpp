
#include <QDebug>
#include "glcompatibility.h"

#include <QtCore/QDebug>
#include <QtGui/QSurfaceFormat>
#include <QtCore/QProcess>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

namespace qtuser_3d
{
	bool isGles()
	{
        return QCoreApplication::testAttribute(Qt::AA_UseOpenGLES);
	}

	bool isSoftwareGL()
	{
		return QCoreApplication::testAttribute(Qt::AA_UseSoftwareOpenGL);
	}

    bool checkMRTSupported()
    {
        if (!QOpenGLContext::currentContext()->functions()->hasOpenGLFeature(QOpenGLFunctions::MultipleRenderTargets)) {
            qWarning("Multiple render targets not supported");
            return false;
        }

        int maxColorAttachments = 0;
        QOpenGLContext::currentContext()->functions()->glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
        qInfo() << "GL_MAX_COLOR_ATTACHMENTS = " << maxColorAttachments;
        if (maxColorAttachments < 8)
        {
            qWarning("GL_MAX_COLOR_ATTACHMENTS  < 8, NOT support!!!");
            return false;
        }

        GLint maxDrawBuf = 0;
        QOpenGLContext::currentContext()->functions()->glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuf);
        qInfo() << "GL_MAX_DRAW_BUFFERS = " << maxDrawBuf;
        if (maxDrawBuf < 8)
        {
            qWarning("GL_MAX_DRAW_BUFFERS  < 8, NOT support!!!");
            return false;
        }

        return true;
    }

    void checkVendor()
    {
        qDebug() << QString("Qt3D Scene3D Running on a %1 from %2").arg((const char*)glGetString(GL_RENDERER))
            .arg((const char*)glGetString(GL_VENDOR));
    }
}