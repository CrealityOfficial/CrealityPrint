#ifndef TRIMESH_SELECT_H
#define TRIMESH_SELECT_H
#include "trimesh2/TriMesh.h"
#include "trimesh2/Vec.h"
#include <vector>

#include "ccglobal/tracer.h"

namespace crslice
{
    struct Cr30Param
    {
        bool belt_support_enable;
        float support_angle;
        float machine_width;
        float machine_depth;
        float beltOffsetX;
        float beltOffsetY;
        Cr30Param()
        {
            belt_support_enable = true;
            support_angle = 45.0f;
            machine_width = 0.0f;
            machine_depth = 0.0f;
            beltOffsetX = 0.0f;
            beltOffsetY = 0.0f;
        }
    };

    std::vector<trimesh::TriMesh*> sliceBelt(std::vector<trimesh::TriMesh*>& meshs, Cr30Param& cr30Param, ccglobal::Tracer* m_progress = nullptr);
}

#endif //