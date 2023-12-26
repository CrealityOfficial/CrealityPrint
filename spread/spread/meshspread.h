#ifndef MESH_SPREAD_1595984973500_H
#define MESH_SPREAD_1595984973500_H

#include "spread/header.h"
#include <set>

namespace Slic3r {
    class TriangleMesh;
    class TriangleSelector;
    namespace sla {
        class IndexedMesh;
    }
}

namespace spread
{
    enum class SPREAD_API EnforcerBlockerType : int8_t {
        // Maximum is 3. The value is serialized in TriangleSelector into 2 bits.
        NONE = 0,
        ENFORCER = 1,
        BLOCKER = 2,
        // Maximum is 15. The value is serialized in TriangleSelector into 6 bits using a 2 bit prefix code.
        Extruder1 = ENFORCER,
        Extruder2 = BLOCKER,
        Extruder3,
        Extruder4,
        Extruder5,
        Extruder6,
        Extruder7,
        Extruder8,
        Extruder9,
        Extruder10,
        Extruder11,
        Extruder12,
        Extruder13,
        Extruder14,
        Extruder15,
        ExtruderMax
    };



    class SPREAD_API MeshSpreadWrapper
    {
    public:
        MeshSpreadWrapper();
        ~MeshSpreadWrapper();

        //初始化数据
        void setInputs(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer = nullptr);

        //分块数据
        void testChunk();
        int chunkCount();
        void chunk(int index, std::vector<trimesh::vec3>& positions, std::vector<int>& flags, std::vector<int>& splitIndices);

        void set_paint_on_overhangs_only(float angle_threshold_deg);

        //圆形
        void circile_factory(const trimesh::vec& center, const trimesh::vec3& camera_pos, float radius, int facet_start, int colorIndex
            , const trimesh::vec& normal, const float offset   //for ClippingPlane
            , std::vector<int>& dirty_chunks);
        void double_circile_factory(const trimesh::vec& center, const trimesh::vec& second_center, const trimesh::vec3& camera_pos,float radius, int facet_start, int colorIndex
            , const trimesh::vec& normal, const float offset   //for ClippingPlane
            , std::vector<int>& dirty_chunks);

        //填充、选面
        void bucket_fill_select_triangles_preview(const trimesh::vec& center, int facet_start, int colorIndex, std::vector<std::vector<trimesh::vec3>>& contour
            , const trimesh::vec& normal, const float offset   //for ClippingPlane
            , bool isFill=true);
        void bucket_fill_select_triangles(int colorIndex, std::vector<int>& dirty_chunks);
        //清除选中状态
        void seed_fill_unselect_all_triangles();

        //获取序列化数据
        void updateData();
        //解序列化数据
        void updateTriangle();

        //重置涂抹
        void reset();

        //获取碰撞点
        int getFacet(const trimesh::vec& point, trimesh::vec& direction, trimesh::vec& cross);

        //序列化数据存取
        std::string get_triangle_as_string(int triangle_idx) const;
        void set_triangle_from_string(int triangle_id, const std::string& str);
        std::vector<std::string> get_data_as_string() const;
        void set_triangle_from_data(std::vector<std::string> strList);
        
        //获取原始面ID
        int source_triangle_index(int index);
        //块数据转faceID
        int chunkId2FaceId(int chunkId, int index);

    private:
        void get_current_select_contours(std::vector<trimesh::vec3>& contour, const trimesh::vec3& offset = trimesh::vec3());
        void dirty_source_triangles_2_chunks(const std::vector<int>& dirty_source_triangls, std::vector<int>& chunks);
    private:
        std::unique_ptr <Slic3r::sla::IndexedMesh> m_emesh;
        std::unique_ptr<Slic3r::TriangleMesh> m_mesh;
        std::unique_ptr <Slic3r::TriangleSelector> m_triangle_selector;
        std::pair<std::vector<std::pair<int, int>>, std::vector<bool>> m_data;
        float m_highlight_by_angle_threshold_deg;

        std::vector<int> m_faceChunkIDs;
        std::vector<std::vector<int>> m_chunkFaces;
    };

}
#endif // MESH_SPREAD_1595984973500_H