#ifndef _QTUSER_3D_TEXTMESHENTITY_1590738216492_H
#define _QTUSER_3D_TEXTMESHENTITY_1590738216492_H

#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include <Qt3DExtras/QExtrudedTextMesh>

namespace qtuser_3d
{
	class SHADER_ENTITY_API TextMeshEntity: public XEntity
	{
	public:
		TextMeshEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~TextMeshEntity();

		void setFont(const QString& family);
		void setText(const QString& text);
		void setColor(const QVector4D& color);
	protected:
		Qt3DExtras::QExtrudedTextMesh* m_textMesh;
		Qt3DRender::QParameter* m_colorParameter;
	};
}
#endif // _QTUSER_3D_TEXTMESHENTITY_1590738216492_H
