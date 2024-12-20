#include "antialiasingentity.h"
#include "effect/antialiasingeffect.h"
#include "qtuser3d/utils/texturecreator.h"

namespace qtuser_3d
{
	AntiAliasingEntity::AntiAliasingEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
		, m_textureSize(nullptr)
		, m_textureId(nullptr)
		, m_antiFlag(nullptr)
	{
		setEffect(new AntiAliasingEffect());
	}

	AntiAliasingEntity::~AntiAliasingEntity()
	{

	}

	void AntiAliasingEntity::setTextureSize(const QVector2D& theSize)
	{
		if (!m_textureSize)
		{
			m_textureSize = setParameter("textureSize", QVector2D(256.0f, 256.0f));
		}

		m_textureSize->setValue(theSize);
	}

	void AntiAliasingEntity::setTextureId(quint16 texId)
	{
		if (!m_textureId)
		{
			m_textureId = setParameter("screenTexture", 0);
		}

		//QString textureName = QStringLiteral("D:\\Visualization\\testIImage111.jpg");

		//Qt3DRender::QTexture2D* t = qtuser_3d::createFromSource(QUrl::fromLocalFile(textureName));
		//setParameter("screenTexture", QVariant::fromValue(t));

		m_textureId->setValue(texId);
	}

	void AntiAliasingEntity::setShareTexture(Qt3DRender::QSharedGLTexture* shareTexture)
	{
		if (!m_textureId)
		{
			m_textureId = setParameter("screenTexture", 0);
		}

		QString textureName = QStringLiteral("D:\\Visualization\\testIImage111.jpg");
		Qt3DRender::QTexture2D* t = qtuser_3d::createFromSource(QUrl::fromLocalFile(textureName));
		setParameter("screenTexture", QVariant::fromValue(t));

		//GLuint textureId = shareTexture->textureId();
		//m_textureId->setValue(QVariant::fromValue(shareTexture));
		//m_textureId->setValue(shareTexture->textureId());
	}

	void AntiAliasingEntity::setAntiFlag(int iFlag)
	{
		if (!m_antiFlag)
		{
			m_antiFlag = setParameter("antiflag", 1);
		}

		m_antiFlag->setValue(iFlag);
	}

}