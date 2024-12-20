#ifndef TOPOMESH_UTILS_1695108550958_H
#define TOPOMESH_UTILS_1695108550958_H
#include "topomesh/interface/idata.h"

namespace topomesh
{
	class PolygonRef;

	struct ColumnarParam
	{
		float zStart = 0.0f;
		float zEnd = 10.0f;
	};

	TOPOMESH_API trimesh::TriMesh* generateColumnar(const TriPolygons& polys, const ColumnarParam& param, const std::vector<std::vector<float>>* height = nullptr);

	TOPOMESH_API TriPolygons GetOpenMeshBoundarys(trimesh::TriMesh* triMesh);

	TOPOMESH_API void findNeignborFacesOfSameAsNormal(trimesh::TriMesh* trimesh, int indicate, float angle_threshold,
									/*out*/ std::vector<int>& faceIndexs);

	TOPOMESH_API void findBoundary(trimesh::TriMesh* trimesh);
	TOPOMESH_API void triangulate(trimesh::TriMesh* trimesh,std::vector<int>& sequentials);


    //////////////////////////////////////////////////////////////////////////
    //utilities for collision checking

    typedef trimesh::dbox2 bounding_box_t;
    typedef std::vector<trimesh::dvec2> contour_2d_t;

    //0: define data
    typedef struct collision_check_info
    {
        size_t model_group_index;
        topomesh::bounding_box_t bounding_box;
        topomesh::contour_2d_t contour;
        bool intersected = false;
        trimesh::dvec2 trans;
    }collision_check_info_t;

    TOPOMESH_API bool contour_is_cw(const contour_2d_t& contour);
    TOPOMESH_API bool contour_is_intersect(const contour_2d_t& contour0, const contour_2d_t& contour1);
    TOPOMESH_API bool offset_contour(double offset_distance, const contour_2d_t& in, contour_2d_t& out);

    TOPOMESH_API bool contour_to_svg(const std::vector<collision_check_info_t>& contours, const std::string& filename);

// 	TOPOMESH_API class Convex_Contour : public PolygonRef
// 	{
// 	public:
// 		Convex_Contour(const std::vector<trimesh::dvec3> &external_contour);
// 
// 		~Convex_Contour()
// 		{
// 		}
// 	};



    //////////////////////////////////////////////////////////////////////////
}

#endif // TOPOMESH_UTILS_1695108550958_H