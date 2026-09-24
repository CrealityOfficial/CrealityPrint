///|/ 复现「拆分后涂色镜像」问题的回归测试。
///|/
///|/ 背景
///|/ ----
///|/ TriangleMesh::split_and_save_relationship() 对 volume() < 0 的连通块调用
///|/ flip_triangles()，它交换每个面内部的顶点顺序 swap(face(1), face(2))。
///|/ 而涂色位流编码的是「相对三角面自身顶点编号」的细分结构，
///|/ ModelVolume::split() 里用 get/set_triangle_as_string() 逐字搬运时不感知
///|/ 这个交换，于是被细分过的面上涂色会沿轴镜像。
///|/
///|/ 关键点：只有**被细分过**的面才会出错。未细分的纯色面位流里只有一个状态
///|/ 位、没有结构信息，顶点顺序怎么换结果都一样。所以测试必须构造出带细分的
///|/ 涂色，这也是手工复现容易失败的原因。
///|/
#include <catch2/catch.hpp>

#include "libslic3r/libslic3r.h"
#include "libslic3r/Model.hpp"
#include "libslic3r/ModelVolume.hpp"
#include "libslic3r/TriangleMesh.hpp"
#include "libslic3r/TriangleSelector.hpp"

#include <algorithm>
#include <set>

using namespace Slic3r;

namespace {

// 以 (ox,oy,oz) 为最小角、边长 size 的立方体，绕序保证外法线朝外（volume > 0）。
indexed_triangle_set make_cube_its(float ox, float oy, float oz, float size)
{
    const float s = size;
    indexed_triangle_set its;
    its.vertices = {
        {ox,     oy,     oz    }, {ox + s, oy,     oz    },
        {ox + s, oy + s, oz    }, {ox,     oy + s, oz    },
        {ox,     oy,     oz + s}, {ox + s, oy,     oz + s},
        {ox + s, oy + s, oz + s}, {ox,     oy + s, oz + s},
    };
    its.indices = {
        {0, 3, 2}, {0, 2, 1},   // -Z
        {4, 5, 6}, {4, 6, 7},   // +Z
        {0, 1, 5}, {0, 5, 4},   // -Y
        {2, 3, 7}, {2, 7, 6},   // +Y
        {1, 2, 6}, {1, 6, 5},   // +X
        {3, 0, 4}, {3, 4, 7},   // -X
    };
    return its;
}

// 把 b 的顶点/面追加到 a，形成两个互不相连的连通块。
void append_its(indexed_triangle_set &a, const indexed_triangle_set &b)
{
    const int base = int(a.vertices.size());
    a.vertices.insert(a.vertices.end(), b.vertices.begin(), b.vertices.end());
    for (const Vec3i32 &f : b.indices)
        a.indices.emplace_back(f[0] + base, f[1] + base, f[2] + base);
}

// 反转绕序 —— 等价于 its_flip_triangles()，使该块 volume() 变负，
// 从而在 split() 里触发 flip_triangles()。
void flip_its(indexed_triangle_set &its)
{
    for (Vec3i32 &f : its.indices)
        std::swap(f[1], f[2]);
}

// 在指定面上造一个**带细分**的涂色。
//
// set_facet() 只能整面上色（位流里只有一个状态 nibble、没有结构），那样的数据
// 对顶点顺序不敏感，无法暴露本问题。select_patch() 能产生细分但依赖光标与相机，
// 不适合单测。所以这里走公开的 3MF 反序列化接口 set_triangle_from_string()，
// 直接喂一段「三边细分 + 只涂一个子面」的十六进制串。
//
// 串的格式由 get_triangle_as_string() / set_triangle_from_string() 定义：
//   * 每个 nibble 一位十六进制字符，**大写** A-F（小写会触发 assert）
//   * 字符串是**逆序**的：get_ 用 insert(begin()) 前插，set_ 用 crbegin() 反向读，
//     所以位流中的第一个 nibble 对应字符串的**最后一个**字符
//
// 我们要的位流（按解码顺序）是：
//   nibble0 = 0b0011 = 3   -> split_sides=3, special_side=0
//   nibble1 = 0b0000 = 0   -> children[3] = NONE   (serialize 倒序写子面)
//   nibble2 = 0b0000 = 0   -> children[2] = NONE
//   nibble3 = 0b0000 = 0   -> children[1] = NONE
//   nibble4..= 带色状态     -> children[0] = state
// 因此字符串 = reverse("3" + "0" + "0" + "0" + <state nibbles>)
std::string subdivided_paint_string(EnforcerBlockerType state)
{
    const int st = int(state);
    std::string bit_order;      // 按位流解码顺序排列的 nibble 字符
    bit_order += '3';           // split_sides=3, special_side=0
    bit_order += "000";         // children[3],[2],[1] = NONE

    static const char *HEX = "0123456789ABCDEF";
    if (st >= 3) {
        // 0b1100 标记（'C'）+ 一个 (st-3) 的 nibble。st-3 >= 15 需要链式扩展，
        // 本测试用的 Extruder2 不会走到。
        const int v = st - 3;
        assert(v < 15);
        bit_order += 'C';
        bit_order += HEX[v];
    } else {
        bit_order += HEX[st << 2];
    }

    // set_triangle_from_string() 反向读取，所以这里反转后返回。
    return std::string(bit_order.rbegin(), bit_order.rend());
}

void paint_subdivided(ModelVolume *volume, int facet_idx, EnforcerBlockerType state)
{
    volume->mmu_segmentation_facets.set_triangle_from_string(
        facet_idx, subdivided_paint_string(state));
}

// 统计一个 volume 上带涂色的面数，以及每个面的位流指纹。
std::map<int, std::string> paint_fingerprint(const ModelVolume *volume)
{
    std::map<int, std::string> out;
    const size_t face_count = volume->mesh().its.indices.size();
    for (size_t i = 0; i < face_count; ++i) {
        const std::string s = volume->mmu_segmentation_facets.get_triangle_as_string(int(i));
        if (! s.empty())
            out[int(i)] = s;
    }
    return out;
}

} // namespace

