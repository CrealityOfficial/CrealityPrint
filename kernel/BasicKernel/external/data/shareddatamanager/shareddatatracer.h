#ifndef CREATIVE_KERNEL_SHAREDDATATRACER_1681019989200_H
#define CREATIVE_KERNEL_SHAREDDATATRACER_1681019989200_H
#include "data/header.h"
#include "data/kernelenum.h"
#include <QHash>
#include <Qt3DCore/qnode.h>
#include <Qt3DRender/qgeometry.h>
#include "cxkernel/data/modelndata.h"
#include "shareddataid.h"
#include "renderdata.h"

namespace cxkernel
{
	class MeshData;
};

namespace creative_kernel
{
	// typedef std::shared_ptr<cxkernel::MeshData> cxkernel::MeshDataPtr;
	// class MeshDataWrapper;
	// class SpreadDataWrapper;
	// class IdGenerator;

	class SharedDataManager;

	class BASIC_KERNEL_API SharedDataTracer
	{
	public:
		SharedDataTracer();
		~SharedDataTracer();

		SharedDataID sharedDataID();
		void setSharedDataID(SharedDataID id);

		int meshID();
		void setMeshID(int meshID);

		int colorsID();
		void setColorsID(int colorsID);

		int seamsID();
		void setSeamsID(int seamsID);

		int supportsID();
		void setSupportsID(int supportsID);

		int materialID();
		void setMaterialID(int materialID);

		ModelNType modelType();
		void setModelType(ModelNType type);
		void setModelType(int type);

		RenderDataID renderDataID();
		void setRenderDataID(RenderDataID renderDataID);

		/* update render */
		virtual void beginUpdateRender();
		virtual void updateRender();
		virtual void endUpdateRender();
		
		/* loss material */
		virtual void onDiscardMaterial(int materialIndex);

	private:
		SharedDataID m_id;
		SharedDataManager* m_manager;

	};

}

#endif // CREATIVE_KERNEL_SHAREDDATATRACER_1681019989200_H