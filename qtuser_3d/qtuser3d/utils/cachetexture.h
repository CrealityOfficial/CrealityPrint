#ifndef QTUSER_3D_CACHE_TEXTURE_202404222046_H
#define QTUSER_3D_CACHE_TEXTURE_202404222046_H

#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DCore/QNode>
#include <Qt3DRender/QTexture>

namespace qtuser_3d
{
	class QTUSER_3D_API CacheTexture : public Qt3DCore::QNode
	{
	public:


		CacheTexture(Qt3DCore::QNode* parent = nullptr);
		virtual ~CacheTexture();

		Qt3DRender::QTexture2D* getTexture(const QString &key);
		void setTexture(const QString& key, Qt3DRender::QTexture2D* tex);

	protected:
		QMap<QString, Qt3DRender::QTexture2D*> m_map;
	};
}
#endif // ! QTUSER_3D_CACHE_TEXTURE_202404222046_H
