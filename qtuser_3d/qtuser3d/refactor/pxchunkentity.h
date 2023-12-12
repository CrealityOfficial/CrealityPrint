#ifndef QTUSER_3D_PICKABLE_X_CHUNKENTITY_1595378430410_H
#define QTUSER_3D_PICKABLE_X_CHUNKENTITY_1595378430410_H
#include "qtuser3d/refactor/pxentity.h"
#include "qtuser3d/module/chunkbuffer.h"
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QAttribute>
#include "qtuser3d/geometry/supportattribute.h"

namespace qtuser_3d
{
	class QTUSER_3D_API PickXChunkEntity : public PickXEntity
		, public ChunkBuffer
	{
		Q_OBJECT
	public:
		PickXChunkEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PickXChunkEntity();

		void create(int chunkFaces, int chunks);
		void getPositionNormal(int chunk, QVector3D* position, QVector3D* normal);

		void setFaceBase(QPoint faceBase);

		int freeChunk() override;
		bool full() override;
		bool checkFace(int chunk, int faceID) override;
		int relativeFaceID(int chunk, int faceID) override;
		int chunkFaces() override;
		void releaseChunk(int chunk) override;
		void updateChunk(int chunk, QByteArray* positionBytes, QByteArray* flagsBytes) override;
		void releaseAllChunks();
		void check(int faceID, Ray& ray, QVector3D& position, QVector3D& normal) override;
		void setChunkUser(int chunk, ChunkBufferUser* user) override;
		ChunkBufferUser* chunkUser(int chunk) override;
		bool faceIDIn(int faceID);
		ChunkBufferUser* chunkUserFromFaceID(int faceID);

		int allFaces();

		int chunkIndex(int chunkID);
		
	protected:
		Qt3DRender::QGeometry* m_geometry;

		Qt3DRender::QAttribute* m_positionAttribute;
		Qt3DRender::QAttribute* m_normalAttribute;
		Qt3DRender::QAttribute* m_flagAttribute;

		Qt3DRender::QBuffer* m_positionBuffer;
		Qt3DRender::QBuffer* m_normalBuffer;
		Qt3DRender::QBuffer* m_flagBuffer;

		QByteArray m_positionByteArray;
		QByteArray m_normalByteArray;
		QByteArray m_flagByteArray;

		QList<int> m_freeList;
		int m_chunkFaces;
		int m_chunkBytes;
		int m_chunks;

		QPoint m_faceRange;

		QVector<ChunkBufferUser*> m_users;
	};
}
#endif // QTUSER_3D_PICKABLE_X_CHUNKENTITY_1595378430410_H