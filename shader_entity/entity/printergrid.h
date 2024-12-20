#ifndef _QTUSER_3D_PRINTERGRID_1590674912369_H
#define _QTUSER_3D_PRINTERGRID_1590674912369_H

#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/math/box3d.h"
#include <QVector4D>

namespace qtuser_3d
{
	class SHADER_ENTITY_API PrinterGrid: public XEntity
	{
	public:
		PrinterGrid(Qt3DCore::QNode* parent = nullptr, float lw = 1.0);
		virtual ~PrinterGrid();

		void updateBounding(Box3D& box, int createtype = 0);
		void setLineColor(QVector4D clr);
		void setGap(float gap);
	protected:
		QVector4D m_lineColor;
		float m_gap;
	};
}
#endif // _QTUSER_3D_PRINTERGRID_1590674912369_H
