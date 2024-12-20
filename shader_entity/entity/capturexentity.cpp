#include "capturexentity.h"
#include "effect/modelsimpleeffect.h"

namespace qtuser_3d
{
	CaptureEntity::CaptureEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		setEffect(new ModelSimpleEffect());
	}

	CaptureEntity::~CaptureEntity()
	{

	}

	void CaptureEntity::onCaptureComplete()
	{
		//这里参数用 true 的话，会影响载入模型时的模型显示; 用 false 的话, 由于设置的 geometry 的 parent 不为空， 所以不会造成内存泄漏
		setGeometry(nullptr, Qt3DRender::QGeometryRenderer::Triangles, false);
	}

}