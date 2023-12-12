#ifndef QTUSER_3D_COLORRENDERPASS_1679990543678_H
#define QTUSER_3D_COLORRENDERPASS_1679990543678_H
#include <QtGui/QVector4D>
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xrenderpass.h"

namespace qtuser_3d
{
	class SHADER_ENTITY_API ColorRenderPass : public qtuser_3d::XRenderPass
	{
	public:
		ColorRenderPass(Qt3DCore::QNode* parent = nullptr);
		virtual ~ColorRenderPass();

	};
}

#endif // QTUSER_3D_COLORRENDERPASS_1679990543678_H