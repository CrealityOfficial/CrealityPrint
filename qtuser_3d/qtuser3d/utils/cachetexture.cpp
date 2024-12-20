#include "cachetexture.h"
#include "textureload.h"
#include "texturecreator.h"

namespace qtuser_3d
{
	CacheTexture::CacheTexture(QNode* parent)
		:QNode(parent)
	{
	}

	CacheTexture::~CacheTexture()
	{
	}

	Qt3DRender::QTexture2D* CacheTexture::getTexture(const QString& key)
	{
		if (m_map.contains(key))
		{
			return m_map.value(key);
		}
		else
		{
			Qt3DRender::QTexture2D* newTexture = qtuser_3d::createFromSource(QUrl(key));
			newTexture->setParent(this);
			m_map.insert(key, newTexture);
			return newTexture;
		}

		return nullptr;
	}

	void CacheTexture::setTexture(const QString& key, Qt3DRender::QTexture2D* tex)
	{
		if (tex)
		{
			tex->setParent(this);
			m_map.insert(key, tex);
		}
	}
}