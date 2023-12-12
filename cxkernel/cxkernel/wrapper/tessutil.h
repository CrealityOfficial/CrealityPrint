#ifndef _TESS_UTIL_H_
#define _TESS_UTIL_H_
#include "cxkernel/cxkernelinterface.h"
#include "trimesh2/Vec.h"
#include <vector>

namespace cxkernel
{
    CXKERNEL_API void tessPolygon(const std::vector<std::vector<trimesh::vec2>>& polygons, std::vector<trimesh::vec3>& vertexs);
}
#endif // _TESS_UTIL_H_