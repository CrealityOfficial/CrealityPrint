#include "cylinderentity.h"
#include "effect/finephongeffect.h"

namespace qtuser_3d
{
	CylinderEntity::CylinderEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		setEffect(new FinePhongEffect());

		setParameter("color", QVector4D(0.5f, 0.0f, 0.0f, 1.0f));
	}

	CylinderEntity::~CylinderEntity()
	{

	}

}