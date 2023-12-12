#include "chunkgroupentity.h"

namespace qtuser_3d
{
	ChunkGroupEntity::ChunkGroupEntity(int chunkFaces, int chunks, Qt3DCore::QNode* parent)
		:XEntity(parent)
		, m_usedEffect(nullptr)
		, m_chunkFaces(chunkFaces)
		, m_chunks(chunks)
	{
	}

	ChunkGroupEntity::~ChunkGroupEntity()
	{
	}

	void ChunkGroupEntity::setFaceBase(QPoint& faceBase)
	{
		m_faceBase = faceBase;
		m_currentBase = m_faceBase;

		for (PickableChunkEntity* entity : m_entities)
			updateChunkEntityFaceBase(entity);
	}

	void ChunkGroupEntity::setUsedEffect(XEffect* effect)
	{
		m_usedEffect = effect;
	}

	void ChunkGroupEntity::updateChunkEntityFaceBase(PickableChunkEntity* entity)
	{
		if (!entity)
			return;
	
		entity->setFaceBase(m_currentBase);
		m_currentBase.setX(m_currentBase.x() + m_chunkFaces * m_chunks);
	}

	ChunkBuffer* ChunkGroupEntity::traitChunkBuffer(int faceID)
	{
		ChunkBuffer* buffer = nullptr;
		for (PickableChunkEntity* entity : m_entities)
		{
			if (entity->faceIDIn(faceID))
			{
				buffer = entity;
				break;
			}
		}
		return buffer;
	}

	ChunkBufferUser* ChunkGroupEntity::traitChunkBufferUser(int faceID)
	{
		ChunkBufferUser* bufferUser = nullptr;
		for (PickableChunkEntity* entity : m_entities)
		{
			bufferUser = entity->chunkUserFromFaceID(faceID);
			if (bufferUser)
			{
				break;
			}
		}
		
		return bufferUser;
	}

	ChunkBuffer* ChunkGroupEntity::freeChunk()
	{
		return freeEntity();
	}

	PickableChunkEntity* ChunkGroupEntity::freeEntity()
	{
		PickableChunkEntity* chunkEntity = nullptr;
		int size = m_entities.size();
		for (int i = 0; i < size; ++i)
		{
			PickableChunkEntity* tChunk = m_entities.at(i);
			if (!tChunk->full())
			{
				chunkEntity = tChunk;
				break;
			}
		}

		if (chunkEntity == nullptr)
		{
			chunkEntity = new PickableChunkEntity(nullptr);
			chunkEntity->setEffect(m_usedEffect);
			chunkEntity->setParent(this);
			chunkEntity->create(m_chunkFaces, m_chunks);
			m_entities.push_back(chunkEntity);

			updateChunkEntityFaceBase(chunkEntity);
		}
		return chunkEntity;
	}
}