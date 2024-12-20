#include "shareddataid.h"

namespace creative_kernel
{
	RenderDataID::RenderDataID()
	{
		meshId = colorsId = -1;
		materialId = 0;
	}

	RenderDataID::RenderDataID(const RenderDataID& other)
	{
		this->meshId = other.meshId;
		this->colorsId = other.colorsId;
		this->materialId = other.materialId;
		if (this->materialId < 0)
		{
			this->materialId = 0;
		}
	}

	RenderDataID::RenderDataID(int meshId, int colorsId, int materialId) :
		meshId(meshId),
		colorsId(colorsId),
		materialId(materialId)
	{
	}

	RenderDataID::~RenderDataID()
	{

	}

	RenderDataID& RenderDataID::operator=(const RenderDataID& other) 
	{
    	if (this != &other) 
		{
			meshId = other.meshId;
			colorsId = other.colorsId;
			materialId = other.materialId;
			if (materialId < 0)
			{
				materialId = 0;
			}
		}
		return *this;
	}

	bool RenderDataID::operator==(const RenderDataID& other) const
	{
		return meshId == other.meshId &&
					colorsId == other.colorsId &&
					materialId == other.materialId;
	}

	bool RenderDataID::operator!=(const RenderDataID& other) const
	{
		return meshId != other.meshId ||
					colorsId != other.colorsId ||
					materialId != other.materialId;
	}

	bool RenderDataID::operator<(const RenderDataID& other) const
	{
		if (this->meshId < other.meshId) 
			return true;
		if (this->colorsId < other.colorsId) 
			return true;
		if (this->materialId < other.materialId) 
			return true;
		return false;
	}

	SharedDataID::SharedDataID()
	{
		seamsId = supportsId = -1;
		modelType = (int)ModelNType::normal_part;
	}

	SharedDataID::SharedDataID(int meshId, int colorsId, int seamsId, int supportsId, int materialId) :
		seamsId(seamsId),
		supportsId(supportsId)
	{
		renderDataId = RenderDataID(meshId, colorsId, materialId);
		modelType = (int)ModelNType::normal_part;
	}

	SharedDataID::SharedDataID(RenderDataID renderDataId, int seamsId, int supportsId) :
		renderDataId(renderDataId),
		seamsId(seamsId),
		supportsId(supportsId)
	{
		modelType = (int)ModelNType::normal_part;
	}

	SharedDataID::~SharedDataID()
	{

	}

	SharedDataID& SharedDataID::operator=(const SharedDataID& other)
	{
		this->renderDataId = other.renderDataId;
		this->seamsId = other.seamsId;
		this->supportsId = other.supportsId;
		this->modelType = other.modelType;
		return *this;
	}

	bool SharedDataID::operator==(const SharedDataID& other) const
	{
		return renderDataId == other.renderDataId &&
					seamsId == other.seamsId &&
					supportsId == other.supportsId &&
					modelType == other.modelType;
	}

	bool SharedDataID::operator!=(const SharedDataID& other) const
	{
		return renderDataId != other.renderDataId ||
					seamsId != other.seamsId ||
					supportsId != other.supportsId ||
					modelType != other.modelType;
	}

	int& SharedDataID::operator()(int type)
	{
		if (type == MeshID)
		{
			return renderDataId.meshId;
		}
		else if (type == ColorsID)
		{
			return renderDataId.colorsId;
		}
		else if (type == MaterialID)
		{
			return renderDataId.materialId;
		}
		else if (type == SeamsID)
		{
			return seamsId;
		}
		else if (type == SupportsID)
		{
			return supportsId;
		}
		else if (type == ModelType)
		{
			return modelType;
		}
		else 
		{
			return renderDataId.meshId;
		}
	}
}