#ifndef CXKERNEL_FINEPHONGEFFECT_1681819876499_H
#define CXKERNEL_FINEPHONGEFFECT_1681819876499_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xeffect.h"
#include <QtGui/QVector4D>

namespace qtuser_3d
{
	class SHADER_ENTITY_API FinePhongEffect : public qtuser_3d::XEffect
	{
	public:
		FinePhongEffect(Qt3DCore::QNode* parent = nullptr);
		virtual ~FinePhongEffect();

	};
}

#endif // CXKERNEL_FINEPHONGEFFECT_1681819876499_H