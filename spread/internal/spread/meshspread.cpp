#include "spread/meshspread.h"
#include "util.h"
#include "Slice3rBase/TriangleMesh.hpp"
#include "Slice3rBase/TriangleSelector.hpp"
#include "Slice3rBase/SLA/IndexedMesh.hpp"
#include "msbase/mesh/chunk.h"

#define MAX_RADIUS 8
#define  PI 3.141592 

namespace spread
{

    MeshSpreadWrapper::MeshSpreadWrapper()
        : m_highlight_by_angle_threshold_deg(0.0f)
    {

    }

    MeshSpreadWrapper::~MeshSpreadWrapper()
    {
        m_chunkFaces.clear();
        m_faceChunkIDs.clear();
        m_data.first.clear();
        m_data.second.clear();
    }

    void MeshSpreadWrapper::setInputs(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer)
    {
        if (mesh == nullptr)
            return;

        m_chunkFaces.clear();
        m_faceChunkIDs.clear();
        m_data.first.clear();
        m_data.second.clear();

        if (m_triangle_selector != nullptr)
             m_triangle_selector->reset();

        m_mesh.reset(simpleConvert(mesh, tracer));
        m_emesh.reset(new Slic3r::sla::IndexedMesh(*m_mesh.get(),true));
        m_triangle_selector.reset(new Slic3r::TriangleSelector(*m_mesh));

        const std::vector<Slic3r::Vec3i>& neigbs = m_triangle_selector->originNeighbors();
        std::vector<trimesh::ivec3> _neigbs(neigbs.size());
        for (int i=0;i< neigbs.size();i++)
        {
            _neigbs[i] = trimesh::ivec3(neigbs[i].x(), neigbs[i].y(), neigbs[i].z());
        }
        const float max_area = 1.0 * PI * MAX_RADIUS * MAX_RADIUS * 5;
        msbase::generateChunk( mesh, _neigbs, max_area,m_faceChunkIDs,m_chunkFaces);

        return;
    }

    void MeshSpreadWrapper::testChunk()
    {
        int faceCount = m_mesh->facets_count();
        //assert(faceCount == (int)neigbs.size());

        int chunkCount = (int)m_chunkFaces.size();
        for (int i = 0; i < chunkCount; ++i)
        {
            const std::vector<int>& faces = m_chunkFaces.at(i);
            for(int j : faces)
                m_triangle_selector->set_facet(j, (Slic3r::EnforcerBlockerType)(i % 8));
        }
    }

    int MeshSpreadWrapper::chunkCount()
    {
        return (int)m_chunkFaces.size();
    }

    void MeshSpreadWrapper::chunk(int index, std::vector<trimesh::vec3>& positions, std::vector<int>& flags, std::vector<int>& splitIndices)
    {
        assert(index >= 0 && index < m_chunkFaces.size());
        indexed_triangle_set indexed;
        m_triangle_selector->get_chunk_facets(index, m_faceChunkIDs, indexed, flags, splitIndices);

        indexed2TriangleSoup(indexed, positions);
    }

    void MeshSpreadWrapper::set_paint_on_overhangs_only(float angle_threshold_deg)
    {
        m_highlight_by_angle_threshold_deg = angle_threshold_deg;
    }

    void MeshSpreadWrapper::circile_factory(const trimesh::vec& center, const trimesh::vec3& camera_pos, float radius, int facet_start, int colorIndex
        , const trimesh::vec& normal, const float offset
        , std::vector<int>& dirty_chunks)
    {
        Slic3r::Vec3f cursor_center(center.x, center.y, center.z);
        Slic3r::Vec3f source(camera_pos.x, camera_pos.y, camera_pos.z);
        float radius_world = radius;
        Slic3r::Transform3d trafo_no_translate = Slic3r::Transform3d::Identity();
        Slic3r::TriangleSelector::ClippingPlane clipping_plane;
        clipping_plane.normal = Slic3r::Vec3f(normal.x, normal.y, normal.z);
        clipping_plane.offset = offset;

        std::unique_ptr<Slic3r::TriangleSelector::Cursor> cursor = Slic3r::TriangleSelector::Circle::cursor_factory(cursor_center,
            source, radius_world, Slic3r::TriangleSelector::CursorType::CIRCLE, trafo_no_translate, clipping_plane);

        bool triangle_splitting_enabled = true;

        Slic3r::EnforcerBlockerType new_state = Slic3r::EnforcerBlockerType(colorIndex);
        //Slic3r::Transform3d trafo_no_translate = Slic3r::Transform3d::Identity();

        m_triangle_selector->select_patch(facet_start, std::move(cursor), new_state, trafo_no_translate,
            triangle_splitting_enabled, m_highlight_by_angle_threshold_deg);

        std::vector<int> dirty_source_triangles;
        m_triangle_selector->clear_dirty_source_triangles(dirty_source_triangles);
        dirty_source_triangles_2_chunks(dirty_source_triangles, dirty_chunks);
    }

