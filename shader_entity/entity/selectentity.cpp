#include "selectentity.h"
#include "qtuser3d/geometry/trianglescreatehelper.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"
#include "renderpass/purerenderpass.h"

namespace qtuser_3d
{
	SelectEntity::SelectEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		XRenderPass *renderPass = new PureRenderPass(this);
		renderPass->addFilterKeyMask("view", 0);

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(renderPass);

		setEffect(effect);

		m_pColorParam = setParameter("color", QVector4D(0.6f, 0.6f, 0.0f, 1.0f));
	}

	SelectEntity::~SelectEntity()
	{
	}

	void SelectEntity::updateData(const std::vector<QVector3D>& vertexData)
	{
		if (vertexData.size() > 0)
		{
			Qt3DRender::QGeometry* geometry = TrianglesCreateHelper::create((int)vertexData.size(), (float*)&vertexData.at(0), nullptr, nullptr, 0, nullptr);
			setGeometry(geometry);
		}
	}

	void SelectEntity::setColor(QVector4D color)
	{
		m_pColorParam->setValue(color);
	}

}