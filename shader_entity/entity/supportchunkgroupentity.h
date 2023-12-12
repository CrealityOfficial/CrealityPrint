#ifndef QTUSER_3D_SUPPORTCHUNKGROUPENTITY_1595474434671_H
#define QTUSER_3D_SUPPORTCHUNKGROUPENTITY_1595474434671_H

#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"

namespace qtuser_3d
{
	class XEffect;
	class SupportChunkEntity;
	class SHADER_ENTITY_API SupportChunkGroupEntity : public XEntity
	{
		Q_OBJECT
	public:
		SupportChunkGroupEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~SupportChunkGroupEntity();

		void setProto(int chunkFaces, int chunks);
		SupportChunkEntity* freeChunkEntity();
		void clearAllChunk();

		void setFaceBase(QPoint& faceBase);
		void setUsedEffect(XEffect* effect);
	protected:
		int m_chunkFacesProto;
		int m_chunksProto;

		QPoint m_faceBase;
		QVector<SupportChunkEntity*> m_chunkEntities;

		XEffect* m_usedEffect;
	};
}
#endif // QTUSER_3D_SUPPORTCHUNKGROUPENTITY_1595474434671_H