#ifndef CREATIVE_KERNEL_LINEEX_ENTITY_1592195157165_H
#define CREATIVE_KERNEL_LINEEX_ENTITY_1592195157165_H
#include "shader_entity_export.h"
#include "lineentity.h"
#include <QVector3D>

namespace qtuser_3d
{
	class SHADER_ENTITY_API LineExEntity : public LineEntity
	{
		Q_OBJECT
	public:
		enum LINE_TYPE
		{
			NORMAL_LINE = 1,
			DOT_LINE = 2,
			PIN_ARROW_LINE = 0x11,
			PIN_ARROW_DOT_LINE = 0x12,
		};

	public:

		LineExEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~LineExEntity();

		int setPoint(const QVector3D& start_pt, const QVector3D& end_pt, const QVector3D& normal);

		int genGeometry();

		int changeLineStype(LINE_TYPE line_type);

		void setDotLineLen(int dot_line_len);
		void setDotLineSpace(int dot_line_space);
		void setArrowLen(int arrow_len);
		void setArrowAngle(float arrow_angle);

	private:
		int genStorageData();

	protected:
		QVector3D m_startPoint;
		QVector3D m_endPoint;

		QVector3D m_normal;

	protected:
		QVector<QVector3D> m_storageData;

	private:
		// 内部所用参数
		LINE_TYPE m_lineType;

		int m_dot_line_len;
		int m_dot_line_space;

		int m_arrow_len;
		float m_arrow_angle;
	};
}
#endif // CREATIVE_KERNEL_BALL_ENTITY_1592195157165_H