SCENARIO("Split preserves painting on parts with reversed winding", "[SplitPaint]")
{
    GIVEN("An object with two disconnected cubes, the second one wound inside-out")
    {
        // A 块正常且更大，B 块反向且较小 —— B 在 split() 里会触发 flip_triangles()。
        //
        // 两块大小必须不同：反向块贡献负体积，等大时对象总体积恰为 0，
        // 在 GUI 里会被 Model::removed_objects_with_zero_volume() 整个删掉
        // （弹窗「体积为零的对象已被移除」）。这里保持与手工测试用的 STL
        // 一致的 30mm / 20mm，使总体积为 +19000。
        indexed_triangle_set cube_a = make_cube_its(0.f, 0.f, 0.f, 30.f);
        indexed_triangle_set cube_b = make_cube_its(60.f, 0.f, 0.f, 20.f);
        flip_its(cube_b);

        indexed_triangle_set combined = cube_a;
        append_its(combined, cube_b);

        Model        model;
        ModelObject *object = model.add_object();
        ModelVolume *volume = object->add_volume(TriangleMesh(combined));
        object->add_instance();

        // 先确认前置条件，否则测试可能根本没触发到目标路径。
        THEN("the two components have opposite winding and a positive total volume")
        {
            std::vector<std::unordered_map<int, int>> relationships;
            std::vector<TriangleMesh> parts =
                TriangleMesh(combined).split_and_save_relationship(relationships);
            REQUIRE(parts.size() == 2);
            // split_and_save_relationship() 已把反向块翻正，所以它返回的两块都是正的。
            // 因此直接验证原始输入的绕序。
            TriangleMesh mesh_a(cube_a);
            TriangleMesh mesh_b(cube_b);
            REQUIRE(mesh_a.volume() > 0.f);   // 正常块
            REQUIRE(mesh_b.volume() < 0.f);   // 反向块 -> 会触发 flip_triangles()
            // 总体积必须为正，否则 GUI 导入时整个对象会被当作"体积为零"删除。
            TriangleMesh mesh_all(combined);
            REQUIRE(mesh_all.volume() > 0.f);
        }

        WHEN("a subdivided paint patch is applied to a face of the reversed cube")
        {
            // 取 B 块的第一个面（合并后索引 = A 块面数 + 0）。
            const int face_in_b = int(cube_a.indices.size());
            paint_subdivided(volume, face_in_b, EnforcerBlockerType::Extruder2);

            const std::map<int, std::string> before = paint_fingerprint(volume);
            REQUIRE(before.size() == 1);
            const std::string source_bits = before.begin()->second;
            // 位流必须包含细分结构，否则这个测试无法暴露问题。
            REQUIRE(source_bits.size() > 1);

            AND_WHEN("the volume is split into parts")
            {
                const size_t parts = volume->split(1);
                REQUIRE(parts == 2);
                REQUIRE(object->volumes.size() == 2);

                THEN("the paint bitstream is carried over unchanged")
                {
                    // 找到承载涂色的那个部件。
                    int painted_parts = 0;
                    std::string carried;
                    for (const ModelVolume *mv : object->volumes) {
                        const std::map<int, std::string> fp = paint_fingerprint(mv);
                        if (! fp.empty()) {
                            ++painted_parts;
                            carried = fp.begin()->second;
                        }
                    }
                    INFO("source bitstream : " << source_bits);
                    INFO("carried bitstream: " << carried);
                    CHECK(painted_parts == 1);
                    // 几何完全相同（拆分不改变顶点位置），所以涂色数据应逐字一致。
                    // 若 flip_triangles() 交换了顶点顺序而搬运未做补偿，
                    // 位流虽然字面相同，但解码出的子树朝向不同 —— 见下一个断言。
                    CHECK(carried == source_bits);
                }

                THEN("the painted area lands on the same physical location")
                {
                    // 更强的断言：把拆分后的涂色解码回具体的子三角面，
                    // 比较其重心是否与原始涂色位置一致。
                    // 这是真正能抓到镜像问题的检查。
                    auto painted_centroids = [](const ModelVolume *mv) {
                        std::vector<Vec3f> out;
                        if (mv->mmu_segmentation_facets.empty())
                            return out;
                        TriangleSelector sel(mv->mesh());
                        sel.deserialize(mv->mmu_segmentation_facets.get_data(), true);
                        const auto &tris  = sel.get_triangles();
                        const auto &verts = sel.get_vertices();
                        for (const auto &t : tris) {
                            if (! t.valid() || t.is_split())
                                continue;
                            if (t.get_state() == EnforcerBlockerType::NONE)
                                continue;
                            const Vec3f c = (verts[t.verts_idxs[0]].v +
                                             verts[t.verts_idxs[1]].v +
                                             verts[t.verts_idxs[2]].v) / 3.f;
                            out.emplace_back(c);
                        }
                        return out;
                    };

                    // 原始涂色位置（在合并网格上重新算一次，作为基准）。
                    Model        ref_model;
                    ModelObject *ref_object = ref_model.add_object();
                    ModelVolume *ref_volume = ref_object->add_volume(TriangleMesh(combined));
                    ref_object->add_instance();
                    paint_subdivided(ref_volume, face_in_b, EnforcerBlockerType::Extruder2);
                    const std::vector<Vec3f> expected = painted_centroids(ref_volume);
                    REQUIRE(! expected.empty());

                    // 拆分后的涂色位置。注意部件被 center_geometry_after_creation()
                    // 重新居中过，所以比较时要加回它的偏移。
                    std::vector<Vec3f> actual;
                    for (const ModelVolume *mv : object->volumes) {
                        if (mv->mmu_segmentation_facets.empty())
                            continue;
                        const Vec3d off = mv->get_offset();
                        for (const Vec3f &c : painted_centroids(mv))
                            actual.emplace_back(c + off.cast<float>());
                    }
                    REQUIRE(! actual.empty());

                    // 每个期望位置都应该能在实际结果里找到匹配（容差 0.01mm）。
                    for (const Vec3f &e : expected) {
                        const bool found = std::any_of(actual.begin(), actual.end(),
                            [&e](const Vec3f &a) { return (a - e).norm() < 0.01f; });
                        INFO("expected painted centroid " << e.x() << "," << e.y() << "," << e.z()
                             << " not found among " << actual.size() << " actual centroids");
                        CHECK(found);
                    }
                }
            }
        }
    }
}

