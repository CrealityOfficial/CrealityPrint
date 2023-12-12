#include "tessutil.h"
#include "Include/tesselator.h"
#include <cstring>

#include "ccglobal/profile.h"

namespace cxkernel
{
    void* stdAlloc(void* userData, unsigned int size)
    {
        int* allocated = (int*)userData;
        TESS_NOTUSED(userData);
        *allocated += (int)size;
        return malloc(size);
    }

    void stdFree(void* userData, void* ptr)
    {
        TESS_NOTUSED(userData);
        free(ptr);
    }

    void tessPolygon(const std::vector<std::vector<trimesh::vec2>>& polygons, std::vector<trimesh::vec3>& vertexs)
    {
        SYSTEM_TICK("total");

        int nvp = 3;
        int allocated = 0;
        TESSalloc ma;
        memset(&ma, 0x0, sizeof(ma));
        ma.memalloc = stdAlloc;
        ma.memfree = stdFree;
        ma.userData = (void*)&allocated;
        ma.extraVertices = 256; // realloc not provided, allow 256 extra vertices.
        TESStesselator* tess = tessNewTess(&ma);
        if (tess)
        {
            SYSTEM_TICK("prepare");
            tessSetOption(tess, TESS_CONSTRAINED_DELAUNAY_TRIANGULATION, 1);

            for (std::vector<trimesh::vec2> contour : polygons)
            {
                tessAddContour(tess, 2, (const void*)&contour.front(), sizeof(trimesh::vec2), contour.size());
            }
            SYSTEM_TICK("prepare");

            SYSTEM_TICK("tess");
            if (tessTesselate(tess, TESS_WINDING_POSITIVE, TESS_POLYGONS, nvp, 2, 0))
            {
                const float* verts = tessGetVertices(tess);
                const int* vinds = tessGetVertexIndices(tess);
                const int* elems = tessGetElements(tess);
                const int nverts = tessGetVertexCount(tess);
                const int nelems = tessGetElementCount(tess);

                for (size_t i = 0; i < nelems; ++i)
                {
                    const int* p = &elems[i * nvp];
                    for (size_t j = 0; j < nvp && p[j] != TESS_UNDEF; ++j)
                    {
                        vertexs.push_back(trimesh::vec3(verts[p[j] * 2], verts[p[j] * 2 + 1], 0.0));
                    }
                }
            }
            SYSTEM_TICK("tess");

            tessDeleteTess(tess);
        }

        SYSTEM_TICK("total");
    }
}