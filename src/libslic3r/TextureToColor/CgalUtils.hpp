#pragma once
#include "TriMesh.hpp"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <chrono>
#include <cstdio>
#include <map>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Slic3r { namespace tex2color {
namespace cgalutils {

using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using CGALMesh = CGAL::Surface_mesh<Kernel::Point_3>;

inline CGALMesh trimesh_to_cgal(const TriMesh& mesh) {
    CGALMesh cm;
    std::vector<CGALMesh::Vertex_index> vmap(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i)
        vmap[i] = cm.add_vertex(Kernel::Point_3(mesh.vertices[i].x(), mesh.vertices[i].y(), mesh.vertices[i].z()));
    for (const auto& f : mesh.indices) {
        cm.add_face(vmap[f[0]], vmap[f[1]], vmap[f[2]]);
    }
    return cm;
}

inline TriMesh cgal_to_trimesh(const CGALMesh& cm) {
    TriMesh mesh;
    std::map<CGALMesh::Vertex_index, size_t> vmap;
    size_t idx = 0;
    for (auto v : cm.vertices()) {
        if (!cm.is_valid(v) || cm.is_removed(v)) continue;
        auto p = cm.point(v);
        mesh.vertices.push_back(Vec3f((float)p.x(), (float)p.y(), (float)p.z()));
        vmap[v] = idx++;
    }
    for (auto f : cm.faces()) {
        if (!cm.is_valid(f) || cm.is_removed(f)) continue;
        auto h = cm.halfedge(f);
        auto v0 = cm.target(h);
        auto v1 = cm.target(cm.next(h));
        auto v2 = cm.target(cm.next(cm.next(h)));
        mesh.indices.push_back(Vec3i32((int)vmap[v0], (int)vmap[v1], (int)vmap[v2]));
    }
    return mesh;
}

inline bool is_mesh_halfedge_compatible(const TriMesh& mesh) {
    using Index = std::uint32_t;
    constexpr Index invalid = std::numeric_limits<Index>::max();

    if (mesh.vertices.size() >= invalid || mesh.indices.size() > invalid / 3)
        return false;
    const std::size_t corner_count = mesh.indices.size() * 3;

    struct EdgeRecord {
        Index low;
        Index high;
        Index low_corner;
        Index high_corner;
        bool  low_to_high;
    };

    std::vector<EdgeRecord> edges;
    edges.reserve(corner_count);
    for (std::size_t fid = 0; fid < mesh.indices.size(); ++fid) {
        const TriFace& face = mesh.indices[fid];
        if (face[0] < 0 || face[1] < 0 || face[2] < 0 ||
            static_cast<std::size_t>(face[0]) >= mesh.vertices.size() ||
            static_cast<std::size_t>(face[1]) >= mesh.vertices.size() ||
            static_cast<std::size_t>(face[2]) >= mesh.vertices.size() ||
            face[0] == face[1] || face[1] == face[2] || face[2] == face[0])
            return false;

        const Index base = static_cast<Index>(fid * 3);
        for (Index i = 0; i < 3; ++i) {
            const Index j    = (i + 1) % 3;
            const Index from = static_cast<Index>(face[i]);
            const Index to   = static_cast<Index>(face[j]);
            if (from < to)
                edges.push_back({from, to, base + i, base + j, true});
            else
                edges.push_back({to, from, base + j, base + i, false});
        }
    }

    std::sort(edges.begin(), edges.end(), [](const EdgeRecord& lhs, const EdgeRecord& rhs) {
        return lhs.low < rhs.low || (lhs.low == rhs.low && lhs.high < rhs.high);
    });

    std::vector<Index> parent(corner_count);
    std::vector<std::uint8_t> rank(corner_count, 0);
    for (Index i = 0; i < static_cast<Index>(corner_count); ++i)
        parent[i] = i;

    auto find_root = [&parent](Index value) {
        Index root = value;
        while (parent[root] != root)
            root = parent[root];
        while (parent[value] != value) {
            const Index next = parent[value];
            parent[value] = root;
            value = next;
        }
        return root;
    };
    auto unite = [&parent, &rank, &find_root](Index lhs, Index rhs) {
        lhs = find_root(lhs);
        rhs = find_root(rhs);
        if (lhs == rhs)
            return;
        if (rank[lhs] < rank[rhs])
            std::swap(lhs, rhs);
        parent[rhs] = lhs;
        if (rank[lhs] == rank[rhs])
            ++rank[lhs];
    };

    for (std::size_t begin = 0; begin < edges.size();) {
        std::size_t end = begin + 1;
        while (end < edges.size() && edges[end].low == edges[begin].low && edges[end].high == edges[begin].high)
            ++end;
        if (end - begin > 2 ||
            (end - begin == 2 && edges[begin].low_to_high == edges[begin + 1].low_to_high))
            return false;
        if (end - begin == 2) {
            unite(edges[begin].low_corner, edges[begin + 1].low_corner);
            unite(edges[begin].high_corner, edges[begin + 1].high_corner);
        }
        begin = end;
    }

    std::vector<Index> first_corner(mesh.vertices.size(), invalid);
    for (std::size_t fid = 0; fid < mesh.indices.size(); ++fid) {
        const TriFace& face = mesh.indices[fid];
        for (Index i = 0; i < 3; ++i) {
            const Index vertex = static_cast<Index>(face[i]);
            const Index corner = static_cast<Index>(fid * 3) + i;
            if (first_corner[vertex] == invalid)
                first_corner[vertex] = corner;
            else if (find_root(first_corner[vertex]) != find_root(corner))
                return false;
        }
    }
    return true;
}
inline bool convert_trimesh_to_cgal(const TriMesh& mesh, CGALMesh& cgal_mesh) {
    cgal_mesh = trimesh_to_cgal(mesh);
    return cgal_mesh.number_of_faces() > 0 || mesh.indices.empty();
}

inline bool convert_trimesh_to_cgal(
    const TriMesh& mesh, const std::vector<Vec2f>& vertex_uvs,
    CGALMesh& cgal_mesh, std::vector<Vec2f>& cgal_vertex_uvs)
{
    cgal_mesh.clear();
    std::vector<CGALMesh::Vertex_index> vmap(mesh.vertices.size());
    cgal_vertex_uvs.clear();

    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        vmap[i] = cgal_mesh.add_vertex(Kernel::Point_3(
            mesh.vertices[i].x(), mesh.vertices[i].y(), mesh.vertices[i].z()));
    }

    cgal_vertex_uvs.resize(cgal_mesh.num_vertices());
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        if (i < vertex_uvs.size())
            cgal_vertex_uvs[vmap[i]] = vertex_uvs[i];
        else
            cgal_vertex_uvs[vmap[i]] = Vec2f(0.f, 0.f);
    }

    for (const auto& f : mesh.indices)
        cgal_mesh.add_face(vmap[f[0]], vmap[f[1]], vmap[f[2]]);

    return true;
}

} // namespace cgalutils
} // namespace tex2color
} // namespace Slic3r
