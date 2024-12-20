#ifndef CHUNKBUFFER_1595811337905_H
#define CHUNKBUFFER_1595811337905_H
#include <QtCore/QByteArray>
#include "qtuser3d/math/ray.h"

namespace qtuser_3d
{
	class ChunkBufferUser;
	class ChunkBuffer
	{
	public:
		virtual ~ChunkBuffer() {}

		virtual bool full() = 0;
		virtual int freeChunk() = 0;
		virtual bool checkFace(int chunk, int faceID) = 0;
		virtual int chunkFaces() = 0;
		virtual int relativeFaceID(int chunk, int faceID) = 0;
		virtual void releaseChunk(int chunk) = 0;
		virtual void setChunkUser(int chunk, ChunkBufferUser* user) = 0;
		virtual ChunkBufferUser* chunkUser(int chunk) = 0;
		virtual void updateChunk(int chunk, QByteArray* positionArray, QByteArray* flagArray) = 0;
		virtual void check(int faceID, Ray& ray, QVector3D& position, QVector3D& normal) = 0;
	};
}
#endif // CHUNKBUFFER_1595811337905_H