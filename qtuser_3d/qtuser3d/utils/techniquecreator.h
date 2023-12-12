#ifndef QTUSER_3D_TECHNIQUECREATOR_1594858991367_H
#define QTUSER_3D_TECHNIQUECREATOR_1594858991367_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QGraphicsApiFilter>

namespace qtuser_3d
{
	class QTUSER_3D_API TechniqueCreator : public QObject
	{
		Q_OBJECT
	public:
		TechniqueCreator(QObject* parent = nullptr);
		virtual ~TechniqueCreator();

		static Qt3DRender::QTechnique* createOpenGLBase(Qt3DCore::QNode* parent = nullptr);
	};
}
#endif // QTUSER_3D_TECHNIQUECREATOR_1594858991367_H