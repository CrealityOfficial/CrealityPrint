#include "util.h"
#include "Slice3rBase/TriangleMesh.hpp"
#include "trimesh2/TriMesh.h"
#include "ccglobal/tracer.h"

namespace spread
{
	Slic3r::TriangleMesh* trimesh2Slic3rTriangleMesh(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
	{
		if (!mesh)
			return nullptr;


        float back_end = tracer ? tracer->end() : 0;
        RESET_PROGRESS_PERCENT(tracer, 0.4);

        indexed_triangle_set indexedTriangleSet;
        trimesh2IndexTriangleSet(mesh, indexedTriangleSet, tracer);

        PROGRESS_RETURN_FINISH_CUR_NEXT(tracer, back_end, nullptr, "");

        Slic3r::TriangleMesh* slic3rMesh = constructTriangleMeshFromIndexTriangleSet(indexedTriangleSet, tracer);

		return slic3rMesh;
	}

    void trimesh2IndexTriangleSet(trimesh::TriMesh* mesh, indexed_triangle_set& indexTriangleSet, ccglobal::Tracer* tracer)
    {
        int pointSize = (int)mesh->vertices.size();
        int facesSize = (int)mesh->faces.size();
        if (pointSize < 3 || facesSize < 1)
            return;

        float back_end = tracer ? tracer->end() : 0;
        RESET_PROGRESS_PERCENT(tracer, 0.4);

        indexTriangleSet.vertices.resize(pointSize);
        indexTriangleSet.indices.resize(facesSize);
        for (int i = 0; i < pointSize; i++)
        {
            const trimesh::vec3& v = mesh->vertices.at(i);
            indexTriangleSet.vertices.at(i) = stl_vertex(v.x, v.y, v.z);

            SPLIT_PROGRESS_MSG_BREAK(i, 100, pointSize, tracer, "");
        }
        PROGRESS_BREAK_FINISH_CUR_NEXT(tracer, back_end, "");

        for (int i = 0; i < facesSize; i++)
        {
            const trimesh::TriMesh::Face& f = mesh->faces.at(i);
            stl_triangle_vertex_indices faceIndex(f[0], f[1], f[2]);
            indexTriangleSet.indices.at(i) = faceIndex;

            SPLIT_PROGRESS_MSG_BREAK(i, 100, facesSize, tracer, "");
        }
        PROGRESS_BREAK_FINISH_CUR_NEXT(tracer, back_end, "");
    }

    Slic3r::TriangleMesh* constructTriangleMeshFromIndexTriangleSet(const indexed_triangle_set& M, ccglobal::Tracer* tracer)
    {
        Slic3r::TriangleMesh* slic3rMesh = new Slic3r::TriangleMesh();

        float back_end = tracer ? tracer->end() : 0;
        RESET_PROGRESS_PERCENT(tracer, 0.1);

        stl_file& stl = slic3rMesh->stl;
        stl.stats.type = inmemory;
        // count facets and allocate memory
        stl.stats.number_of_facets = uint32_t(M.indices.size());
        stl.stats.original_num_facets = int(stl.stats.number_of_facets);
        stl_allocate(&stl);

        PROGRESS_RETURN_FINISH_CUR_NEXT(tracer, back_end, nullptr, "");
        RESET_PROGRESS_PERCENT(tracer, 0.3);

#pragma omp parallel for
        for (int i = 0; i < (int)stl.stats.number_of_facets; ++i) {
            stl_facet facet;
            facet.vertex[0] = M.vertices[size_t(M.indices[i](0))];
            facet.vertex[1] = M.vertices[size_t(M.indices[i](1))];
            facet.vertex[2] = M.vertices[size_t(M.indices[i](2))];
            facet.extra[0] = 0;
            facet.extra[1] = 0;

            stl_normal normal;
            stl_calculate_normal(normal, &facet);
            stl_normalize_vector(normal);
            facet.normal = normal;

            stl.facet_start[i] = facet;
        }

        PROGRESS_RETURN_FINISH_CUR_NEXT(tracer, back_end, nullptr, "");
        RESET_PROGRESS_PERCENT(tracer, 0.6);

        stl_get_size(&stl);
        PROGRESS_RETURN_FINISH_CUR_NEXT(tracer, back_end, nullptr, "");

        slic3rMesh->require_shared_vertices();
        JUDGE_PROGRESS_MSG_RETURN(tracer, back_end, nullptr, "");
        return slic3rMesh;
    }

    Slic3r::TriangleMesh* simpleConvert(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
    {
        if (!mesh)
            return nullptr;

        indexed_triangle_set indexed_set;
        trimesh2IndexTriangleSet(mesh, indexed_set, tracer);

        return new Slic3r::TriangleMesh(std::move(indexed_set));
    }

    void indexed2TriangleSoup(const indexed_triangle_set& indexed, std::vector<trimesh::vec3>& triangles)
    {
        triangles.clear();
        int count = (int)indexed.indices.size();
        if (count > 0)
        {
            triangles.resize(3 * count);
            for (int i = 0; i < count; ++i)
            {
                const stl_triangle_vertex_indices& indices = indexed.indices.at(i);
                for (int j = 0; j < 3; ++j)
                    triangles.at(3 * i + j) = toVector(indexed.vertices.at(indices(j)));
            }
        }
    }

    trimesh::TriMesh* slic3rMesh2TriMesh(const Slic3r::TriangleMesh& mesh)
    {
        trimesh::TriMesh* out = new trimesh::TriMesh();

        const stl_file& stl = mesh.stl;
        uint32_t facesn = stl.facet_start.size();
        out->vertices.resize(facesn * 3);
        out->faces.resize(facesn);
        for (uint32_t i = 0; i < facesn; i++)
        {
            trimesh::TriMesh::Face& faceIndex = out->faces[i];
            const stl_vertex& vertexpos = stl.facet_start[i].vertex[0];

            out->vertices[i * 3] = trimesh::vec3(vertexpos.x(), vertexpos.y(), vertexpos.z());
            const stl_vertex& vertexpos1 = stl.facet_start[i].vertex[1];
            out->vertices[i * 3 + 1] = trimesh::vec3(vertexpos1.x(), vertexpos1.y(), vertexpos1.z());
            const stl_vertex& vertexpos2 = stl.facet_start[i].vertex[2];
            out->vertices[i * 3 + 2] = trimesh::vec3(vertexpos2.x(), vertexpos2.y(), vertexpos2.z());
            faceIndex[0] = i * 3;
            faceIndex[1] = i * 3 + 1;
            faceIndex[2] = i * 3 + 2;
        }

        return out;
    }

}