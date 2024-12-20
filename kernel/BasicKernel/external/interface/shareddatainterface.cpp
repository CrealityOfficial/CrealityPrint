#include "shareddatainterface.h"
#include "kernel/kernel.h"
#include "external/data/shareddatamanager/shareddatamanager.h"

namespace creative_kernel
{
	SharedDataID registerModelData(ModelDataPtr modelData)
	{
		return getKernel()->sharedDataManager()->registerModelData(modelData);
	}

	int registerMeshData(cxkernel::MeshDataPtr meshData, bool reuse)
	{
		return getKernel()->sharedDataManager()->registerMeshData(meshData, reuse);
	}

	int registerColor(const std::vector<std::string>& data)
	{
		return getKernel()->sharedDataManager()->registerColor(data);
	}
	
	int registerSeam(const std::vector<std::string>& data)
	{
		return getKernel()->sharedDataManager()->registerSeam(data);
	}
	
	int registerSupport(const std::vector<std::string>& data)
	{
		return getKernel()->sharedDataManager()->registerSupport(data);
	}
	
	void coverMeshData(cxkernel::MeshDataPtr meshData, int meshID)
	{
		return getKernel()->sharedDataManager()->coverMeshData(meshData, meshID);
	}

	void clearSharedData()
	{
		return getKernel()->sharedDataManager()->clear();
	}

	cxkernel::MeshDataPtr getMeshData(int id)
	{
		return getKernel()->sharedDataManager()->getMeshData(id);
	}
	
	cxkernel::MeshDataPtr getMeshData(SharedDataID id)
	{
		return getKernel()->sharedDataManager()->getMeshData(id);
	}

	bool getMeshNeedRepair(int id)
	{
		return getKernel()->sharedDataManager()->getMeshNeedRepair(id);
	}

	bool getMeshNeedRepair(SharedDataID id)
	{
		return getKernel()->sharedDataManager()->getMeshNeedRepair(id);
	}
	
	std::vector<std::string> getColors(int id)
	{
		return getKernel()->sharedDataManager()->getColors(id);
	}
	
	std::vector<std::string> getColors(SharedDataID id)
	{
		return getKernel()->sharedDataManager()->getColors(id);
	}

	std::vector<std::string> getSeams(int id)
	{
		return getKernel()->sharedDataManager()->getSeams(id);
	}
	
	std::vector<std::string> getSeams(SharedDataID id)
	{
		return getKernel()->sharedDataManager()->getSeams(id);
	}
	
	std::vector<std::string> getSupports(int id)
	{
		return getKernel()->sharedDataManager()->getSupports(id);
	}
	
	std::vector<std::string> getSupports(SharedDataID id)
	{
		return getKernel()->sharedDataManager()->getSupports(id);
	}
	
	RenderDataPtr getRenderData(SharedDataID id)
	{
		return getKernel()->sharedDataManager()->getRenderData(id);
	}
	
	
}