#ifndef TRIMESH_COLORFLAGS_H
#define TRIMESH_COLORFLAGS_H
#include "trimesh2/TriMesh.h"
#include <set>

#include "ccglobal/tracer.h"

namespace crslice
{
    void getColors(trimesh::TriMesh* mesh, std::set<int>& flags, ccglobal::Tracer* m_progress = nullptr);
}

#endif //TRIMESH_COLORFLAGS_H