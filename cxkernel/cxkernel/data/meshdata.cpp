#include "meshdata.h"
#include "cxkernel/data/trimeshutils.h"

#include "qhullWrapper/hull/meshconvex.h"
#include "qhullWrapper/hull/hullface.h"

#include "msbase/mesh/checker.h"
#include "msbase/mesh/dumplicate.h"
#include "msbase/mesh/tinymodify.h"
#include "msbase/mesh/merge.h"

#include "qtusercore/module/progressortracer.h"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QDataStream>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>

namespace cxkernel
{
	MeshData::MeshData(TriMeshPtr mesh, bool toCenter)
    {
		setMesh(mesh, toCenter);
    }

    MeshData::MeshData()
    {

    }

	void MeshData::setMesh(TriMeshPtr _mesh, bool toCenter)
	{
		mesh = _mesh;
        if (!mesh)
            return;

        mesh->normals.clear();
        bool processResult = msbase::dumplicateMesh(mesh.get());
        if (!processResult)
        {
            hull = mesh;
            return;
        }

		mesh->clear_bbox();
		mesh->need_bbox();

        if(toCenter)
            offset = msbase::moveTrimesh2Center(mesh.get(), false);

        trimesh::TriMesh* _hull = qhullWrapper::convex_hull_3d(mesh.get());
		msbase::dumplicateMesh(_hull);
		hull.reset(_hull);
	}

    trimesh::dbox3 MeshData::calculateBox(const trimesh::xform& matrix)
    {
        trimesh::dbox3 b;

        if (hull)
        {
            auto total_box = tbb::parallel_reduce(
                tbb::blocked_range<int>(0, hull->vertices.size()),
                trimesh::dbox3(),
                [&](tbb::blocked_range<int> r, trimesh::dbox3 totalBox)
                {
                    for (int i = r.begin(); i < r.end(); ++i)
                    {
                        totalBox += matrix * trimesh::dvec3(hull->vertices[i]);
                    }

                    return totalBox;
                }, std::plus<trimesh::dbox3>());

            b = total_box;

            //for (const trimesh::vec3& v : hull->vertices)
            //    b += matrix * trimesh::dvec3(v);

        }
        else if (mesh)
        {
            if (mesh->flags.size() == mesh->vertices.size())
            {
                for (int i : mesh->flags)
                    b += matrix * trimesh::dvec3(mesh->vertices.at(i));
            }
            else
            {
                int size = (int)mesh->vertices.size();
                for (int i = 0; i < size; ++i)
                {
                    trimesh::dvec3 v = (trimesh::dvec3)mesh->vertices.at(i);
                    b += matrix * v;
                }
            }
        }

        return b;
    }

    trimesh::dbox3 MeshData::localBox()
    {
        trimesh::dbox3 b;
        if (mesh)
        {
            b += trimesh::dvec3(mesh->bbox.min);
            b += trimesh::dvec3(mesh->bbox.max);
        }
        return b;
    }

    double MeshData::localZ()
    {
        trimesh::dbox3 b = localBox();
        return b.min.z - offset.z;
    }

    void MeshData::calculateFaces()
    {
        if (faces.size() == 0)
        {
            std::vector<qhullWrapper::HullFace> _faces;
            qhullWrapper::hullFacesFromConvexMesh(hull.get(), _faces);
            qhullWrapper::hullFacesFromMeshNear(mesh, _faces);
            int size = _faces.size();
            if (size > 0)
            {
                faces.resize(size);
                for (int i = 0; i < size; ++i)
                {
                    KernelHullFace& face = faces.at(i);
                    const qhullWrapper::HullFace& _face = _faces.at(i);
                    face.mesh = _face.mesh;
                    face.normal = _face.normal;
                    face.hullarea = _face.hullarea;
                }
            }
        }
    }

    void MeshData::resetHull()
    {
        if (!hull)
        {
            hull.reset(qhullWrapper::convex_hull_3d(mesh.get()));
        }
    }

    void MeshData::adaptSmallBox(const trimesh::dbox3& box)
    {
        if (!box.valid)
            return;

        trimesh::xform xf = trimesh::xform::scale(25.4);

        if (mesh)
            trimesh::apply_xform(mesh.get(), xf);
        if (hull)
            trimesh::apply_xform(hull.get(), xf);
    }

    void MeshData::adaptBigBox(const trimesh::dbox3& box)
    {
        trimesh::dbox3 _box = box;
        trimesh::dbox3 _b = calculateBox();

        if (!_box.valid)
            return;

        if (!_b.valid)
            return;

        trimesh::dvec3 bsize = 0.9 * _box.size();
        trimesh::dvec3 scale = bsize / _b.size();
        float s = scale.min();
        trimesh::xform xf = trimesh::xform::scale(s);

        if (mesh)
            trimesh::apply_xform(mesh.get(), xf);
        if (hull)
            trimesh::apply_xform(hull.get(), xf);
    }

