#ifndef QTUSER_3D_CHUNKGROUPENTITY_1612248232078_H
#define QTUSER_3D_CHUNKGROUPENTITY_1612248232078_H
#include "shader_entity_export.h"
#include "qtuser3d/refactor/xentity.h"
#include "entity/pickablechunkentity.h"
#include <QtCore/QMap>

namespace qtuser_3d
{
	class SHADER_ENTITY_API ChunkGroupEntity : public XEntity
	{
		Q_OBJECT
	public:
		ChunkGroupEntity(int chunkFaces, int chunks, Qt3DCore::QNode* parent = nullptr);
		virtual ~ChunkGroupEntity();

		void setFaceBase(QPoint& faceBase);
		void setUsedEffect(XEffect* effect);
		ChunkBuffer* traitChunkBuffer(int faceID);
		ChunkBufferUser* traitChunkBufferUser(int faceID);

		ChunkBuffer* freeChunk();
	protected:
		PickableChunkEntity* freeEntity();
		void updateChunkEntityFaceBase(PickableChunkEntity* entity);
	protected:
		QPoint m_faceBase;
		QPoint m_currentBase;

		int m_chunkFaces;
		int m_chunks;

		XEffect* m_usedEffect;

		QVector<PickableChunkEntity*> m_entities;
	};
}

#endif // QTUSER_3D_CHUNKGROUPENTITY_1612248232078_H