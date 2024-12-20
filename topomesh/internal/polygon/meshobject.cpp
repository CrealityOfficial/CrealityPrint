#include "meshobject.h"

namespace topomesh
{
	MeshObject::MeshObject()
	{

	}

	MeshObject::~MeshObject()
	{

	}

	AABB3D MeshObject::box()
	{
		return m_aabb;
	}

	void MeshObject::calculateBox()
	{
		for (MeshVertex& v : vertices)
			m_aabb.include(v.p);
	}

	void MeshObject::clear()
	{
		vertices.clear();
		faces.clear();
	}
}