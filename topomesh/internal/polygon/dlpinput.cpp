#include "dlpinput.h"

namespace topomesh
{
	DLPInput::DLPInput()
	{

	}

	DLPInput::~DLPInput()
	{
	}

	AABB3D DLPInput::box() const
	{
		AABB3D box;
		for (const MeshObjectPtr& ptr : Meshes)
			box.include(ptr->box());
		return box;
	}

	int DLPInput::meshCount() const
	{
		return (int)Meshes.size();
	}
}