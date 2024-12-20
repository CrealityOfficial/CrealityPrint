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
		//��������� true �Ļ�����Ӱ������ģ��ʱ��ģ����ʾ; �� false �Ļ�, �������õ� geometry �� parent ��Ϊ�գ� ���Բ�������ڴ�й©
		setGeometry(nullptr, Qt3DRender::QGeometryRenderer::Triangles, false);
	}

}