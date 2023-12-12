#ifndef DLPCHUNKBUFFERPROVIDER_1597800461637_H
#define DLPCHUNKBUFFERPROVIDER_1597800461637_H

namespace qtuser_3d
{
	enum class DLPUserType
	{
		eDLPTop,
		eDLPMiddle,
		eDLPPlatformTrunk,
		eDLPModelTrunk,
		eDLPLink,
#ifdef DLP_ONLY
		eDLPSupportPos,
#endif
		eDLPNum
	};

	class ChunkBuffer;
	class ChunkBufferUser;
	class DLPChunkBufferProvider
	{
	public:
		virtual ~DLPChunkBufferProvider() {}

		virtual ChunkBuffer* freeSupportChunk(DLPUserType userType) = 0;
	};
}

#endif // DLPCHUNKBUFFERPROVIDER_1597800461637_H