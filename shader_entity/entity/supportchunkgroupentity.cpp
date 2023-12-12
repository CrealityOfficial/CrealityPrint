#include "supportchunkgroupentity.h"
#include "entity/supportchunkentity.h"

namespace qtuser_3d
{
	SupportChunkGroupEntity::SupportChunkGroupEntity(Qt3DCore::QNode* parent)
		:XEntity(parent)
		, m_chunkFacesProto(200)
		, m_chunksProto(10)
		, m_usedEffect(nullptr)
	{
	}
	
	SupportChunkGroupEntity::~SupportChunkGroupEntity()
	{
	}

	void SupportChunkGroupEntity::setProto(int chunkFaces, int chunks)
	{
		m_chunkFacesProto = chunkFaces;
		m_chunksProto = chunks;
	}

	SupportChunkEntity* SupportChunkGroupEntity::freeChunkEntity()
	{
		SupportChunkEntity* chunkEntity = nullptr;
		int size = m_chunkEntities.size();
		for (int i = 0; i < size; ++i)
		{
			SupportChunkEntity* tChunk = m_chunkEntities.at(i);
			if (!tChunk->full())
			{
				chunkEntity = tChunk;
				break;
			}
		}

		if (chunkEntity == nullptr)
		{
			chunkEntity = new SupportChunkEntity(nullptr);
			chunkEntity->setEffect(m_usedEffect);
			chunkEntity->setParent(this);
			chunkEntity->create(m_chunkFacesProto, m_chunksProto);
			int index = m_chunkEntities.size();
			chunkEntity->setFaceBase(m_faceBase + QPoint(m_chunkFacesProto * m_chunksProto * index, 0));
			m_chunkEntities.push_back(chunkEntity);
		}
		return chunkEntity;
	}

	void SupportChunkGroupEntity::clearAllChunk()
	{
		for (SupportChunkEntity* chunkEntity : m_chunkEntities)
			chunkEntity->releaseAllChunks();
	}

	void SupportChunkGroupEntity::setFaceBase(QPoint& faceBase)
	{
		m_faceBase = faceBase;
		for (SupportChunkEntity* chunkEntity : m_chunkEntities)
		{
			chunkEntity->setFaceBase(faceBase);
			faceBase.setX(faceBase.x() + m_chunkFacesProto * m_chunksProto);
		}
	}

	void SupportChunkGroupEntity::setUsedEffect(XEffect* effect)
	{
		m_usedEffect = effect;
	}
}