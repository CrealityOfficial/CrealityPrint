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


    class TrianglePatch;
    class TriangleNeighborState;

    class SPREAD_API MeshSpreadWrapper 
    {
    public:
        MeshSpreadWrapper();
        ~MeshSpreadWrapper();

        //初始化数据
        void setInputs(trimesh::TriMesh* mesh, ccglobal::Tracer* tracer = nullptr);
        //group接口
        void setGroupInputs(std::vector<trimesh::TriMesh*>m_groupmesh,ccglobal::Tracer* tracer=nullptr);


        //分块数据
        void testChunk();
        int chunkCount();
        void chunk(int index, std::vector<trimesh::vec3>& positions, std::vector<int>& flags, std::vector<int>& splitIndices);
        //专用于缝隙填充  flags_before为原始颜色，flags为将要涂的颜色
        void chunk_gap_fill(int index, std::vector<trimesh::vec3>& positions, std::vector<int>& flags,std::vector<int>& flags_before, std::vector<int>& splitIndices);

        void set_paint_on_overhangs_only(float angle_threshold_deg);

        //圆形
        void circile_factory(const trimesh::vec& center, const trimesh::vec3& camera_pos, float radius, int facet_start, int colorIndex
            , const trimesh::vec& normal, const float offset   //for ClippingPlane
            , std::vector<int>& dirty_chunks);
        void double_circile_factory(const trimesh::vec& center, const trimesh::vec& second_center, const trimesh::vec3& camera_pos,float radius, int facet_start, int colorIndex
            , const trimesh::vec& normal, const float offset   //for ClippingPlane
            , std::vector<int>& dirty_chunks);
        //高度填充
        //除了第三个参数是高度外，其他参数和圆形涂抹一样
        void height_factory(const trimesh::vec& center, const trimesh::vec3& camera_pos, float height, int facet_start, int colorIndex
            , const trimesh::vec& normal, const float offset
            , std::vector<int>& dirty_chunks);
        //球形涂抹
        void sphere_factory(const trimesh::vec& center, const trimesh::vec3& camera_pos, float radius, int facet_start, int colorIndex
            , const trimesh::vec& normal, const float offset
            , std::vector<int>& dirty_chunks);
        void double_sphere_factory(const trimesh::vec& center, const trimesh::vec& second_center, const trimesh::vec3& camera_pos, float radius, int facet_start, int colorIndex
            , const trimesh::vec& normal, const float offset   
            , std::vector<int>& dirty_chunks);
        //得到高度轮廓点
        void get_height_contour(const trimesh::vec& worldSpaceCenter, float height,std::vector<std::vector<trimesh::vec3>>& contour);

        //填充、选面、边沿填充
        //增加了一个是否为边沿检测的参数，true为启动边沿检测
        // 第三个bool参数专为支撑
        void bucket_fill_select_triangles_preview(const trimesh::vec& center, int facet_start, int colorIndex, std::vector<std::vector<trimesh::vec3>>& contour
            , const trimesh::vec& normal, const float offset   //for ClippingPlane
            , float seed_fill_angle = 30
            , bool isFill=false
            , bool isBorder=false
            , bool isSupport=false);
        void bucket_fill_select_triangles(int colorIndex, std::vector<int>& dirty_chunks);
        //清除选中状态
        void seed_fill_unselect_all_triangles();

        //所有三角面颜色转换
        void change_state_all_triangles(int ori_state,int des_state, std::vector<int>& dirty_chunks);


        //获取序列化数据
        void updateData();
        //解序列化数据
        void updateTriangle();

        //重置涂抹
        void reset();

        //获取碰撞点
        int getFacet(const trimesh::vec& point, trimesh::vec& direction, trimesh::vec& cross);
        // group  后两个int第一个返回mesh索引 第二个返回face索引
        void getFacet_Group(const trimesh::vec& point, trimesh::vec& direction, trimesh::vec& cross, int& mesh_index, int& face_index);

        //序列化数据存取
        std::string get_triangle_as_string(int triangle_idx) const;
        void set_triangle_from_string(int triangle_id, const std::string& str);
        std::vector<std::string> get_data_as_string() const;
        std::vector<std::vector<std::string>> get_date_as_groupstring() const;
        void set_triangle_from_data(std::vector<std::string> strList);
        //group
        void set_triangle_from_data_group(std::vector<std::vector<std::string>> strList_group);
        //获取原始面ID
        int source_triangle_index(int index);
        //块数据转faceID
        int chunkId2FaceId(int chunkId, int index);

        //计算patch面积
        float get_patch_area(const TrianglePatch& patch);
        //缝隙填充接口
        bool get_triangles_per_patch(float max_limit_area , std::vector<int>& dirty_chunks);
        //执行缝隙填充（无法回复）
        void apply_triangle_state(std::vector<int>& dirty_chunks);

        bool judge_hit(const trimesh::vec& point, trimesh::vec& direction);

        void clearBuffer();

        bool judge_select_triangles();

        void setMeshGlobalMatrix(const trimesh::xform& globalXf);
        trimesh::xform getMeshGlobalMatrix();

    private:
        void get_current_select_contours(std::vector<trimesh::vec3>& contour, const trimesh::vec3& offset = trimesh::vec3());
        void dirty_source_triangles_2_chunks(const std::vector<int>& dirty_source_triangls, std::vector<int>& chunks);
    private:
        std::unique_ptr <Slic3r::sla::IndexedMesh> m_emesh;
        std::unique_ptr<Slic3r::TriangleMesh> m_mesh;
        std::unique_ptr <Slic3r::TriangleSelector> m_triangle_selector;
        std::pair<std::vector<std::pair<int, int>>, std::vector<bool>> m_data;
        float m_highlight_by_angle_threshold_deg;

        std::vector<TrianglePatch> m_triangle_patches;
        std::vector<TriangleNeighborState>  m_triangle_virtual_state; 
        std::vector<int> last_triangle_change_state;
        std::vector<int> before_chunks;

        std::vector<int> m_faceChunkIDs;
        std::vector<std::vector<int>> m_chunkFaces;
     
        std::vector<int> group_faces_size;

        trimesh::xform m_meshGlobalMatrix;
    };

}
#endif // MESH_SPREAD_1595984973500_H