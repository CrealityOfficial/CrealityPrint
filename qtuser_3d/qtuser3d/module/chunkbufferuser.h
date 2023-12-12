#ifndef QTUSER_3D_CHUNKBUFFERUSER_1595830297480_H
#define QTUSER_3D_CHUNKBUFFERUSER_1595830297480_H
#include "qtuser3d/qtuser3dexport.h"
#include "qtuser3d/module/dlpchunkbufferprovider.h"

namespace qtuser_3d
{
	class ChunkBuffer;
	class QTUSER_3D_API ChunkBufferUser 
	{
	public:
		ChunkBufferUser();
		virtual ~ChunkBufferUser();

		void createBytes(QByteArray* positionBytes, QByteArray* flagBytes);
		void setChunk(qtuser_3d::ChunkBuffer* buffer, int chunk);
		int chunk();

		void setState(int state);
		int state();
		void updateState(int state);
		bool check(int faceID);
		int relativeFaceID(int faceID);

		bool tracked();
		void setTrack(bool track);

		void updatePositionOnly();
		void updateAll();
		void updateFlag();
		void updateChunkBuffer(QByteArray positionBytes, QByteArray flagsBytes);

		int validSize();
		void fillBuffer(float* buffer);

		qtuser_3d::ChunkBuffer* buffer();

		virtual DLPUserType dlpUserType(); // only for dlp
	protected:
		virtual int updatePosition(QByteArray& data) = 0;
		virtual void updateBuffer(float* buffer) = 0;  // valid size
	protected:
		int m_state;
		int m_validSize;

		qtuser_3d::ChunkBuffer* m_buffer;
		int m_chunk;

		int m_vertexNum;

		bool m_track;
	};
}
#endif // QTUSER_3D_CHUNKBUFFERUSER_1595830297480_H