    void MeshSpreadWrapper::double_circile_factory(const trimesh::vec& center, const trimesh::vec& second_center, const trimesh::vec3& camera_pos,float radius, int facet_start, int colorIndex
        , const trimesh::vec& normal, const float offset
        , std::vector<int>& dirty_chunks)
    {
        Slic3r::Vec3f cursor_center(center.x, center.y, center.z);
        Slic3r::Vec3f second_cursor_center(second_center.x, second_center.y, second_center.z);
        Slic3r::Vec3f source(camera_pos.x, camera_pos.y, camera_pos.z);
        float radius_world = radius;
        Slic3r::Transform3d trafo = Slic3r::Transform3d::Identity();
        Slic3r::TriangleSelector::ClippingPlane clipping_plane;
        clipping_plane.normal = Slic3r::Vec3f(normal.x, normal.y, normal.z);
        clipping_plane.offset = offset;

        std::unique_ptr<Slic3r::TriangleSelector::Cursor> cursor = Slic3r::TriangleSelector::DoublePointCursor::cursor_factory(cursor_center, second_cursor_center,
            source, radius_world, Slic3r::TriangleSelector::CursorType::CIRCLE, trafo, clipping_plane);

        bool triangle_splitting_enabled = true;

        Slic3r::EnforcerBlockerType new_state = Slic3r::EnforcerBlockerType(colorIndex);
        Slic3r::Transform3d trafo_no_translate = Slic3r::Transform3d::Identity();

        m_triangle_selector->select_patch(facet_start, std::move(cursor), new_state, trafo_no_translate,
            triangle_splitting_enabled, m_highlight_by_angle_threshold_deg);

        std::vector<int> dirty_source_triangles;
        m_triangle_selector->clear_dirty_source_triangles(dirty_source_triangles);
        dirty_source_triangles_2_chunks(dirty_source_triangles, dirty_chunks);
    }

    void MeshSpreadWrapper::updateData()
    {
        m_data.first.shrink_to_fit();
        m_data.second.shrink_to_fit();
        m_data = m_triangle_selector->serialize();
    }

    void MeshSpreadWrapper::bucket_fill_select_triangles(int colorIndex, std::vector<int>& dirty_chunks)
    {
        m_triangle_selector->seed_fill_apply_on_triangles(Slic3r::EnforcerBlockerType(colorIndex));

        std::vector<int> dirty_source_triangles;
        m_triangle_selector->clear_dirty_source_triangles(dirty_source_triangles);
        dirty_source_triangles_2_chunks(dirty_source_triangles, dirty_chunks);
    }

    void MeshSpreadWrapper::bucket_fill_select_triangles_preview(const trimesh::vec& center, int facet_start, int colorIndex, std::vector<std::vector<trimesh::vec3>>& contour
        , const trimesh::vec& normal, const float offset
        , bool isFill)
    {
        Slic3r::TriangleSelector::CursorType _cursor_type = Slic3r::TriangleSelector::CursorType(Slic3r::TriangleSelector::CursorType::GAP_FILL); 
        float seed_fill_angle = 30.0f;
        bool propagate = true;
        bool force_reselection = true;

        if (!isFill)
        {
            seed_fill_angle = -1.0f;
            propagate = false;
        }

        Slic3r::Transform3d trafo_no_translate = Slic3r::Transform3d::Identity();
        Slic3r::TriangleSelector::ClippingPlane _clipping_plane;
        _clipping_plane.normal = Slic3r::Vec3f(normal.x, normal.y, normal.z);
        _clipping_plane.offset = offset;

        Slic3r::EnforcerBlockerType new_state = Slic3r::EnforcerBlockerType(colorIndex);
        if (facet_start >= 0 && facet_start < m_triangle_selector->getFacetsNum())
        {
            //m_triangle_selector->bucket_fill_select_triangles(
            m_triangle_selector->seed_fill_select_triangles(
                Slic3r::Vec3f(center)
                , facet_start
                , trafo_no_translate
                , _clipping_plane
                , seed_fill_angle
                , m_highlight_by_angle_threshold_deg
                , false);

            std::vector<Slic3r::Vec2i> contour_edges = m_triangle_selector->get_seed_fill_contour();
            contour.reserve(contour_edges.size());
            for (const Slic3r::Vec2i& edge : contour_edges) {
                std::vector<trimesh::vec> line;
                int index= edge(0);
                auto vector = m_triangle_selector->getVectors(index);
                line.emplace_back(trimesh::vec3(vector.x(), 
                                                vector.y(), 
                                                vector.z()));

                index = edge(1);
                vector = m_triangle_selector->getVectors(index);
                line.emplace_back(trimesh::vec3(vector.x(), 
                                                vector.y(), 
                                                vector.z()));

                contour.emplace_back(line);
            }
        }
    }

    void MeshSpreadWrapper::seed_fill_unselect_all_triangles()
    {
        m_triangle_selector->seed_fill_unselect_all_triangles();
    }

