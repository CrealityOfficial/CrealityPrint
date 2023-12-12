#ifndef QTUSER_3D_SELECT_ENTITY_H
#define QTUSER_3D_SELECT_ENTITY_H

#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include <QtGui/QVector3D>
#include <vector>

namespace qtuser_3d
{
	
	class SHADER_ENTITY_API SelectEntity : public XEntity
	{
		Q_OBJECT
	public:
		SelectEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~SelectEntity();

		void updateData(const std::vector<QVector3D>& vertexData);
		void setColor(QVector4D color);
	private:
		Qt3DRender::QParameter* m_pColorParam;
	};
}
#endif // QTUSER_3D_BASICENTITY_1594569444448_H