SCENARIO("Split preserves painting on parts with normal winding (control)", "[SplitPaint]")
{
    GIVEN("An object with two disconnected cubes, both wound correctly")
    {
        // 与主测试用同样的尺寸，唯一差别是不翻转 —— 两块都是正体积，
        // 不会触发 flip_triangles()，用于隔离「拆分本身是否丢涂色」。
        indexed_triangle_set cube_a = make_cube_its(0.f, 0.f, 0.f, 30.f);
        indexed_triangle_set cube_b = make_cube_its(60.f, 0.f, 0.f, 20.f);

        indexed_triangle_set combined = cube_a;
        append_its(combined, cube_b);

        Model        model;
        ModelObject *object = model.add_object();
        ModelVolume *volume = object->add_volume(TriangleMesh(combined));
        object->add_instance();

        WHEN("a subdivided paint patch is applied and the volume is split")
        {
            const int face_in_b = int(cube_a.indices.size());
            paint_subdivided(volume, face_in_b, EnforcerBlockerType::Extruder2);
            const std::map<int, std::string> before = paint_fingerprint(volume);
            REQUIRE(before.size() == 1);
            const std::string source_bits = before.begin()->second;

            REQUIRE(volume->split(1) == 2);

            THEN("the paint is carried over unchanged (baseline behaviour)")
            {
                std::string carried;
                for (const ModelVolume *mv : object->volumes) {
                    const std::map<int, std::string> fp = paint_fingerprint(mv);
                    if (! fp.empty())
                        carried = fp.begin()->second;
                }
                INFO("source : " << source_bits);
                INFO("carried: " << carried);
                CHECK(carried == source_bits);
            }
        }
    }
}