    void MeshSpreadWrapper::get_current_select_contours(std::vector<trimesh::vec3>& contour, const trimesh::vec3& offset)
    {
        std::vector<Slic3r::Vec2i> contour_edges = m_triangle_selector->get_seed_fill_contour();
        contour.clear();

        int size = (int)contour_edges.size();
        if (size == 0)
            return;

        contour.resize(2 * size);
        for(int i = 0; i < size; ++i)
        {
            const Slic3r::Vec2i& edge = contour_edges.at(i);
            contour.at(2 * i) = toVector(m_triangle_selector->vertex(edge(0))) + offset;
            contour.at(2 * i + 1) = toVector(m_triangle_selector->vertex(edge(1))) + offset;
        }
    }

    void MeshSpreadWrapper::dirty_source_triangles_2_chunks(const std::vector<int>& dirty_source_triangls, std::vector<int>& chunks)
    {
        chunks.clear();
        int size = (int)m_chunkFaces.size();
        std::vector<bool> chunk_dirty(size, false);
        
        for (int source : dirty_source_triangls)
        {
            chunk_dirty.at(m_faceChunkIDs[source]) = true;
        }

        for (int i = 0; i < size; ++i)
        {
            if (chunk_dirty.at(i))
                chunks.push_back(i);
        }
    }

    std::string MeshSpreadWrapper::get_triangle_as_string(int triangle_idx) const
    {
        std::string out;

        auto triangle_it = std::lower_bound(m_data.first.begin(), m_data.first.end(), triangle_idx, [](const std::pair<int, int>& l, const int r) { return l.first < r; });
        if (triangle_it != m_data.first.end() && triangle_it->first == triangle_idx) {
            int offset = triangle_it->second;
            int end = ++triangle_it == m_data.first.end() ? int(m_data.second.size()) : triangle_it->second;
            while (offset < end) {
                int next_code = 0;
                for (int i = 3; i >= 0; --i) {
                    next_code = next_code << 1;
                    if (offset + i < m_data.second.size())
                        next_code |= int(m_data.second[offset + i]);
                    else 
                        next_code |= int(m_data.second[0]);
                }
                offset += 4;

                assert(next_code >= 0 && next_code <= 15);
                char digit = next_code < 10 ? next_code + '0' : (next_code - 10) + 'A';
                out.insert(out.begin(), digit);
            }
        }
        return out;
    }
    void MeshSpreadWrapper::set_triangle_from_string(int triangle_id, const std::string& str)
    {
        assert(!str.empty());
        //assert(m_data.first.empty() || m_data.first.back().first < triangle_id);
        m_data.first.emplace_back(triangle_id, int(m_data.second.size()));

        m_data.second.reserve(m_data.second.size() + str.size()*4 +1);
        for (auto it = str.crbegin(); it != str.crend(); ++it) {
            const char ch = *it;
            int dec = 0;
            if (ch >= '0' && ch <= '9')
                dec = int(ch - '0');
            else if (ch >= 'A' && ch <= 'F')
                dec = 10 + int(ch - 'A');
            else
                assert(false);

            // Convert to binary and append into code.
            for (int i = 0; i < 4; ++i)
                m_data.second.insert(m_data.second.end(), bool(dec & (1 << i)));         
        }
    }

    void MeshSpreadWrapper::updateTriangle()
    {
        m_triangle_selector->deserialize(m_data);
    }

    void MeshSpreadWrapper::reset()
    {
        m_triangle_selector->reset();
    }

    int MeshSpreadWrapper::source_triangle_index(int index)
    {
        return m_triangle_selector->source_triangle(index);
    }

    int MeshSpreadWrapper::chunkId2FaceId(int chunkId, int index)
    {
        if (chunkId>= 0 && chunkId < m_faceChunkIDs.size())
        {
            if (index >=0 && index < m_chunkFaces[chunkId].size())
            {
                return m_chunkFaces[chunkId][index];
            }
        }

        return 0;
    }

    int MeshSpreadWrapper::getFacet(const trimesh::vec& point, trimesh::vec& direction, trimesh::vec& cross)
    {
        Slic3r::Vec3d _point(point.x, point.y,point.z);
        Slic3r::Vec3d _direction(direction.x, direction.y, direction.z);
        Slic3r::sla::IndexedMesh::hit_result hit = m_emesh->query_ray_hit(_point, _direction);

        if (hit.is_hit())
        {
            cross = trimesh::vec(hit.position().x(), hit.position().y(), hit.position().z()) ;
        }
        return hit.face();
    }

    std::vector<std::string> MeshSpreadWrapper::get_data_as_string() const
    {
        std::vector<std::string> data;
        int facets_count = m_mesh->its.indices.size();
        data.resize(facets_count);
        for (int i = 0; i < facets_count; ++i)
        {
            std::string face_data = get_triangle_as_string(i);
            if (face_data.empty())
                continue;

            data[i] = face_data;
        }
        return data;
    }

    void MeshSpreadWrapper::set_triangle_from_data(std::vector<std::string> strList)
    {
        m_data.first.clear();
        m_data.second.clear();
        int facets_count = m_mesh->its.indices.size();
        if (strList.size() == 0)
            strList = std::vector<std::string>(facets_count);
        
        assert(strList.size() == facets_count);
        for (int i = 0; i < facets_count; ++i)
        {
            const std::string& str = strList[i];
            if (!str.empty())
                set_triangle_from_string(i, str);
        }

        updateTriangle();
    }
}