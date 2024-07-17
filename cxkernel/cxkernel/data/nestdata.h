#ifndef QCXUTIL_NESTDATA_1685692131729_H
#define QCXUTIL_NESTDATA_1685692131729_H
#include "cxkernel/data/header.h"

namespace cxkernel
{
    class CXKERNEL_API NestData
    {
    public:
        NestData();
        virtual ~NestData();

        void copyData(const NestData* nd);

        void setDirty(bool dirty);
        bool dirty();

        std::vector<trimesh::vec3> path(TriMeshPtr hull, const trimesh::vec3& scale = trimesh::vec3(1.0f, 1.0f, 1.0f), bool simple = false);
        std::vector<trimesh::vec3> qPath(TriMeshPtr hull, const trimesh::quaternion& rotation, const trimesh::vec3& scale = trimesh::vec3(1.0f, 1.0f, 1.0f), bool simple = false);

        const std::vector<trimesh::vec3>& cPath(bool simple = false);
        std::vector<trimesh::vec3> debug_path();
        std::vector<trimesh::vec3> concave_path(TriMeshPtr mesh, const trimesh::vec3 scale);
        std::vector<trimesh::vec3> q_concave_path(TriMeshPtr mesh, const trimesh::quaternion& _rotation, const trimesh::vec3& scale);

        void setNestRotation(const trimesh::quaternion& rotation);
        trimesh::quaternion nestRotation();
    protected:
        void calculateXYConvex(TriMeshPtr hull, const trimesh::fxform& rxf = trimesh::fxform::identity(),
            const trimesh::vec3& scale = trimesh::vec3(1.0f, 1.0f, 1.0f));

        std::vector<trimesh::vec3> calculateGlobalXYConvex(TriMeshPtr hull, const trimesh::fxform& rxf = trimesh::fxform::identity(),
            const trimesh::vec3& scale = trimesh::vec3(1.0f, 1.0f, 1.0f));
    protected:
        TriMeshPtr convex;
        std::vector<trimesh::vec3> simples;
        trimesh::quaternion rotation;
  
        bool m_dirty;
    };

    typedef std::shared_ptr<NestData> NestDataPtr;

    CXKERNEL_API void calculateConvex(TriMeshPtr mesh, std::vector<trimesh::vec3>& convex);
}

#endif // QCXUTIL_NESTDATA_1685692131729_H