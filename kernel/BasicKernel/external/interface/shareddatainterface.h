#ifndef SHARED_DATA_INTERFACE_1592732397175_H
#define SHARED_DATA_INTERFACE_1592732397175_H
#include "basickernelexport.h"
#include "data/rawdata.h"
#include "data/shareddatamanager/renderdata.h"

namespace creative_kernel
{	
	BASICKERNEL_API SharedDataID registerModelData(ModelDataPtr modelData);
	BASIC_KERNEL_API int registerMeshData(cxkernel::MeshDataPtr meshData, bool reuse = true);
	BASIC_KERNEL_API int registerColor(const std::vector<std::string>& data);
	BASIC_KERNEL_API int registerSeam(const std::vector<std::string>& data);
	BASIC_KERNEL_API int registerSupport(const std::vector<std::string>& data);
	BASIC_KERNEL_API void coverMeshData(cxkernel::MeshDataPtr meshData, int meshID);

	BASIC_KERNEL_API void clearSharedData();

	BASIC_KERNEL_API cxkernel::MeshDataPtr getMeshData(int id);
	BASIC_KERNEL_API cxkernel::MeshDataPtr getMeshData(SharedDataID id);
	BASIC_KERNEL_API bool getMeshNeedRepair(int id);
	BASIC_KERNEL_API bool getMeshNeedRepair(SharedDataID id);

	BASIC_KERNEL_API std::vector<std::string> getColors(int id);
	BASIC_KERNEL_API std::vector<std::string> getColors(SharedDataID id);

	BASIC_KERNEL_API std::vector<std::string> getSeams(int id);
	BASIC_KERNEL_API std::vector<std::string> getSeams(SharedDataID id);

	BASIC_KERNEL_API std::vector<std::string> getSupports(int id);
	BASIC_KERNEL_API std::vector<std::string> getSupports(SharedDataID id);

	BASIC_KERNEL_API RenderDataPtr getRenderData(SharedDataID id);
	
}
#endif // SHARED_DATA_INTERFACE_1592732397175_H