    void MeshData::convex(const trimesh::xform& matrix, std::vector<trimesh::dvec3>& datas)
    {
        std::vector<trimesh::vec2> hullPoints2D;
        if (hull)
        {
            int size = (int)hull->vertices.size();
            hullPoints2D.reserve(size);
            for (const trimesh::vec3& v : hull->vertices)
            {
                trimesh::vec3 p = matrix * v;

                hullPoints2D.emplace_back(trimesh::vec2(p.x, p.y));
            }
        }
        else if (mesh->flags.size() > 0)
        {
            int size = (int)mesh->vertices.size();
            for (int i : mesh->flags)
            {
                if (i >= 0 && i < size)
                {
                    trimesh::vec3 p = matrix * mesh->vertices.at(i);
                    hullPoints2D.emplace_back(trimesh::vec2(p.x, p.y));
                }
            }
        }

        if (hullPoints2D.size() == 0)
            return;

        trimesh::TriMesh* m = qhullWrapper::convex2DPolygon((const float*)hullPoints2D.data(), hullPoints2D.size());
        if (!m)
            return;

        datas.clear();
        for (const trimesh::vec3& p : m->vertices)
            datas.emplace_back(trimesh::vec3(p.x, p.y, 0.0f));

        delete m;
    }

    bool MeshData::traitTriangle(int faceID, std::vector<trimesh::vec3>& position, const trimesh::fxform& matrix, bool _offset)
    {
        if (!mesh || faceID < 0 || faceID >= (int)mesh->faces.size())
            return false;

        const trimesh::TriMesh::Face& face = mesh->faces.at(faceID);
        position.clear();
        position.push_back(matrix * mesh->vertices.at(face.x));
        position.push_back(matrix * mesh->vertices.at(face.y));
        position.push_back(matrix * mesh->vertices.at(face.z));

        if (_offset)
        {
            trimesh::vec3 n = trimesh::trinorm(position.at(0), position.at(1), position.at(2));
            trimesh::normalize(n);

            position.at(0) += 0.1f * n;
            position.at(1) += 0.1f * n;
            position.at(2) += 0.1f * n;
        }
        return true;
    }

    TriMeshPtr MeshData::createGlobalMesh(const trimesh::xform& matrix)
    {
        if (!mesh)
            return nullptr;

        trimesh::TriMesh* newMesh = new trimesh::TriMesh();
        *newMesh = *mesh;
        trimesh::apply_xform(newMesh, matrix);
        return TriMeshPtr(newMesh);
    }

    bool MeshData::traitTriangleEx(int faceID, std::vector<trimesh::vec3>& position, trimesh::vec3& normal, const trimesh::fxform& matrix, float offsetValue, bool _offset)
    {
        if (!mesh || faceID < 0 || faceID >= (int)mesh->faces.size())
            return false;

        const trimesh::TriMesh::Face& face = mesh->faces.at(faceID);
        position.clear();
        position.push_back(matrix * mesh->vertices.at(face.x));
        position.push_back(matrix * mesh->vertices.at(face.y));
        position.push_back(matrix * mesh->vertices.at(face.z));

        if (_offset)
        {
            normal = trimesh::trinorm(position.at(0), position.at(1), position.at(2));
            trimesh::normalize(normal);

            position.at(0) += offsetValue * normal;
            position.at(1) += offsetValue * normal;
            position.at(2) += offsetValue * normal;
        }
        return true;
    }

	std::vector<cxkernel::KernelHullFace> MeshData::calculateFaces(TriMeshPtr mesh, TriMeshPtr hull)
    {
        std::vector<cxkernel::KernelHullFace> faces;
        std::vector<qhullWrapper::HullFace> _faces;
        qhullWrapper::hullFacesFromConvexMesh(hull.get(), _faces);
        qhullWrapper::hullFacesFromMeshNear(mesh, _faces);
        int size = _faces.size();
        if (size > 0)
        {
            faces.resize(size);
            for (int i = 0; i < size; ++i)
            {
                KernelHullFace& face = faces.at(i);
                const qhullWrapper::HullFace& _face = _faces.at(i);
                face.mesh = _face.mesh;
                face.normal = _face.normal;
                face.hullarea = _face.hullarea;
            }
        }
        return faces;
    }

    TriMeshPtr MeshData::calculateHull(TriMeshPtr mesh)
    {
        trimesh::TriMesh* _hull = qhullWrapper::convex_hull_3d(mesh.get());
        return TriMeshPtr(_hull);
    }
}