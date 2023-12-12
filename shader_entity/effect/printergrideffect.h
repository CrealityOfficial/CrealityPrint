#ifndef CXKERNEL_PRINTERGRIDEFFECT_1681899751633_H
#define CXKERNEL_PRINTERGRIDEFFECT_1681899751633_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xeffect.h"
#include <QtGui/QVector4D>
#include <Qt3DRender/QLineWidth>

namespace qtuser_3d
{
	class SHADER_ENTITY_API PrinterGridEffect : public qtuser_3d::XEffect
	{
	public:
		PrinterGridEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~PrinterGridEffect();

		void setLineWidth(float lineWidth);
		void setLineColor(const QVector4D& color);
		void setXShowColor(const QVector4D& color);
		void setYShowColor(const QVector4D& color);
		void setXYIndexColor(const QVector4D& color);
		void setXYIndex(float xIndex, float yIndex);
	protected:
		Qt3DRender::QParameter* m_lineColorParam;
		Qt3DRender::QParameter* m_xshowColorParam;
		Qt3DRender::QParameter* m_yshowColorParam;

		Qt3DRender::QParameter* m_xIndexParam;
		Qt3DRender::QParameter* m_yIndexParam;
		Qt3DRender::QParameter* m_yIndexColorParam;

		Qt3DRender::QLineWidth* m_lineWidth;
	};
}

#endif // CXKERNEL_PRINTERGRIDEFFECT_1681899751633_H