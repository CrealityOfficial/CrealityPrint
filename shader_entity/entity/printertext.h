#ifndef _QTUSER_3D_PRINTERTEXT_1590739546472_H
#define _QTUSER_3D_PRINTERTEXT_1590739546472_H
#include "shader_entity_export.h"
#include <Qt3DCore/QEntity>
#include "qtuser3d/math/box3d.h"

namespace qtuser_3d
{
	class TextMeshEntity;
	class SHADER_ENTITY_API PrinterText: public Qt3DCore::QEntity
	{
	public:
		PrinterText(Qt3DCore::QNode* parent = nullptr);
		virtual ~PrinterText();

		void updateLen(Box3D& box, float gap, int major);
		void updateScaleMark(Box3D& box, float gap, float fontSize);
	protected:
		QList<TextMeshEntity*> m_majors;
		QList<TextMeshEntity*> m_minors;
	};
}
#endif // _QTUSER_3D_PRINTERTEXT_1590739546472_H
