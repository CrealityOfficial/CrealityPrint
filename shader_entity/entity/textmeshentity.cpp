#include "textmeshentity.h"
#include "qtuser3d/refactor/xrenderpass.h"
#include "qtuser3d/refactor/xeffect.h"
#include "renderpass/purerenderpass.h"

namespace qtuser_3d
{
	TextMeshEntity::TextMeshEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
	{
		XRenderPass* pass = new PureRenderPass(this);
		pass->addFilterKeyMask("view", 0);
		XEffect* effect = new XEffect(this);
		effect->addRenderPass(pass);
		setEffect(effect);

		m_colorParameter = setParameter("color", QVector4D(1.0f, 0.0f, 0.0f, 1.0f));
		m_textMesh = new Qt3DExtras::QExtrudedTextMesh(this);
		QFont font("Arial", 64, -1, false);
		m_textMesh->setFont(font);
		m_textMesh->setDepth(0.01f);
		
		replaceGeometryRenderer(m_textMesh);
	}

	TextMeshEntity::~TextMeshEntity()
	{
	}

	void TextMeshEntity::setFont(const QString& family)
	{
		QFont font(family, 64, -1, false);
		m_textMesh->setFont(font);
	}

	void TextMeshEntity::setText(const QString& text)
	{
		m_textMesh->setText(text);
	}

	void TextMeshEntity::setColor(const QVector4D& color)
	{
		m_colorParameter->setValue(color);
	}
}
