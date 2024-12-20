#ifndef QTUSER_3D_PRINTERSKIRTENTITY_1594887310960_H
#define QTUSER_3D_PRINTERSKIRTENTITY_1594887310960_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/math/box3d.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API PrinterSkirtDecorEntity : public XEntity
	{
		Q_OBJECT
	public:
		PrinterSkirtDecorEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PrinterSkirtDecorEntity();

		void updateBoundingBox(const Box3D& box);
		void setColor(const QVector4D& color);
	protected:
		Qt3DRender::QParameter* m_colorParameter;
	};


	class SHADER_ENTITY_API PrinterSkirtEntity : public XEntity
	{
		Q_OBJECT
	public:
		PrinterSkirtEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PrinterSkirtEntity();

		void updateBoundingBox(const Box3D& box);
		
		void setHighlight(bool highlight);

		void setInnerColor(const QVector4D& color);
		void setOuterColor(const QVector4D& color);
		void setVerticalBottomColor(const QVector4D& color);

	protected:
		PrinterSkirtDecorEntity *m_outerEntity;
		PrinterSkirtDecorEntity *m_verticalEntity;
		PrinterSkirtDecorEntity* m_innerHighlightEntity;
	};
}
#endif // QTUSER_3D_PRINTERSKIRTENTITY_1594887310960_H