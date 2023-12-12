#include "finephongeffect.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "renderpass/phongrenderpass.h"

namespace qtuser_3d
{
	FinePhongEffect::FinePhongEffect(Qt3DCore::QNode* parent)
		: XEffect(parent)
	{
		addRenderPass(new qtuser_3d::PhongRenderPass());
		addPassFilter(0, "view");
	}

	FinePhongEffect::~FinePhongEffect()
	{

	}

}