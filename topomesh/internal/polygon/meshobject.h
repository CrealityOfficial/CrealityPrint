#ifndef CX_MESHOBJECT_1600055530996_H
#define CX_MESHOBJECT_1600055530996_H
#include <memory>
#include "vertex.h"

namespace topomesh
{
	class MeshObject
	{
	public:
		MeshObject();
		~MeshObject();

        std::vector<MeshVertex> vertices;
        std::vector<MeshFace> faces;

        AABB3D box();
        void calculateBox();
        void clear();
    protected:
        AABB3D m_aabb;
	};

	typedef std::shared_ptr<MeshObject> MeshObjectPtr;
}

#endif // CX_MESHOBJECT_1600055530996_H