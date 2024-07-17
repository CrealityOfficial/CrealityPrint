#ifndef CREATIVE_KERNEL_MODELN_202311081401_H
#define CREATIVE_KERNEL_MODELN_202311081401_H

#include "basickernelexport.h"
#include <Qt3DRender/QGeometry>
#include "cxkernel/data/modelndata.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API ModelNRenderData
	{
	public:
		ModelNRenderData(cxkernel::ModelNDataPtr data);
		virtual~ModelNRenderData();

		int checkMaterialChanged();
		void discardMaterial(int materialIndex);
		cxkernel::ModelNDataPtr data();
		Qt3DRender::QGeometry* geometry();

		void updateIndexRenderData();
		void updateRenderData();
		void updateRenderDataForced();

	protected:
		void createGeometry();
		void destroyGeometry();

	protected:
		cxkernel::ModelNDataPtr m_data;
		Qt3DRender::QGeometry* m_geometry;
	};

	typedef std::shared_ptr<ModelNRenderData> ModelNRenderDataPtr;
};

#endif //CREATIVE_KERNEL_MODELN_202311081401_H