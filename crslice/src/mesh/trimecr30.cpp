#include "trimecr30.h"
#include "box2dgrid.h"
#include "filler.h"
#include "trimesh2/TriMesh_algo.h"

namespace crslice
{
    trimesh::TriMesh* _generateBeltSupport(trimesh::TriMesh* mesh, float angle)
    {
        if (!mesh)
            return nullptr;
        mesh->need_bbox();
        trimesh::box3 box = mesh->bbox;
        float supportSize = std::max(box.size().x, box.size().y) / 30.0f;

        std::vector<VerticalSeg> supports;
        Box2DGrid grid;

        trimesh::fxform xf = trimesh::fxform::identity();
        grid.build(mesh, xf, false);
        grid.autoSupport(supports, supportSize, angle, false);
        size_t size = supports.size();

        if (size == 0)
            return nullptr;

        trimesh::TriMesh * supportMesh = new trimesh::TriMesh();
        float r = supportSize * sqrtf(2.0f) * 0.9f / 2.0f;
        (void)r;

        int vertexNum = 12 * 3 * size;
        supportMesh->vertices.resize(vertexNum);
        for (int i = 0; i < size; ++i)
        {
            VerticalSeg& seg = supports.at(i);
            fillCylinderSoup(&supportMesh->vertices.at(0) + 36 * i, supportSize, seg.b,
                supportSize, seg.t, 4, 45.0f);
        }

        size_t vertexSize = supportMesh->vertices.size();
        if (vertexSize < 3)
        {
            delete supportMesh;
            return nullptr;
        }

        supportMesh->faces.reserve(vertexSize / 3);
        for (size_t i = 0; i < vertexSize / 3; ++i)
        {
            trimesh::TriMesh::Face f;
            f.x = (int)(3 * i);
            f.y = (int)(3 * i + 1);
            f.z = (int)(3 * i + 2);
            supportMesh->faces.push_back(f);
        }
        return supportMesh;
    }

    trimesh::fxform beltXForm(const trimesh::vec3 & offset, float angle)
    {
        float theta = angle * M_PIf / 180.0f;

        trimesh::fxform xf0 = trimesh::fxform::trans(offset);

        trimesh::fxform xf1 = trimesh::fxform::identity();
        xf1(2, 2) = 1.0f / sinf(theta);
        xf1(1, 2) = -1.0f / tanf(theta);

        trimesh::fxform xf2 = trimesh::fxform::identity();
        xf2(2, 2) = 0.0f;
        xf2(1, 1) = 0.0f;
        xf2(2, 1) = -1.0f;
        xf2(1, 2) = 1.0f;

        trimesh::fxform xf3 = trimesh::fxform::trans(0.0f, 0.0f, 5000.0f);

        trimesh::fxform xf = xf3 * xf2 * xf1 * xf0;
        //trimesh::fxform xf = xf3 * xf0;
        return xf;
    }

    std::vector<trimesh::TriMesh*> _beforeSliceBelt(const std::vector<trimesh::TriMesh*>& meshs, Cr30Param& cr30Param, trimesh::fxform & m_xf)
    {
        std::vector<trimesh::TriMesh*> out;
        size_t size = meshs.size();

        if (cr30Param.belt_support_enable)
        {
            //CXLogInfo("Belt Slice Enable Support");
            std::vector<trimesh::TriMesh*> tmps;
            for (size_t i = 0; i < size; ++i)
            {
                trimesh::TriMesh* mesh = meshs.at(i);
                float angle = 45.0f;

                angle = (float)cr30Param.support_angle;
                trimesh::TriMesh* support= _generateBeltSupport(mesh, angle);

                if (support)
                {
                    tmps.push_back(support);
                }
            }

            for (auto mesh : tmps)
                out.push_back(mesh);
        }
        else
        {
            ;
        }

        double w = cr30Param.machine_width;
        double d = cr30Param.machine_depth;

        std::vector<trimesh::TriMesh*> meshes;
        meshes.insert(meshes.end(),meshs.begin(), meshs.end());
        meshes.insert(meshes.end(), out.begin(), out.end());

        size = meshes.size();

        if (size == 0)
            return out;

        trimesh::box3 box0;
        for (size_t i = 0; i < size; ++i)
        {
            trimesh::TriMesh* mesh = meshes.at(i);
            mesh->clear_bbox();
            mesh->need_bbox();
            box0 += mesh->bbox;
        }

        trimesh::vec3 offset = trimesh::vec3(w / 2.0 - box0.center().x, d / 2.0 - box0.min.y, -box0.min.z);
        trimesh::xform ttxf = trimesh::xform::trans(w / 2.0 - box0.center().x, d / 2.0 - box0.min.y, -box0.min.z);
        for (size_t i = 0; i < size; ++i)
        {
            trimesh::TriMesh* mesh = meshes.at(i);
            trimesh::fxform xf = beltXForm(offset, 45.0f);
            trimesh::apply_xform(mesh, trimesh::xform(xf));
        }

        trimesh::box3 box;
        for (size_t i = 0; i < size; ++i)
        {
            trimesh::TriMesh* mesh = meshes.at(i);
            mesh->clear_bbox();
            mesh->need_bbox();
            box += mesh->bbox;
        }

        cr30Param.beltOffsetX = w / 2.0 - box0.center().x;
        cr30Param.beltOffsetY = box0.min.y - box.min.z;
        trimesh::xform txf = trimesh::xform::trans(0.0, -box.min.y, -box.min.z);
        for (size_t i = 0; i < size; ++i)
        {
            trimesh::TriMesh* mesh = meshes.at(i);
            trimesh::apply_xform(mesh, txf);
            mesh->clear_bbox();
            mesh->need_bbox();
        }

        m_xf = trimesh::inv(ttxf * txf);

        bool has_error = false;
        for (size_t i = 0; i < size; ++i)
        {
            trimesh::TriMesh* mesh = meshes.at(i);
            for (trimesh::vec3& v : mesh->vertices)
            {
                float y = v.y + v.z;
                if (y < 0.0f)
                {
                    has_error = true;
                }
            }
        }
        if (has_error)
        {
            ;
        }

        return out;
    }

    std::vector<trimesh::TriMesh*> sliceBelt(std::vector<trimesh::TriMesh*>& meshs, Cr30Param& cr30Param, ccglobal::Tracer* m_progress )
    {
        trimesh::fxform m_xf;
        return _beforeSliceBelt(meshs, cr30Param, m_xf);
    }
}





