#ifndef QTUSER_3D_AXISENTITY_1595053218074_H
#define QTUSER_3D_AXISENTITY_1595053218074_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"

namespace qtuser_3d
{
	class TextMeshEntity;
	class PureColorEntity;

	class SHADER_ENTITY_API AxisEntity : public XEntity
	{
		Q_OBJECT
	public:
		AxisEntity(Qt3DCore::QNode* parent = nullptr, int axistype = 0, QVector3D* s_use = nullptr);
		virtual ~AxisEntity();

		void translate(QVector3D v);

		void setXAxisColor(const QVector4D& color);
		void setYAxisColor(const QVector4D& color);
		void setZAxisColor(const QVector4D& color);

	protected:
		PureColorEntity* m_xAxis;
		PureColorEntity* m_yAxis;
		PureColorEntity* m_zAxis;

		TextMeshEntity* m_xText;
		TextMeshEntity* m_yText;
		TextMeshEntity* m_zText;
	};
}
#endif // QTUSER_3D_AXISENTITY_1595053218074_H