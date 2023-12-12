#include "techniquecreator.h"
#include "qtuser3d/module/glcompatibility.h"
#include <QtGui/QOpenGLContext>

namespace qtuser_3d
{
	TechniqueCreator::TechniqueCreator(QObject* parent)
		:QObject(parent)
	{
	}
	
	TechniqueCreator::~TechniqueCreator()
	{
	}

	Qt3DRender::QTechnique* TechniqueCreator::createOpenGLBase(Qt3DCore::QNode* parent)
	{
		Qt3DRender::QTechnique* technique = new Qt3DRender::QTechnique(parent);

		if (qtuser_3d::isGles())
		{
			technique->graphicsApiFilter()->setApi(Qt3DRender::QGraphicsApiFilter::OpenGLES);
			technique->graphicsApiFilter()->setMajorVersion(2);
			technique->graphicsApiFilter()->setMinorVersion(0);
		}
		else
		{
			technique->graphicsApiFilter()->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
			technique->graphicsApiFilter()->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
			technique->graphicsApiFilter()->setMajorVersion(2);
			technique->graphicsApiFilter()->setMinorVersion(1);
		}
		return technique;
	}
}