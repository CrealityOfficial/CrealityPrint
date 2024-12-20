#ifndef CREATIVE_KERNEL_SHAREDDATAID_1681019989200_H
#define CREATIVE_KERNEL_SHAREDDATAID_1681019989200_H
#include "data/header.h"
#include "data/kernelenum.h"

namespace creative_kernel
{
	enum SharedDataIDType
	{
		MeshID = 0,
		ColorsID,
		MaterialID,
		SeamsID,
		SupportsID,
		ModelType
	};

	class BASIC_KERNEL_API RenderDataID
	{
	public:
		RenderDataID();
		RenderDataID(const RenderDataID& other);
		RenderDataID(int meshId, int colorsId, int materialId);
		~RenderDataID();

		 RenderDataID& operator=(const RenderDataID& other);
		 bool operator==(const RenderDataID& other) const;
		 bool operator!=(const RenderDataID& other) const;
		//inline bool operator>(const RenderDataID& other) const;
		//inline bool operator>=(const RenderDataID& other) const;
		 bool operator<(const RenderDataID& other) const;
		//inline bool operator<=(const RenderDataID& other) const;

	public:
		int meshId;
		int colorsId;
		int materialId;
	};

	class BASIC_KERNEL_API SharedDataID
	{
	public:
		SharedDataID();
		SharedDataID(int meshId, int colorsId, int seamsId, int supportsId, int materialId);
		SharedDataID(RenderDataID renderDataId, int seamsId, int supportsId);
		~SharedDataID();

		SharedDataID& operator=(const SharedDataID& other);
		bool operator==(const SharedDataID& other) const;
		bool operator!=(const SharedDataID& other) const;
		int& operator()(int arg);

		RenderDataID renderDataId;
		int seamsId;
		int supportsId;
		int modelType;
	};

}

#endif // CREATIVE_KERNEL_SHAREDDATAID_1681019989200_H