
#include "cube.h"
#include <assert.h>
#include "BRepPrimAPI_MakeBox.hxx"
#include "cadcore/internal/util.h"


namespace cadcore 
{
    
    trimesh::TriMesh* create_cube(double l, double w, int h)
    {
        BRepPrimAPI_MakeBox maker(l, w, h);
        maker.Build();

        assert(maker.IsDone());

        const TopoDS_Shape& shape = maker.Shape();
        trimesh::TriMesh* mesh = Shape_Triangulation(shape, 0.1);
    

        return mesh;
    }

}; // namespace Slic3r
