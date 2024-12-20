#include "spread/meshspread.h"
#include "util.h"
#include "Slice3rBase/TriangleMesh.hpp"
#include "Slice3rBase/TriangleSelector.hpp"
#include "Slice3rBase/SLA/IndexedMesh.hpp"
#include "msbase/mesh/chunk.h"
#include "msbase/mesh/merge.h"

#define MAX_RADIUS 8
#define  PI 3.141592 

namespace spread
{
    Slic3r::Transform3d trixform_2_slice3rform(const trimesh::xform& trimeshMat)
    {
		Slic3r::Transform3d slice3r_matrix = Slic3r::Transform3d::Identity();
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
                slice3r_matrix(i, j) = trimeshMat[i + j * 4];
			}
		}

        return slice3r_matrix;
    }

    Slic3r::TriangleSelector::ClippingPlane get_clipping_plane_in_mesh_coordinates(const Slic3r::Transform3d& trafo_matrix, const trimesh::vec& normal, float offset)
    {
        Slic3r::Vec3d  clp_normal = Slic3r::Vec3d(normal.x, normal.y, normal.z);
        double clp_offset = offset;

        Slic3r::Transform3d trafo_normal = Slic3r::Transform3d(trafo_matrix.linear().transpose());
        Slic3r::Transform3d trafo_inv = trafo_matrix.inverse();

        Slic3r::Vec3d point_on_plane = clp_normal * clp_offset;
        Slic3r::Vec3d point_on_plane_transformed = trafo_inv * point_on_plane;
        Slic3r::Vec3d normal_transformed = trafo_normal * clp_normal;
        float offset_transformed = float(point_on_plane_transformed.dot(normal_transformed));

        return Slic3r::TriangleSelector::ClippingPlane({ float(normal_transformed.x()), float(normal_transformed.y()), float(normal_transformed.z()), offset_transformed });
    }

    class TrianglePatch{
    public:
        std::vector<float>  patch_vertices;
        std::vector<int> triangle_indices;

        Slic3r::EnforcerBlockerType patch_state = Slic3r::EnforcerBlockerType::NONE;
        std::set< Slic3r::EnforcerBlockerType> neighbor_types;
        std::vector<int> neighbor_state_number;
        float area = 0.f;
    };

    class TriangleNeighborState {
    public:
        std::vector<Slic3r::EnforcerBlockerType> virtual_state;
    };


    MeshSpreadWrapper::MeshSpreadWrapper()
        : m_highlight_by_angle_threshold_deg(0.0f)
    {

    }

    MeshSpreadWrapper::~MeshSpreadWrapper()
    {
        clearBuffer();
    }


    void MeshSpreadWrapper::clearBuffer()
    {
        m_chunkFaces.clear();
        m_faceChunkIDs.clear();
        m_data.first.clear();
        m_data.second.clear();
        m_triangle_patches.clear();
        m_triangle_virtual_state.clear();
        //m_triangle_selector->reset();
        m_triangle_selector.reset();
        last_triangle_change_state.clear();
        before_chunks.clear();
        m_mesh->clear();
        m_mesh.reset();
        m_mesh.release();
        m_emesh.reset();
        m_emesh.release();       
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

    void MeshSpreadWrapper::setGroupInputs(std::vector<trimesh::TriMesh*>m_groupmesh, ccglobal::Tracer* tracer)
    {
        if (m_groupmesh.empty())
            return;
        group_faces_size.clear();
        int begin = 0;
        for (const auto mesh : m_groupmesh)
        {
            group_faces_size.push_back(begin);
            begin += mesh->faces.size();
        }
        trimesh::TriMesh* group_mesh = msbase::mergeMeshes(m_groupmesh);
        setInputs(group_mesh, tracer);
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

    void MeshSpreadWrapper::chunk_gap_fill(int index, std::vector<trimesh::vec3>& positions, std::vector<int>& flags, std::vector<int>& flags_before, std::vector<int>& splitIndices)
    {
        if (m_triangle_virtual_state.empty()) return;
        assert(index >= 0 && index < m_chunkFaces.size());
        indexed_triangle_set indexed;
        m_triangle_selector->get_chunk_facets(index, m_faceChunkIDs, indexed, flags_before, splitIndices);
        for (int ti : splitIndices)
        {
            flags.push_back((int)m_triangle_virtual_state[0].virtual_state[ti]);
        }

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
        Slic3r::Transform3d trafo_matrix = trixform_2_slice3rform(m_meshGlobalMatrix);

        Slic3r::TriangleSelector::ClippingPlane clipping_plane = get_clipping_plane_in_mesh_coordinates(trafo_matrix, normal, offset);

        std::unique_ptr<Slic3r::TriangleSelector::Cursor> cursor = Slic3r::TriangleSelector::Circle::cursor_factory(cursor_center,
            source, radius_world, Slic3r::TriangleSelector::CursorType::CIRCLE, trafo_matrix, clipping_plane);
     
        bool triangle_splitting_enabled = true;

        Slic3r::EnforcerBlockerType new_state = Slic3r::EnforcerBlockerType(colorIndex);
        //Slic3r::Transform3d trafo_no_translate = Slic3r::Transform3d::Identity();

        m_triangle_selector->select_patch(facet_start, std::move(cursor), new_state, trafo_no_translate,
            triangle_splitting_enabled, m_highlight_by_angle_threshold_deg);

        std::vector<int> dirty_source_triangles;
        m_triangle_selector->clear_dirty_source_triangles(dirty_source_triangles);
        dirty_source_triangles_2_chunks(dirty_source_triangles, dirty_chunks);
    }


    void MeshSpreadWrapper::height_factory(const trimesh::vec& center, const trimesh::vec3& camera_pos, float height, int facet_start, int colorIndex
        , const trimesh::vec& normal, const float offset
        , std::vector<int>& dirty_chunks)
    {     
        Slic3r::Vec3f source(camera_pos.x, camera_pos.y, camera_pos.z);
        Slic3r::Transform3d trafo_no_translate = Slic3r::Transform3d::Identity();

        Slic3r::Transform3d trafo_matrix = trixform_2_slice3rform(m_meshGlobalMatrix);

        Slic3r::TriangleSelector::ClippingPlane clipping_plane = get_clipping_plane_in_mesh_coordinates(trafo_matrix, normal, offset);

        //height
        std::unique_ptr<Slic3r::TriangleSelector::Cursor> cursor = Slic3r::TriangleSelector::SinglePointCursor::cursor_factory(center.z, source,
            height, trafo_matrix, clipping_plane);

        /*std::unique_ptr<Slic3r::TriangleSelector::Cursor> cursor = Slic3r::TriangleSelector::HeightRange::cursor_factory(center.z, source,
            height, trafo_no_translate, clipping_plane);*/

        bool triangle_splitting_enabled = true;

        Slic3r::EnforcerBlockerType new_state = Slic3r::EnforcerBlockerType(colorIndex);
        m_triangle_selector->select_patch(facet_start, std::move(cursor), new_state, trafo_no_translate,
            triangle_splitting_enabled, m_highlight_by_angle_threshold_deg);

        std::vector<int> dirty_source_triangles;
        m_triangle_selector->clear_dirty_source_triangles(dirty_source_triangles);
        dirty_source_triangles_2_chunks(dirty_source_triangles, dirty_chunks);
    }


    void MeshSpreadWrapper::sphere_factory(const trimesh::vec& center, const trimesh::vec3& camera_pos, float radius, int facet_start, int colorIndex
        , const trimesh::vec& normal, const float offset
        , std::vector<int>& dirty_chunks)
    {
        Slic3r::Vec3f source(camera_pos.x, camera_pos.y, camera_pos.z);
        Slic3r::Vec3f inter_center(center.x, center.y, center.z);
        Slic3r::Transform3d trafo_no_translate = Slic3r::Transform3d::Identity();

        Slic3r::Transform3d trafo_matrix = trixform_2_slice3rform(m_meshGlobalMatrix);

        Slic3r::TriangleSelector::ClippingPlane clipping_plane = get_clipping_plane_in_mesh_coordinates(trafo_matrix, normal, offset);

        std::unique_ptr<Slic3r::TriangleSelector::Cursor> cursor = Slic3r::TriangleSelector::SinglePointCursor::cursor_factory(inter_center, source,
            radius, Slic3r::TriangleSelector::CursorType::SPHERE, trafo_matrix, clipping_plane);
        bool triangle_splitting_enabled = true;

        Slic3r::EnforcerBlockerType new_state = Slic3r::EnforcerBlockerType(colorIndex);
        m_triangle_selector->select_patch(facet_start, std::move(cursor), new_state, trafo_no_translate,
            triangle_splitting_enabled, m_highlight_by_angle_threshold_deg);

        std::vector<int> dirty_source_triangles;
        m_triangle_selector->clear_dirty_source_triangles(dirty_source_triangles);
        dirty_source_triangles_2_chunks(dirty_source_triangles, dirty_chunks);

    }

    void MeshSpreadWrapper::double_sphere_factory(const trimesh::vec& center, const trimesh::vec& second_center, const trimesh::vec3& camera_pos, float radius, int facet_start, int colorIndex
        , const trimesh::vec& normal, const float offset
        , std::vector<int>& dirty_chunks)
    {
        Slic3r::Vec3f source(camera_pos.x, camera_pos.y, camera_pos.z);
        Slic3r::Vec3f inter_center(center.x, center.y, center.z);
        Slic3r::Vec3f second_cursor_center(second_center.x, second_center.y, second_center.z);

        Slic3r::Transform3d trafo_matrix = trixform_2_slice3rform(m_meshGlobalMatrix);

        Slic3r::TriangleSelector::ClippingPlane clipping_plane = get_clipping_plane_in_mesh_coordinates(trafo_matrix, normal, offset);
       
        std::unique_ptr<Slic3r::TriangleSelector::Cursor> cursor = Slic3r::TriangleSelector::DoublePointCursor::cursor_factory(inter_center, second_cursor_center,
            source, radius, Slic3r::TriangleSelector::CursorType::SPHERE, trafo_matrix, clipping_plane);
        bool triangle_splitting_enabled = true;
        Slic3r::Transform3d trafo_no_translate = Slic3r::Transform3d::Identity();
        Slic3r::EnforcerBlockerType new_state = Slic3r::EnforcerBlockerType(colorIndex);
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

        Slic3r::Transform3d trafo_matrix = trixform_2_slice3rform(m_meshGlobalMatrix);

        Slic3r::TriangleSelector::ClippingPlane clipping_plane = get_clipping_plane_in_mesh_coordinates(trafo_matrix, normal, offset);

        std::unique_ptr<Slic3r::TriangleSelector::Cursor> cursor = Slic3r::TriangleSelector::DoublePointCursor::cursor_factory(cursor_center, second_cursor_center,
            source, radius_world, Slic3r::TriangleSelector::CursorType::CIRCLE, trafo_matrix, clipping_plane);

        bool triangle_splitting_enabled = true;

        Slic3r::EnforcerBlockerType new_state = Slic3r::EnforcerBlockerType(colorIndex);
        Slic3r::Transform3d trafo_no_translate = Slic3r::Transform3d::Identity();

        m_triangle_selector->select_patch(facet_start, std::move(cursor), new_state, trafo_no_translate,
            triangle_splitting_enabled, m_highlight_by_angle_threshold_deg);

        std::vector<int> dirty_source_triangles;
        m_triangle_selector->clear_dirty_source_triangles(dirty_source_triangles);
        dirty_source_triangles_2_chunks(dirty_source_triangles, dirty_chunks);
    }


    void MeshSpreadWrapper::apply_triangle_state(std::vector<int>& dirty_chunks)
    {
        if (last_triangle_change_state.empty()) return;
        std::vector<int> last_dirty_source_triangles;
        for (int tri : last_triangle_change_state)
        {
            Slic3r::EnforcerBlockerType type = m_triangle_virtual_state[0].virtual_state[tri];
            m_triangle_selector->set_triangle_state(tri, type);
            last_dirty_source_triangles.push_back(m_triangle_selector->get_source_triangle(tri));
        }    
        dirty_source_triangles_2_chunks(last_dirty_source_triangles, dirty_chunks);
    }


    float MeshSpreadWrapper::get_patch_area(const TrianglePatch& patch)
    {
        float total_area = 0.f;
        const std::vector<int>& triangle = patch.triangle_indices;
        const std::vector<float>& vertices = patch.patch_vertices;
        const int stride = 9;

        for (int i = 0; i < triangle.size(); i ++)
        {
            Slic3r::Vec3f v0(vertices[i * stride], vertices[i * stride + 1], vertices[i * stride + 2]);
            Slic3r::Vec3f v1(vertices[i * stride+3], vertices[i * stride + 4], vertices[i * stride + 5]);
            Slic3r::Vec3f v2(vertices[i * stride+6], vertices[i * stride + 7], vertices[i * stride + 8]);
            total_area += std::abs((v1 - v0).cross(v2 - v0).norm()) / 2;           
        }                           

        return total_area;
    }

    bool MeshSpreadWrapper::get_triangles_per_patch( float max_limit_area, std::vector<int>& dirty_chunks)
    {
        dirty_chunks.clear();
        m_triangle_patches.clear();
        m_triangle_virtual_state.clear();
        last_triangle_change_state.clear();
        TriangleNeighborState tns;
        tns.virtual_state.resize(m_triangle_selector->get_triangles_size(), Slic3r::EnforcerBlockerType::NONE);
        m_triangle_virtual_state.push_back(tns);
        
        dirty_chunks.insert(dirty_chunks.end(), before_chunks.begin(), before_chunks.end());


        auto [neighbors, neighbors_propagated] = m_triangle_selector->precompute_all_neighbors();
        std::vector<bool>  visited(m_triangle_selector->get_triangles_size(), false);

        int start_face_idx = 0;
        while (1)
        {
            for (; start_face_idx < visited.size(); start_face_idx++) {
                if (!visited[start_face_idx] && m_triangle_selector->get_triangle_isvalid(start_face_idx) && !m_triangle_selector->get_triangle_issplit(start_face_idx))
                    break;
            }

            if (start_face_idx >= m_triangle_selector->get_triangles_size()) break;

            Slic3r::EnforcerBlockerType frist_state = m_triangle_selector->get_triangle_state(start_face_idx);
            TrianglePatch patch;
            patch.neighbor_state_number.resize(32, 0);
            std::queue<int> facet_queue;
            facet_queue.push(start_face_idx);
            
            while (!facet_queue.empty())
            {
                int current_facet = facet_queue.front();
                facet_queue.pop();

                if (!visited[current_facet]) {
                    for (int i = 0; i < 3; i++)
                    {
                        int j = m_triangle_selector->get_triangle_vert_idx(current_facet, i);
                        int index = int(patch.patch_vertices.size() / 3);

                        patch.patch_vertices.emplace_back(m_triangle_selector->get_vertices_coord(j, 0));
                        patch.patch_vertices.emplace_back(m_triangle_selector->get_vertices_coord(j, 1));
                        patch.patch_vertices.emplace_back(m_triangle_selector->get_vertices_coord(j, 2));

                        //patch.triangle_indices.emplace_back(index);
                    }

                    patch.triangle_indices.push_back(current_facet);

                    std::vector<int> touching_triangles = m_triangle_selector->get_all_touching_triangles(current_facet, neighbors[current_facet], neighbors_propagated[current_facet]);

                    for (const int tr_idx : touching_triangles) {
                        if (tr_idx < 0)
                            continue;
                        if (m_triangle_selector->get_triangle_state(tr_idx) != frist_state)
                        {
                            patch.neighbor_types.insert(m_triangle_selector->get_triangle_state(tr_idx));
                            Slic3r::EnforcerBlockerType triangle_state = m_triangle_selector->get_triangle_state(tr_idx);
                            int state_int = (int)triangle_state;
                            patch.neighbor_state_number[state_int]++;
                            continue;
                        }
                        if (visited[tr_idx])
                            continue;
                        facet_queue.push(tr_idx);
                    }
                }
                visited[current_facet] = true;
            }

            patch.area = get_patch_area(patch);
            patch.patch_state = frist_state;
            m_triangle_patches.emplace_back(std::move(patch));
        }
        if (m_triangle_patches.empty() || m_triangle_patches.size() == 1) return false;

        bool is_gap = false;
        std::vector<int> new_dirty_chunks; 
        std::vector<int> last_dirty_source_triangles;
        for (TrianglePatch& p : m_triangle_patches)
        {
            int max_state = 0;
            for (int sti = 0; sti < p.neighbor_state_number.size(); sti++)
            {
                if (p.neighbor_state_number[max_state] < p.neighbor_state_number[sti])
                    max_state = sti;
            }
            Slic3r::EnforcerBlockerType neighbot_type = Slic3r::EnforcerBlockerType(max_state);
            Slic3r::EnforcerBlockerType type = p.patch_state;
            if (p.area > max_limit_area)
            {
                for (int tri : p.triangle_indices)
                {
                    m_triangle_virtual_state[0].virtual_state[tri] = type;
                }              
            }
            else
            {
                is_gap = true;
                for (int tri : p.triangle_indices)
                {                  
                    m_triangle_virtual_state[0].virtual_state[tri] = neighbot_type;
                    

                    last_triangle_change_state.push_back(tri);
                    last_dirty_source_triangles.push_back(m_triangle_selector->get_source_triangle(tri));
                }
            }
        }
       
        dirty_source_triangles_2_chunks(last_dirty_source_triangles, new_dirty_chunks);
        before_chunks.clear();
        before_chunks.insert(before_chunks.end(), new_dirty_chunks.begin(), new_dirty_chunks.end());
        dirty_chunks.insert(dirty_chunks.end(), new_dirty_chunks.begin(), new_dirty_chunks.end());
        std::sort(dirty_chunks.begin(), dirty_chunks.end());
        dirty_chunks.erase(std::unique(dirty_chunks.begin(), dirty_chunks.end()), dirty_chunks.end());
        return is_gap;
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

    void MeshSpreadWrapper::change_state_all_triangles(int ori_state, int des_state, std::vector<int>& dirty_chunks)
    {
        std::vector<int> last_dirty_source_triangles;
        for(int index=0;index< m_triangle_selector->get_triangles_size();index++)
        {
            if (!m_triangle_selector->get_triangle_isvalid(index) || m_triangle_selector->get_triangle_issplit(index))
                continue;
           // std::cout << "index: " << index << " " << (int)m_triangle_selector->get_triangle_state(index) << "\n";
            if ((int)m_triangle_selector->get_triangle_state(index) == ori_state)
            {
                Slic3r::EnforcerBlockerType state = Slic3r::EnforcerBlockerType(des_state);
                m_triangle_selector->set_triangle_state(index, state);
                last_dirty_source_triangles.push_back(m_triangle_selector->get_source_triangle(index));
            }
            dirty_source_triangles_2_chunks(last_dirty_source_triangles, dirty_chunks);
        }
    }

    void MeshSpreadWrapper::get_height_contour(const trimesh::vec& worldSpaceCenter, float height, std::vector<std::vector<trimesh::vec3>>& contour)
    {
        std::vector<std::vector<Slic3r::Vec3f>> contou;

        Slic3r::Transform3f trafo_matrix = Slic3r::Transform3f::Identity();
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                trafo_matrix(i, j) = m_meshGlobalMatrix[i + j * 4];
            }
        }

        m_triangle_selector->get_height_lines(worldSpaceCenter.z, worldSpaceCenter.z+height, contou, trafo_matrix);

        for (std::vector<Slic3r::Vec3f>& vtor : contou)
        {
            std::vector<trimesh::vec3> line;
            for (Slic3r::Vec3f& v3f : vtor)
            {
                line.push_back(trimesh::vec3(v3f[0], v3f[1], v3f[2]));
            }
            contour.push_back(line);
        }
    }

    void MeshSpreadWrapper::bucket_fill_select_triangles_preview(const trimesh::vec& center, int facet_start, int colorIndex, std::vector<std::vector<trimesh::vec3>>& contour
        , const trimesh::vec& normal, const float offset
        , float seed_fill_angle
        , bool isFill
        , bool isBorder
        , bool isSupport)
    {
        float angle = seed_fill_angle;
        if (isFill && !isBorder)
            angle = 180.f;
        Slic3r::TriangleSelector::CursorType _cursor_type = Slic3r::TriangleSelector::CursorType(Slic3r::TriangleSelector::CursorType::GAP_FILL); 
        //float seed_fill_angle = 30.0f;
        bool propagate = true;
        bool force_reselection = true;

        if (!isFill&&!isBorder)
        {
            angle = -1.0f;
            propagate = false;
        }

        Slic3r::Transform3d trafo_matrix = trixform_2_slice3rform(m_meshGlobalMatrix);

        Slic3r::TriangleSelector::ClippingPlane _clipping_plane = get_clipping_plane_in_mesh_coordinates(trafo_matrix, normal, offset);

        Slic3r::EnforcerBlockerType new_state = Slic3r::EnforcerBlockerType(colorIndex);
        if (facet_start >= 0 && facet_start < m_triangle_selector->getFacetsNum())
        {        
            if(isSupport)
            {
                m_triangle_selector->seed_fill_select_triangles(
                    Slic3r::Vec3f(center)
                    , facet_start
                    , trafo_matrix
                    , _clipping_plane
                    , angle
                    , m_highlight_by_angle_threshold_deg);
            }
            else
            {
                m_triangle_selector->bucket_fill_select_triangles(
               Slic3r::Vec3f(center)
               , facet_start
               , _clipping_plane
               , angle
               , propagate);
            }
                                                                              
            std::vector<Slic3r::Vec2i> contour_edges = m_triangle_selector->get_seed_fill_contour();
            contour.reserve(contour_edges.size());
            for (const Slic3r::Vec2i& edge : contour_edges) {
                std::vector<trimesh::vec> line;
                int index= edge(0);
                auto vector = m_triangle_selector->getVectors(index);
                line.emplace_back(m_meshGlobalMatrix * trimesh::vec3(vector.x(),
                                                vector.y(), 
                                                vector.z()));

                index = edge(1);
                vector = m_triangle_selector->getVectors(index);
                line.emplace_back(m_meshGlobalMatrix * trimesh::vec3(vector.x(),
                                                vector.y(), 
                                                vector.z()));

                contour.emplace_back(line);
            }
        }
    }

    void MeshSpreadWrapper::seed_fill_unselect_all_triangles()
    {
        if (m_triangle_selector)
        {
            m_triangle_selector->seed_fill_unselect_all_triangles();
        }
    }

    bool MeshSpreadWrapper::judge_select_triangles()
    {
        if (m_triangle_selector)
        {
            return m_triangle_selector->judge_select_triangles();
        }
        return false;
    }

    void MeshSpreadWrapper::setMeshGlobalMatrix(const trimesh::xform& globalXf)
    {
        m_meshGlobalMatrix = globalXf;
    }

    trimesh::xform MeshSpreadWrapper::getMeshGlobalMatrix()
    {
        return m_meshGlobalMatrix;
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
        Slic3r::Transform3d trafo_matrix = Slic3r::Transform3d::Identity();
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                trafo_matrix(i, j) = m_meshGlobalMatrix[i + j * 4];
            }
        }

        Slic3r::Vec3d _point(point.x, point.y, point.z);
        Slic3r::Vec3d _direction(direction.x, direction.y, direction.z);

        Slic3r::Transform3d inv = trafo_matrix.inverse();
        _point = inv * _point;

        _direction = inv.linear() *_direction;

        std::vector<Slic3r::sla::IndexedMesh::hit_result> hits = m_emesh->query_ray_hits(_point, _direction);
        if (hits.empty())
        {
            return -1;
        }

        Slic3r::sla::IndexedMesh::hit_result hit = hits[0];
        
        if (hit.is_hit())
        {
            cross = trimesh::vec(hit.position().x(), hit.position().y(), hit.position().z()) ;
        }
        return hit.face();
    }

    void MeshSpreadWrapper::getFacet_Group(const trimesh::vec& point, trimesh::vec& direction, trimesh::vec& cross, int& mesh_index, int& face_index)
    {
        Slic3r::Vec3d _point(point.x, point.y, point.z);
        Slic3r::Vec3d _direction(direction.x, direction.y, direction.z);
        Slic3r::sla::IndexedMesh::hit_result hit = m_emesh->query_ray_hit(_point, _direction);

        if (hit.is_hit())
        {
            cross = trimesh::vec(hit.position().x(), hit.position().y(), hit.position().z());
        }

        int hit_face = hit.face();
        int facets_count = m_mesh->its.indices.size();
        for (int i = 0; i < group_faces_size.size(); i++)
        {
            if (i!= group_faces_size.size()-1 &&hit_face >= group_faces_size[i] && hit_face < group_faces_size[i + 1])
            {
                mesh_index = i;
                face_index = hit_face - group_faces_size[i];
                break;
            }
            else if (i == group_faces_size.size() - 1 && hit_face >= group_faces_size[i] && hit_face < facets_count)
            {
                mesh_index = i;
                face_index = hit_face - group_faces_size[i];
                break;
            }
        }
    }

    bool MeshSpreadWrapper::judge_hit(const trimesh::vec& point, trimesh::vec& direction)
    {
        Slic3r::Vec3d _point(point.x, point.y, point.z);
        Slic3r::Vec3d _direction(direction.x, direction.y, direction.z);
        Slic3r::sla::IndexedMesh::hit_result hit = m_emesh->query_ray_hit(_point, _direction);
        return hit.is_hit();
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

    std::vector<std::vector<std::string>> MeshSpreadWrapper::get_date_as_groupstring() const
    {
        std::vector<std::vector<std::string>> group_date;
        group_date.resize(group_faces_size.size());

        int facets_count = m_mesh->its.indices.size();
        for (int i = 0 ,j = 0; i < facets_count; ++i)
        {
            if (j<group_faces_size.size()&&i == group_faces_size[j])
            {
                group_date.push_back(std::vector<std::string>());
                j++;
            }

            std::string face_data = get_triangle_as_string(i);
            if (face_data.empty())
                face_data = "";
            group_date.back().push_back(face_data);
        }

        return group_date;
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


    void MeshSpreadWrapper::set_triangle_from_data_group(std::vector<std::vector<std::string>> strList_group)
    {
        m_data.first.clear();
        m_data.second.clear();
        int facets_count = m_mesh->its.indices.size();
        for (int i = 0; i < strList_group.size(); i++)
        {
            if (strList_group[i].empty())
            {
                if(i< group_faces_size.size()-1)
                    strList_group[i] = std::vector<std::string>(group_faces_size[i+1]-group_faces_size[i]);
                else if(i== group_faces_size.size() -1 )
                    strList_group[i] = std::vector<std::string>(facets_count - group_faces_size[i]);
            }
        }
        std::vector<std::string> str_contianer;
        for (int i = 0; i < strList_group.size(); i++)
        {
            for (int j = 0; j < strList_group[i].size(); j++)
                str_contianer.push_back(strList_group[i][j]);
        }
        assert(str_contianer.size() == facets_count);
        for (int i = 0; i < facets_count; ++i)
        {
            const std::string& str = str_contianer[i];
            if (!str.empty())
                set_triangle_from_string(i, str);
        }

        updateTriangle();

    }
}