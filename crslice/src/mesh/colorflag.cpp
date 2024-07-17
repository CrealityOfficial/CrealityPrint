#include "colorflag.h"

namespace crslice
{
    void getColors(trimesh::TriMesh* mesh, std::set<int>& flags, ccglobal::Tracer* m_progress)
    {
        if (mesh)
        {
            for (auto& num : mesh->flags)
            {
                flags.insert(num);
            }
        }
    }
}





