#include "TextureToColor.hpp"

#include <tbb/parallel_for.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <unordered_map>

#include <boost/next_prior.hpp>
#include "CgalUtils.hpp"
#include "ColorUtils.hpp"
#include "libslic3r/TriangleMesh.hpp"
#include <filesystem>
#include <fstream>
#include "Repair.hpp"
#include "libslic3r/AABBTreeIndirect.hpp"
#include <boost/log/trivial.hpp>

namespace Slic3r { namespace tex2color {

using namespace color_utils;

// #define OUTPUT_TEST_RESULT

static void SaveToOFF(const std::string& path, const TriMesh& mesh, const std::vector<RGB>& face_colors)
{
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        BOOST_LOG_TRIVIAL(warning) << "SaveToOFF: failed to open " << path;
        return;
    }

    const auto& vertices = mesh.vertices;
    const auto& faces = mesh.indices;

    ofs << "OFF\n";
    ofs << vertices.size() << " " << faces.size() << " 0\n";

    for (const auto& v : vertices) {
        ofs << v.x() << " " << v.y() << " " << v.z() << "\n";
    }

    for (std::size_t i = 0; i < faces.size(); ++i) {
        const auto& f = faces[i];
        ofs << "3 " << f[0] << " " << f[1] << " " << f[2];
        if (i < face_colors.size()) {
            ofs << " " << face_colors[i][0] / 255.0
                << " " << face_colors[i][1] / 255.0
                << " " << face_colors[i][2] / 255.0
                << " 1.0";
        }
        ofs << "\n";
    }
}

static std::vector<std::size_t> count_cluster_label_usage(const std::vector<std::size_t>& face_labels, std::size_t cluster_count)
{
    std::vector<std::size_t> usage(cluster_count, 0);
    for (std::size_t label : face_labels) {
        if (label < cluster_count) {
            ++usage[label];
        }
    }
    return usage;
}

static bool discard_unused_cluster_centers(std::vector<RGB>& cluster_centers, std::vector<std::size_t>& face_labels, const char* stage_name)
{
    if (cluster_centers.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "TextureToColor: cannot discard unused cluster centers at " << stage_name
                                   << ", no cluster center is available.";
        return false;
    }

    const std::vector<std::size_t> usage = count_cluster_label_usage(face_labels, cluster_centers.size());
    std::vector<std::size_t> label_remap(cluster_centers.size(), std::numeric_limits<std::size_t>::max());
    std::vector<RGB> used_cluster_centers;
    used_cluster_centers.reserve(cluster_centers.size());

    for (std::size_t cluster_id = 0; cluster_id < cluster_centers.size(); ++cluster_id) {
        if (usage[cluster_id] == 0) {
            continue;
        }
        label_remap[cluster_id] = used_cluster_centers.size();
        used_cluster_centers.push_back(cluster_centers[cluster_id]);
    }

    if (used_cluster_centers.size() == cluster_centers.size()) {
        return true;
    }
    if (used_cluster_centers.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "TextureToColor: cannot discard unused cluster centers at " << stage_name
                                   << ", no face uses any valid cluster center.";
        return false;
    }

    for (std::size_t& label : face_labels) {
        if (label >= label_remap.size() || label_remap[label] == std::numeric_limits<std::size_t>::max()) {
            BOOST_LOG_TRIVIAL(warning) << "TextureToColor: cannot remap cluster label " << label
                                       << " at " << stage_name << ".";
            return false;
        }
        label = label_remap[label];
    }

    BOOST_LOG_TRIVIAL(debug) << "TextureToColor: discarded " << (cluster_centers.size() - used_cluster_centers.size())
                             << " unused adaptive cluster centers at " << stage_name << ".";
    cluster_centers = std::move(used_cluster_centers);
    return true;
}

static bool ensure_all_cluster_centers_used(const std::vector<RGB>& source_face_colors, const std::vector<RGB>& cluster_centers,
                                           std::vector<std::size_t>& face_labels, const char* stage_name)
{
    if (source_face_colors.size() != face_labels.size()) {
        BOOST_LOG_TRIVIAL(warning) << "TextureToColor: cannot preserve cluster colors at " << stage_name
                                   << ", face color count does not match label count.";
        return false;
    }
    if (cluster_centers.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "TextureToColor: cannot preserve cluster colors at " << stage_name
                                   << ", no cluster center is available.";
        return false;
    }
    if (cluster_centers.size() > face_labels.size()) {
        BOOST_LOG_TRIVIAL(warning) << "TextureToColor: cannot use all cluster centers at " << stage_name
                                   << ", centers=" << cluster_centers.size() << " faces=" << face_labels.size() << ".";
        return false;
    }

    std::vector<std::size_t> usage = count_cluster_label_usage(face_labels, cluster_centers.size());
    std::size_t missing_count = 0;
    for (std::size_t cluster_id = 0; cluster_id < usage.size(); ++cluster_id) {
        if (usage[cluster_id] != 0) {
            continue;
        }
        ++missing_count;

        double best_cost = std::numeric_limits<double>::max();
        std::size_t best_face_id = std::numeric_limits<std::size_t>::max();
        std::size_t best_old_cluster_id = std::numeric_limits<std::size_t>::max();

        for (std::size_t fid = 0; fid < face_labels.size(); ++fid) {
            const std::size_t old_cluster_id = face_labels[fid];
            if (old_cluster_id >= cluster_centers.size() || usage[old_cluster_id] <= 1) {
                continue;
            }

            const double old_dist = calc_rgb_color_difference_by_ciede2000(source_face_colors[fid], cluster_centers[old_cluster_id]);
            const double new_dist = calc_rgb_color_difference_by_ciede2000(source_face_colors[fid], cluster_centers[cluster_id]);
            const double cost = new_dist - old_dist;
            if (cost < best_cost) {
                best_cost = cost;
                best_face_id = fid;
                best_old_cluster_id = old_cluster_id;
            }
        }

        if (best_face_id == std::numeric_limits<std::size_t>::max()) {
            BOOST_LOG_TRIVIAL(warning) << "TextureToColor: failed to assign a seed face for unused cluster " << cluster_id
                                       << " at " << stage_name << ".";
            continue;
        }

        face_labels[best_face_id] = cluster_id;
        --usage[best_old_cluster_id];
        ++usage[cluster_id];
    }

    if (missing_count > 0) {
        BOOST_LOG_TRIVIAL(debug) << "TextureToColor: reassigned seed faces for " << missing_count
                                 << " unused cluster centers at " << stage_name << ".";
    }

    for (std::size_t count : usage) {
        if (count == 0) {
            return false;
        }
    }
    return true;
}

// Bilinear interpolation texture sampling; sub-pixel precision avoids nearest-neighbor aliasing
static RGB get_pixel_color(float u, float v, const cv::Mat& texture) {
    u = u - std::floor(u);
    v = v - std::floor(v);

    // glTF UV convention: (0,0) = top-left, v increases downward
    float fx = u * (texture.cols - 1);
    float fy = v * (texture.rows - 1);

    int x0 = std::clamp(static_cast<int>(fx), 0, texture.cols - 1);
    int y0 = std::clamp(static_cast<int>(fy), 0, texture.rows - 1);
    int x1 = std::min(x0 + 1, texture.cols - 1);
    int y1 = std::min(y0 + 1, texture.rows - 1);

    float wx = fx - x0;
    float wy = fy - y0;

    const int ch = texture.channels();
    auto sample = [&](int row, int col) -> std::array<float, 3> {
        const uchar* ptr = texture.data + row * texture.step[0] + col * ch;
        return {static_cast<float>(ptr[2]), static_cast<float>(ptr[1]), static_cast<float>(ptr[0])};
    };

    auto c00 = sample(y0, x0);
    auto c10 = sample(y0, x1);
    auto c01 = sample(y1, x0);
    auto c11 = sample(y1, x1);

    // Bilinear blend: lerp(lerp(c00,c10,wx), lerp(c01,c11,wx), wy)
    RGB color;
    for (int i = 0; i < 3; ++i) {
        float top = c00[i] * (1.0f - wx) + c10[i] * wx;
        float bot = c01[i] * (1.0f - wx) + c11[i] * wx;
        color[i] = static_cast<std::size_t>(std::clamp(top * (1.0f - wy) + bot * wy, 0.0f, 255.0f));
    }
    return color;
}

// 7-point triangular Gaussian quadrature barycentric coordinates and weights (precision sufficient for capturing texture detail within faces)
static constexpr std::array<std::array<float, 3>, 7> GAUSS_TRI_BARY = {{
    {1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f},
    {0.059715871f, 0.470142064f, 0.470142064f},
    {0.470142064f, 0.059715871f, 0.470142064f},
    {0.470142064f, 0.470142064f, 0.059715871f},
    {0.797426985f, 0.101286507f, 0.101286507f},
    {0.101286507f, 0.797426985f, 0.101286507f},
    {0.101286507f, 0.101286507f, 0.797426985f},
}};
static constexpr std::array<float, 7> GAUSS_TRI_WEIGHT = {0.225f, 0.132394152f, 0.132394152f, 0.132394152f, 0.125939181f, 0.125939181f, 0.125939181f};
static_assert(
    []() constexpr {
        float sum = 0.0f;
        for (auto w : GAUSS_TRI_WEIGHT) {
            sum += w;
        }
        return sum > 0.999f && sum < 1.001f;
    }(),
    "Sum of Gaussian quadrature weights must be 1.0");

// Multi-point Gaussian quadrature sampling on a single face; returns weighted average color.
// GAUSS_TRI_WEIGHT sums to 1.0 (Hammer quadrature formula), no normalization needed.
static RGB sample_face_color(const std::array<Vec2f, 3>& uvs, const cv::Mat& texture) {
    float r = 0.0f, g = 0.0f, b = 0.0f;
    for (int k = 0; k < 7; ++k) {
        float u = GAUSS_TRI_BARY[k][0] * uvs[0].x() + GAUSS_TRI_BARY[k][1] * uvs[1].x() + GAUSS_TRI_BARY[k][2] * uvs[2].x();
        float v = GAUSS_TRI_BARY[k][0] * uvs[0].y() + GAUSS_TRI_BARY[k][1] * uvs[1].y() + GAUSS_TRI_BARY[k][2] * uvs[2].y();
        RGB c = get_pixel_color(u, v, texture);
        float w = GAUSS_TRI_WEIGHT[k];
        r += w * c[0];
        g += w * c[1];
        b += w * c[2];
    }
    return RGB{static_cast<std::size_t>(std::clamp(r, 0.0f, 255.0f)), static_cast<std::size_t>(std::clamp(g, 0.0f, 255.0f)),
               static_cast<std::size_t>(std::clamp(b, 0.0f, 255.0f))};
}

// Use array<Vec2f,3> instead of vector<Vec2f> for UV storage to avoid per-face heap allocations at million-face scale
using FaceUVArray = std::array<Vec2f, 3>;

static bool linear_subdivision(TriMesh& mesh, std::vector<FaceUVArray>& uv_coords, const std::function<void(int)>& sub_progress = nullptr) {
    const auto& original_vertices = mesh.vertices;
    const auto& original_faces = mesh.indices;
    TriVertices sub_vertices = mesh.vertices;
    sub_vertices.reserve(original_vertices.size() + original_faces.size() * 3);
    TriFaces sub_faces;
    std::vector<FaceUVArray> sub_uv_coords;

    // Single-level flat map with edge key encoding replaces nested unordered_map;
    // merges two vertex indices into a single uint64_t to reduce hash lookups and indirection.
    if (original_vertices.size() >= (1ULL << 32)) [[unlikely]] {
        BOOST_LOG_TRIVIAL(warning) << "[boundary] " << __FUNCTION__ << " vertex_count=" << original_vertices.size() << " exceeds 32-bit edge_key encoding range, skipping subdivision";
        return false;
    }
    auto edge_key = [](std::size_t a, std::size_t b) -> uint64_t {
        return a < b ? ((static_cast<uint64_t>(a) << 32) | b) : ((static_cast<uint64_t>(b) << 32) | a);
    };
    std::unordered_map<uint64_t, std::size_t> map_edge_to_sub_vtx;
    map_edge_to_sub_vtx.reserve(original_faces.size() * 3 / 2);

    for (const auto& face : original_faces) {
        for (std::size_t i = 0; i < 3; ++i) {
            std::size_t vtx_1 = face[i];
            std::size_t vtx_2 = face[(i + 1) % 3];
            uint64_t key = edge_key(vtx_1, vtx_2);
            if (map_edge_to_sub_vtx.count(key) > 0) {
                continue;
            }
            TriVertex edge_vtx = (original_vertices[vtx_1] + original_vertices[vtx_2]) * 0.5;
            map_edge_to_sub_vtx[key] = sub_vertices.size();
            sub_vertices.push_back(edge_vtx);
        }
    }
    if (sub_progress) {
        sub_progress(50);
    }

    // Subdivide faces and their UVs: each original face splits into 4 sub-faces (parallel writes, no contention)
    const std::size_t N = original_faces.size();
    sub_faces.resize(N * 4);
    sub_uv_coords.resize(N * 4);
    std::atomic<bool> has_missing_edge{false};

    tbb::parallel_for(tbb::blocked_range<std::size_t>(0, N), [&](const tbb::blocked_range<size_t>& range) {
        for (std::size_t fid = range.begin(); fid < range.end(); ++fid) {
            const std::size_t base = fid * 4;
            const auto& face = original_faces[fid];
            std::size_t vtx_0 = face[0];
            std::size_t vtx_1 = face[1];
            std::size_t vtx_2 = face[2];

            auto it01 = map_edge_to_sub_vtx.find(edge_key(vtx_0, vtx_1));
            auto it12 = map_edge_to_sub_vtx.find(edge_key(vtx_1, vtx_2));
            auto it20 = map_edge_to_sub_vtx.find(edge_key(vtx_2, vtx_0));
            if (it01 == map_edge_to_sub_vtx.end() || it12 == map_edge_to_sub_vtx.end() || it20 == map_edge_to_sub_vtx.end()) [[unlikely]] {
                has_missing_edge.store(true, std::memory_order_relaxed);
                Vec3i32 degen(vtx_0, vtx_0, vtx_0);
                FaceUVArray degen_uv = {uv_coords[fid][0], uv_coords[fid][0], uv_coords[fid][0]};
                for (int k = 0; k < 4; ++k) {
                    sub_faces[base + k] = degen;
                    sub_uv_coords[base + k] = degen_uv;
                }
                continue;
            }
            std::size_t e01 = it01->second;
            std::size_t e12 = it12->second;
            std::size_t e20 = it20->second;

            const Vec2f& uv0 = uv_coords[fid][0];
            const Vec2f& uv1 = uv_coords[fid][1];
            const Vec2f& uv2 = uv_coords[fid][2];
            Vec2f uv_e01 = (uv0 + uv1) * 0.5f;
            Vec2f uv_e12 = (uv1 + uv2) * 0.5f;
            Vec2f uv_e20 = (uv2 + uv0) * 0.5f;

            sub_faces[base + 0] = Vec3i32(vtx_0, e01, e20);
            sub_uv_coords[base + 0] = {uv0, uv_e01, uv_e20};

            sub_faces[base + 1] = Vec3i32(e01, vtx_1, e12);
            sub_uv_coords[base + 1] = {uv_e01, uv1, uv_e12};

            sub_faces[base + 2] = Vec3i32(e01, e12, e20);
            sub_uv_coords[base + 2] = {uv_e01, uv_e12, uv_e20};

            sub_faces[base + 3] = Vec3i32(e20, e12, vtx_2);
            sub_uv_coords[base + 3] = {uv_e20, uv_e12, uv2};
        }
    });
    // Remove degenerate triangles (three identical vertices) to avoid impacting downstream SDF / Remesh steps
    if (has_missing_edge.load(std::memory_order_relaxed)) {
        std::size_t write_idx = 0;
        for (std::size_t i = 0; i < sub_faces.size(); ++i) {
            if (sub_faces[i][0] == sub_faces[i][1] && sub_faces[i][1] == sub_faces[i][2]) {
                continue;
            }
            if (write_idx != i) {
                sub_faces[write_idx] = sub_faces[i];
                sub_uv_coords[write_idx] = sub_uv_coords[i];
            }
            ++write_idx;
        }
        BOOST_LOG_TRIVIAL(warning) << "[warning] linear_subdivision has missing edge vertex, removed " << (sub_faces.size() - write_idx) << " degenerate triangles";
        sub_faces.resize(write_idx);
        sub_uv_coords.resize(write_idx);
    }

    if (sub_progress) {
        sub_progress(100);
    }
    BOOST_LOG_TRIVIAL(debug) << "TextureToColor: input faces count = " << mesh.indices.size() << ".";
    mesh = TriMesh(sub_faces, sub_vertices);
    uv_coords = std::move(sub_uv_coords);
    BOOST_LOG_TRIVIAL(debug) << "TextureToColor: output faces count = " << mesh.indices.size() << ".";
    return true;
}

using VertexColor = std::array<float, 4>;

// Quantize continuous per-vertex colors into a small palette of cluster centers.
// The legacy OBJ vertex-color import consumed discrete filament ids, so split
// decisions could be made by comparing integers. Quantizing up front restores
// that property for the adaptive splitter below.
static bool quantize_vertex_colors(
    const std::vector<VertexColor>& vertex_colors,
    const TextureToColorSettings& settings,
    AlgoCancelCallback cancel_callback,
    std::vector<RGB>& out_centers,
    std::vector<std::size_t>& out_vertex_cluster_ids)
{
    out_centers.clear();
    out_vertex_cluster_ids.clear();
    if (vertex_colors.empty())
        return false;

    std::vector<RGB> vertex_rgb(vertex_colors.size());
    for (std::size_t i = 0; i < vertex_colors.size(); ++i) {
        for (int c = 0; c < 3; ++c) {
            float v = std::clamp(vertex_colors[i][c] * 255.0f, 0.0f, 255.0f);
            vertex_rgb[i][c] = static_cast<std::size_t>(v);
        }
    }

    ClusterParameters para;
    para.cancel_callback = cancel_callback ? [&]() { return cancel_callback(); } : std::function<bool()>{};
    if (settings.target_colors_num == 0) {
        para.max_color_distance = settings.max_color_distance;
        para.max_cluster_k      = settings.max_cluster_k;
        out_centers = cluster_adaptive(vertex_rgb, para);
    } else {
        para.cluster_k = settings.target_colors_num;
        out_centers = cluster_k_means(vertex_rgb, para);
    }
    if (out_centers.empty()) {
        BOOST_LOG_TRIVIAL(debug) << "quantize_vertex_colors: no cluster center generated.";
        return false;
    }

    out_vertex_cluster_ids.resize(vertex_rgb.size());
    for (std::size_t i = 0; i < vertex_rgb.size(); ++i) {
        std::size_t nearest_id = 0;
        if (!calc_nearest_color_id(out_centers, vertex_rgb[i], nearest_id))
            nearest_id = 0;
        out_vertex_cluster_ids[i] = nearest_id;
    }
    BOOST_LOG_TRIVIAL(debug) << "quantize_vertex_colors: quantized " << vertex_rgb.size()
                             << " vertex colors into " << out_centers.size() << " clusters.";
    return true;
}

// Single-level adaptive subdivision driven by per-vertex cluster ids.
//
// Reproduces the split topology that the legacy OBJ vertex-color import encoded
// into mmu_segmentation_facets (TriangleSelector::perform_split cases 1/2/3), but
// materializes it as real geometry. An edge is split at its midpoint if and only
// if its two endpoints belong to different clusters. Because that predicate reads
// only the shared endpoints, adjacent faces always reach the same conclusion and
// no T-junctions can appear.
static bool adaptive_split_by_vertex_clusters(
    TriMesh& mesh,
    const std::vector<std::size_t>& vertex_cluster_ids,
    const std::vector<RGB>& cluster_centers,
    std::vector<RGB>& out_face_colors)
{
    const TriVertices original_vertices = mesh.vertices;
    const TriFaces    original_faces    = mesh.indices;
    if (original_vertices.empty() || original_faces.empty() || cluster_centers.empty())
        return false;
    if (vertex_cluster_ids.size() != original_vertices.size()) {
        BOOST_LOG_TRIVIAL(warning) << "adaptive_split_by_vertex_clusters: cluster id count ("
                                   << vertex_cluster_ids.size() << ") != vertex count ("
                                   << original_vertices.size() << ").";
        return false;
    }
    if (original_vertices.size() >= (1ULL << 32)) [[unlikely]] {
        BOOST_LOG_TRIVIAL(warning) << "adaptive_split_by_vertex_clusters: vertex_count="
                                   << original_vertices.size() << " exceeds 32-bit edge_key range.";
        return false;
    }

    TriVertices out_vertices = original_vertices;
    TriFaces    out_faces;
    out_faces.reserve(original_faces.size() * 5);
    out_face_colors.clear();
    out_face_colors.reserve(original_faces.size() * 5);

    auto edge_key = [](std::size_t a, std::size_t b) -> uint64_t {
        return a < b ? ((static_cast<uint64_t>(a) << 32) | b)
                     : ((static_cast<uint64_t>(b) << 32) | a);
    };
    std::unordered_map<uint64_t, std::size_t> edge_to_mid;
    edge_to_mid.reserve(original_faces.size() * 3 / 2);

    // Midpoints on shared edges must be deduplicated so that neighbouring faces
    // reference the same vertex instead of coincident duplicates.
    auto midpoint_of_edge = [&](std::size_t a, std::size_t b) -> std::size_t {
        const uint64_t key = edge_key(a, b);
        auto it = edge_to_mid.find(key);
        if (it != edge_to_mid.end())
            return it->second;
        const std::size_t idx = out_vertices.size();
        out_vertices.push_back((original_vertices[a] + original_vertices[b]) * 0.5f);
        edge_to_mid.emplace(key, idx);
        return idx;
    };
    // Points strictly inside an original face are never shared, so they skip the map.
    // The midpoint is computed before push_back so a reallocation cannot dangle it.
    auto append_interior_midpoint = [&](std::size_t a, std::size_t b) -> std::size_t {
        const TriVertex mid = (out_vertices[a] + out_vertices[b]) * 0.5f;
        const std::size_t idx = out_vertices.size();
        out_vertices.push_back(mid);
        return idx;
    };
    auto emit = [&](std::size_t a, std::size_t b, std::size_t c, std::size_t cluster_id) {
        out_faces.push_back(Vec3i32(static_cast<int>(a), static_cast<int>(b), static_cast<int>(c)));
        out_face_colors.push_back(cluster_centers[cluster_id]);
    };

    for (const auto& f : original_faces) {
        const std::size_t v[3] = {static_cast<std::size_t>(f[0]), static_cast<std::size_t>(f[1]), static_cast<std::size_t>(f[2])};
        const std::size_t c[3] = {vertex_cluster_ids[v[0]], vertex_cluster_ids[v[1]], vertex_cluster_ids[v[2]]};

        // Case A: uniform cluster, keep the face untouched.
        if (c[0] == c[1] && c[1] == c[2]) {
            emit(v[0], v[1], v[2], c[0]);
            continue;
        }

        // Case B: two vertices share a cluster and the third is isolated. Split the
        // two edges incident to the isolated vertex, which are exactly the
        // cross-cluster ones; the opposite edge stays intact.
        int iso = -1;
        if (c[1] == c[2])      iso = 0;
        else if (c[2] == c[0]) iso = 1;
        else if (c[0] == c[1]) iso = 2;
        if (iso >= 0) {
            const int i = iso, j = (iso + 1) % 3, k = (iso + 2) % 3;
            const std::size_t m_ij = midpoint_of_edge(v[i], v[j]);
            const std::size_t m_ki = midpoint_of_edge(v[k], v[i]);
            emit(v[i], m_ij, m_ki, c[i]);
            emit(m_ij, v[j], m_ki, c[j]);
            emit(v[j], v[k],  m_ki, c[j]);
            continue;
        }

        // Case C: all three clusters differ. Split every edge, then cut the centre
        // triangle once more. The centre is equidistant from all three clusters, so
        // the legacy heuristic selects the cut by widest interior angle, which is
        // the vertex opposite the longest edge.
        const std::size_t m01 = midpoint_of_edge(v[0], v[1]);
        const std::size_t m12 = midpoint_of_edge(v[1], v[2]);
        const std::size_t m20 = midpoint_of_edge(v[2], v[0]);
        emit(v[0], m01, m20, c[0]);
        emit(m01, v[1], m12, c[1]);
        emit(m12, v[2], m20, c[2]);

        const TriVertex& p0 = original_vertices[v[0]];
        const TriVertex& p1 = original_vertices[v[1]];
        const TriVertex& p2 = original_vertices[v[2]];
        const float sq_opposite_v0 = (p2 - p1).squaredNorm();
        const float sq_opposite_v1 = (p0 - p2).squaredNorm();
        const float sq_opposite_v2 = (p1 - p0).squaredNorm();
        int   widest     = 0;
        float widest_len = sq_opposite_v0;
        if (sq_opposite_v1 > widest_len) { widest = 1; widest_len = sq_opposite_v1; }
        if (sq_opposite_v2 > widest_len) { widest = 2; }

        if (widest == 0) {
            const std::size_t mc = append_interior_midpoint(m20, m01);
            emit(m12, m20, mc,  c[1]);
            emit(mc,  m01, m12, c[2]);
        } else if (widest == 1) {
            const std::size_t mc = append_interior_midpoint(m01, m12);
            emit(m20, m01, mc,  c[0]);
            emit(mc,  m12, m20, c[2]);
        } else {
            const std::size_t mc = append_interior_midpoint(m12, m20);
            emit(m01, m12, mc,  c[1]);
            emit(mc,  m20, m01, c[0]);
        }
    }

    BOOST_LOG_TRIVIAL(info) << "adaptive_split_by_vertex_clusters: faces " << original_faces.size()
                            << " -> " << out_faces.size() << ", vertices " << original_vertices.size()
                            << " -> " << out_vertices.size();
    mesh = TriMesh(out_faces, out_vertices);
    return true;
}

// A validated pre-Windows candidate may normalize moderately damaged boundary components and
// try larger cycles. The normal RepairMesh default remains unchanged for all other callers and
// fallback paths.
static constexpr std::size_t MAX_PRE_WINDOWS_REPAIR_HOLE_EDGES = 1024;

// Large raw boundary counts may come from duplicated vertices along otherwise closed seams.
// Keep this candidate deliberately narrow: it is normalization-only (no hole filling), and
// the result still has to pass the same strict topology/bbox validation as the bounded repair.
static constexpr std::size_t MAX_PRE_WINDOWS_SEAM_NORMALIZATION_OPEN_EDGES = 100000;
static constexpr std::size_t MIN_FACES_PER_SEAM_NORMALIZATION_OPEN_EDGE = 20;
static constexpr double MIN_PAIRED_SEAM_BOUNDARY_RATIO = 0.80;
static constexpr double MAX_PRE_WINDOWS_REPAIR_FACE_CHANGE_RATIO = 0.01;

struct BoundaryPreflightStats
{
    std::size_t total_edges = 0;
    std::size_t component_count = 0;
    std::size_t max_component_edges = 0;
    std::size_t non_cycle_vertices = 0;
    std::size_t max_vertex_degree = 0;
    bool all_components_are_cycles = true;
};

struct BoundaryGeometryEdgeKey
{
    std::array<std::uint32_t, 3> first;
    std::array<std::uint32_t, 3> second;

    bool operator==(const BoundaryGeometryEdgeKey& rhs) const
    {
        return first == rhs.first && second == rhs.second;
    }
};

struct BoundaryGeometryEdgeKeyHash
{
    std::size_t operator()(const BoundaryGeometryEdgeKey& key) const
    {
        std::size_t seed = 0xcbf29ce484222325ULL;
        auto mix = [&seed](std::uint32_t value) {
            seed ^= static_cast<std::size_t>(value) + 0x9e3779b9U + (seed << 6) + (seed >> 2);
        };
        for (std::uint32_t value : key.first)
            mix(value);
        for (std::uint32_t value : key.second)
            mix(value);
        return seed;
    }
};

struct SeamNormalizationPreflightStats
{
    std::size_t total_edges = 0;
    std::size_t geometrically_paired_edges = 0;
    std::size_t duplicate_edge_keys = 0;
    std::size_t max_edge_multiplicity = 0;
};

static std::array<std::uint32_t, 3> vertex_coordinate_bits(const Vec3f& vertex)
{
    std::array<std::uint32_t, 3> bits{};
    for (std::size_t axis = 0; axis < bits.size(); ++axis) {
        // Treat +0 and -0 as the same coordinate, matching geometric equality.
        const float coordinate = vertex[axis] == 0.0f ? 0.0f : vertex[axis];
        std::memcpy(&bits[axis], &coordinate, sizeof(coordinate));
    }
    return bits;
}

struct VertexCoordinateBitsHash
{
    std::size_t operator()(const std::array<std::uint32_t, 3>& bits) const
    {
        std::size_t seed = 0xcbf29ce484222325ULL;
        for (std::uint32_t value : bits)
            seed ^= static_cast<std::size_t>(value) + 0x9e3779b9U + (seed << 6) + (seed >> 2);
        return seed;
    }
};

struct ExactVertexWeldStats
{
    std::size_t merged_vertices = 0;
    std::size_t degenerate_faces = 0;
    std::size_t invalid_face_indices = 0;
};

// Merge vertices with bit-identical coordinates while preserving face order and count. Because
// face ids do not change, per-face texture colors remain valid and need no nearest-face resample.
static bool weld_exact_duplicate_vertices(
    const TriMesh& mesh, TriMesh& out_mesh, ExactVertexWeldStats& stats)
{
    if (mesh.vertices.empty() || mesh.indices.empty() ||
        mesh.vertices.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;

    std::unordered_map<std::array<std::uint32_t, 3>, int, VertexCoordinateBitsHash> unique_vertices;
    unique_vertices.reserve(mesh.vertices.size());
    TriVertices welded_vertices;
    welded_vertices.reserve(mesh.vertices.size());
    std::vector<int> vertex_remap(mesh.vertices.size(), -1);

    for (std::size_t vertex_idx = 0; vertex_idx < mesh.vertices.size(); ++vertex_idx) {
        const auto bits = vertex_coordinate_bits(mesh.vertices[vertex_idx]);
        const int next_index = static_cast<int>(welded_vertices.size());
        const auto [it, inserted] = unique_vertices.emplace(bits, next_index);
        if (inserted)
            welded_vertices.push_back(mesh.vertices[vertex_idx]);
        else
            ++stats.merged_vertices;
        vertex_remap[vertex_idx] = it->second;
    }

    TriFaces welded_faces;
    welded_faces.reserve(mesh.indices.size());
    for (const TriFace& face : mesh.indices) {
        TriFace welded_face = face;
        bool valid = true;
        for (int corner = 0; corner < 3; ++corner) {
            if (face[corner] < 0 || static_cast<std::size_t>(face[corner]) >= vertex_remap.size()) {
                ++stats.invalid_face_indices;
                valid = false;
                break;
            }
            welded_face[corner] = vertex_remap[face[corner]];
        }
        if (!valid)
            continue;
        if (welded_face[0] == welded_face[1] || welded_face[1] == welded_face[2] ||
            welded_face[2] == welded_face[0])
            ++stats.degenerate_faces;
        welded_faces.push_back(welded_face);
    }

    if (stats.merged_vertices == 0 || stats.degenerate_faces != 0 ||
        stats.invalid_face_indices != 0 || welded_faces.size() != mesh.indices.size())
        return false;

    out_mesh = TriMesh(std::move(welded_faces), std::move(welded_vertices));
    return true;
}

// Count raw boundary edges that have another boundary edge with exactly the same geometric
// endpoints. This cheaply distinguishes duplicated OBJ seams from tens of thousands of real
// hole edges before paying for a full polygon-soup normalization pass.
static SeamNormalizationPreflightStats seam_normalization_preflight_stats(
    const TriMesh& mesh, const std::vector<Vec3i32>& face_neighbors, std::size_t open_edges)
{
    SeamNormalizationPreflightStats stats;
    if (face_neighbors.size() != mesh.indices.size())
        return stats;

    std::unordered_map<BoundaryGeometryEdgeKey, std::size_t, BoundaryGeometryEdgeKeyHash> edge_counts;
    edge_counts.reserve(2 * open_edges);
    for (std::size_t face_idx = 0; face_idx < face_neighbors.size(); ++face_idx) {
        for (int edge_idx = 0; edge_idx < 3; ++edge_idx) {
            if (face_neighbors[face_idx][edge_idx] >= 0)
                continue;
            const auto edge = its_triangle_edge(mesh.indices[face_idx], edge_idx);
            auto first = vertex_coordinate_bits(mesh.vertices[edge[0]]);
            auto second = vertex_coordinate_bits(mesh.vertices[edge[1]]);
            if (second < first)
                std::swap(first, second);
            ++edge_counts[{first, second}];
            ++stats.total_edges;
        }
    }

    for (const auto& [edge, count] : edge_counts) {
        (void) edge;
        if (count > 1) {
            ++stats.duplicate_edge_keys;
            stats.geometrically_paired_edges += count - count % 2;
        }
        stats.max_edge_multiplicity = std::max(stats.max_edge_multiplicity, count);
    }
    return stats;
}

// Measure the raw boundary graph before copying and repairing a multi-million-triangle mesh.
// Branching/non-manifold boundaries are allowed only on this bounded candidate path: RepairMesh
// first normalizes its polygon soup, and the repaired result is accepted only after strict checks.
static BoundaryPreflightStats boundary_preflight_stats(
    const TriMesh& mesh, const std::vector<Vec3i32>& face_neighbors)
{
    BoundaryPreflightStats stats;
    if (face_neighbors.size() != mesh.indices.size())
        return stats;

    std::unordered_map<int, std::size_t> vertex_nodes;
    vertex_nodes.reserve(2 * std::min<std::size_t>(MAX_REPAIRABLE_MESH_BOUNDARY_EDGES,
                                                   its_num_open_edges(face_neighbors)));
    std::vector<std::size_t> parent;
    std::vector<std::size_t> degrees;
    std::vector<std::pair<std::size_t, std::size_t>> boundary_edges;

    auto node_for = [&](int vertex) {
        const auto [it, inserted] = vertex_nodes.emplace(vertex, parent.size());
        if (inserted) {
            parent.push_back(it->second);
            degrees.push_back(0);
        }
        return it->second;
    };
    auto find_root = [&](std::size_t node) {
        while (parent[node] != node) {
            parent[node] = parent[parent[node]];
            node = parent[node];
        }
        return node;
    };
    auto unite = [&](std::size_t lhs, std::size_t rhs) {
        lhs = find_root(lhs);
        rhs = find_root(rhs);
        if (lhs != rhs)
            parent[rhs] = lhs;
    };

    for (std::size_t face_idx = 0; face_idx < face_neighbors.size(); ++face_idx) {
        for (int edge_idx = 0; edge_idx < 3; ++edge_idx) {
            if (face_neighbors[face_idx][edge_idx] >= 0)
                continue;
            const auto edge = its_triangle_edge(mesh.indices[face_idx], edge_idx);
            const std::size_t lhs = node_for(edge[0]);
            const std::size_t rhs = node_for(edge[1]);
            ++degrees[lhs];
            ++degrees[rhs];
            unite(lhs, rhs);
            boundary_edges.emplace_back(lhs, rhs);
        }
    }

    stats.total_edges = boundary_edges.size();
    std::vector<std::size_t> component_edges(parent.size(), 0);
    for (const auto& edge : boundary_edges)
        ++component_edges[find_root(edge.first)];
    for (std::size_t count : component_edges) {
        if (count == 0)
            continue;
        ++stats.component_count;
        stats.max_component_edges = std::max(stats.max_component_edges, count);
    }
    for (std::size_t degree : degrees) {
        stats.max_vertex_degree = std::max(stats.max_vertex_degree, degree);
        if (degree != 2)
            ++stats.non_cycle_vertices;
    }
    stats.all_components_are_cycles = stats.non_cycle_vertices == 0;
    return stats;
}

// Shared pipeline: mesh repair -> color clustering -> label assignment -> smoothing.
// Called by both TextureToColor (after UV sampling) and ClusterAndSmooth (after vertex-color oversample).
// progress_callback reports 0~100 within this function; the caller maps it to its own global range.
static bool repair_cluster_smooth(
    TriMesh& mesh,
    std::vector<RGB>& face_colors,
    std::vector<RGB>& out_clustered_face_colors,
    const TextureToColorSettings& settings,
    AlgoProgressCallback progress_callback,
    AlgoCancelCallback cancel_callback,
    const char* log_prefix,
    PreparedTextureColorData* prepared_output)
{
    using Clock = std::chrono::steady_clock;
    const auto total_started_at = Clock::now();
    auto stage_started_at = total_started_at;
    auto log_stage = [&](const char* stage) {
        const auto now = Clock::now();
        BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix << " stage=" << stage
                                << " elapsed_ms="
                                << std::chrono::duration<double, std::milli>(now - stage_started_at).count()
                                << " faces=" << mesh.facets_count();
        stage_started_at = now;
    };

    auto report = [&](int pct, const char* msg) {
        if (progress_callback)
            progress_callback({pct, msg});
    };
    auto cancelled = [&]() -> bool {
        if (cancel_callback && cancel_callback()) {
            BOOST_LOG_TRIVIAL(debug) << log_prefix << " cancelled";
            return true;
        }
        return false;
    };

    report(0, "Repairing mesh");
    if (cancelled()) return false;

    // Resample face colors onto a repaired mesh via centroid nearest-neighbor.
    auto resample_face_colors = [&](TriMesh&& repaired_mesh) -> bool {
        const auto resample_started_at = Clock::now();
        TriVertices old_vertices = std::move(mesh.vertices);
        TriFaces    old_indices  = std::move(mesh.indices);
        const size_t old_vertex_count = old_vertices.size();
        const size_t old_face_count   = old_indices.size();
        const size_t new_vertex_count = repaired_mesh.vertices.size();
        const size_t new_face_count   = repaired_mesh.indices.size();

        const auto aabb_started_at = Clock::now();
        auto aabb_tree = AABBTreeIndirect::build_aabb_tree_over_indexed_triangle_set(old_vertices, old_indices);
        BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                   << " stage=repair_resample_aabb_build elapsed_ms="
                                   << std::chrono::duration<double, std::milli>(Clock::now() - aabb_started_at).count()
                                   << " old_vertices=" << old_vertex_count
                                   << " old_faces=" << old_face_count;
        mesh = std::move(repaired_mesh);

        BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                   << " stage=repaired_mesh_closed_check skipped=true reason=diagnostic_only"
                                   << " new_vertices=" << new_vertex_count
                                   << " new_faces=" << new_face_count;

        std::vector<RGB> new_face_colors(mesh.facets_count());
        const auto query_started_at = Clock::now();
        tbb::parallel_for(tbb::blocked_range<std::size_t>(0, mesh.facets_count()), [&](const tbb::blocked_range<size_t>& range) {
            for (std::size_t fid = range.begin(); fid < range.end(); ++fid) {
                const auto& face = mesh.indices[fid];
                Vec3f center = (mesh.vertices[face[0]] + mesh.vertices[face[1]] + mesh.vertices[face[2]]) / 3.0f;
                size_t hit_idx = 0;
                Vec3f  closest;
                AABBTreeIndirect::squared_distance_to_indexed_triangle_set(
                    old_vertices, old_indices, aabb_tree, center, hit_idx, closest);
                new_face_colors[fid] = face_colors[hit_idx];
            }
        });
        BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                   << " stage=repair_resample_nearest_face_queries elapsed_ms="
                                   << std::chrono::duration<double, std::milli>(Clock::now() - query_started_at).count()
                                   << " source_faces=" << old_face_count
                                   << " query_faces=" << new_face_count;
        face_colors = std::move(new_face_colors);
        BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                   << " stage=repair_resample_total elapsed_ms="
                                   << std::chrono::duration<double, std::milli>(Clock::now() - resample_started_at).count()
                                   << " old_vertices=" << old_vertex_count
                                   << " old_faces=" << old_face_count
                                   << " new_vertices=" << new_vertex_count
                                   << " new_faces=" << new_face_count;
        return true;
    };

    auto run_internal_repair = [&](const char* source, const RepairSetting& repair_setting) -> std::shared_ptr<TriMesh> {
        const size_t input_vertices = mesh.vertices.size();
        const size_t input_faces    = mesh.indices.size();
        const auto repair_started_at = Clock::now();
        std::shared_ptr<TriMesh> repaired_mesh;
        const bool repaired = RepairMesh(
            mesh, repaired_mesh,
            [&](AlgoProgress repair_progress) { report(5, repair_progress.message); },
            [&]() { return cancelled(); }, repair_setting);
        BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                   << " stage=internal_repair_mesh elapsed_ms="
                                   << std::chrono::duration<double, std::milli>(Clock::now() - repair_started_at).count()
                                   << " source=" << source
                                   << " success=" << repaired
                                   << " max_hole_edges=" << repair_setting.max_hole_edges
                                   << " max_boundary_edges=" << repair_setting.max_boundary_edges
                                   << " input_vertices=" << input_vertices
                                   << " input_faces=" << input_faces
                                   << " output_vertices=" << (repaired_mesh ? repaired_mesh->vertices.size() : 0)
                                   << " output_faces=" << (repaired_mesh ? repaired_mesh->indices.size() : 0);
        if (!repaired || !repaired_mesh) {
            BOOST_LOG_TRIVIAL(debug) << log_prefix << ": RepairMesh failed.";
            return {};
        }
        return repaired_mesh;
    };

    auto repair_and_resample = [&]() -> bool {
        std::shared_ptr<TriMesh> repaired_mesh = run_internal_repair("halfedge_fallback", RepairSetting{});
        if (!repaired_mesh)
            return false;
        if (cancelled()) return false;
        return resample_face_colors(std::move(*repaired_mesh));
    };

    auto validate_internal_repair = [&](const TriMesh& candidate, const char* source) -> bool {
        const auto validation_started_at = Clock::now();
        if (candidate.vertices.empty() || candidate.indices.empty())
            return false;

        bool finite_vertices = true;
        Vec3f source_min   = Vec3f::Constant(std::numeric_limits<float>::max());
        Vec3f source_max   = Vec3f::Constant(std::numeric_limits<float>::lowest());
        Vec3f repaired_min = source_min;
        Vec3f repaired_max = source_max;
        for (const Vec3f& vertex : mesh.vertices) {
            source_min = source_min.cwiseMin(vertex);
            source_max = source_max.cwiseMax(vertex);
        }
        for (const Vec3f& vertex : candidate.vertices) {
            finite_vertices = finite_vertices && std::isfinite(vertex.x()) &&
                              std::isfinite(vertex.y()) && std::isfinite(vertex.z());
            repaired_min = repaired_min.cwiseMin(vertex);
            repaired_max = repaired_max.cwiseMax(vertex);
        }

        const auto repaired_neighbors = its_face_neighbors_par(static_cast<const indexed_triangle_set&>(candidate));
        const std::size_t repaired_open_edges = its_num_open_edges(repaired_neighbors);
        const bool halfedge_compatible = repaired_open_edges == 0 &&
            cgalutils::is_mesh_halfedge_compatible(candidate);
        const double source_diag = (source_max - source_min).cast<double>().norm();
        const double center_drift = (((repaired_min + repaired_max) - (source_min + source_max)) * 0.5f)
                                        .cast<double>().norm();
        const double size_delta = ((repaired_max - repaired_min) - (source_max - source_min))
                                      .cast<double>().norm();
        const double bbox_tolerance = 0.01 * std::max(1.0, source_diag);
        const double face_change_ratio = std::abs(
            static_cast<double>(candidate.indices.size()) - static_cast<double>(mesh.indices.size())) /
            static_cast<double>(std::max<std::size_t>(1, mesh.indices.size()));
        const bool accepted = finite_vertices && halfedge_compatible &&
            center_drift <= bbox_tolerance && size_delta <= bbox_tolerance &&
            face_change_ratio <= MAX_PRE_WINDOWS_REPAIR_FACE_CHANGE_RATIO;

        BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                   << " stage=internal_repair_fast_path_validation elapsed_ms="
                                   << std::chrono::duration<double, std::milli>(Clock::now() - validation_started_at).count()
                                   << " source=" << source
                                   << " accepted=" << accepted
                                   << " finite_vertices=" << finite_vertices
                                   << " open_edges=" << repaired_open_edges
                                   << " halfedge_compatible=" << halfedge_compatible
                                   << " center_drift=" << center_drift
                                   << " size_delta=" << size_delta
                                   << " bbox_tolerance=" << bbox_tolerance
                                   << " face_change_ratio=" << face_change_ratio
                                   << " face_change_limit=" << MAX_PRE_WINDOWS_REPAIR_FACE_CHANGE_RATIO
                                   << " vertices=" << candidate.vertices.size()
                                   << " faces=" << candidate.indices.size();
        return accepted;
    };

    if (!settings.mesh_prevalidated) {
        bool mesh_known_halfedge_compatible = false;
        if (settings.mesh_repair_decision != MeshRepairDecision::ImportWithoutRepair) {
            const auto mesh_stats_started_at = Clock::now();
            const auto face_neighbors = its_face_neighbors_par(static_cast<const indexed_triangle_set&>(mesh));
            const std::size_t open_edges = its_num_open_edges(face_neighbors);
            BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix << " stage=mesh_open_edge_check elapsed_ms="
                                       << std::chrono::duration<double, std::milli>(Clock::now() - mesh_stats_started_at).count()
                                       << " faces=" << mesh.facets_count() << " open_edges=" << open_edges;
            if (open_edges != 0) {
                BOOST_LOG_TRIVIAL(info) << log_prefix << ": mesh has non-manifold geometry or open boundaries, open_edges="
                                        << open_edges;
                if (settings.mesh_repair_decision == MeshRepairDecision::Ask) {
                    if (settings.mesh_repair_decision_required)
                        *settings.mesh_repair_decision_required = true;
                    return false;
                }
                if (settings.mesh_repair_decision == MeshRepairDecision::RepairAndImport) {
                    bool internal_fast_path_succeeded = false;
                    BoundaryPreflightStats boundary_stats;
                    if (open_edges <= MAX_REPAIRABLE_MESH_BOUNDARY_EDGES)
                        boundary_stats = boundary_preflight_stats(mesh, face_neighbors);
                    const bool internal_fast_path_eligible =
                        boundary_stats.total_edges == open_edges &&
                        boundary_stats.max_component_edges <= MAX_PRE_WINDOWS_REPAIR_HOLE_EDGES;
                    const bool seam_candidate_size_eligible =
                        open_edges > MAX_REPAIRABLE_MESH_BOUNDARY_EDGES &&
                        open_edges <= MAX_PRE_WINDOWS_SEAM_NORMALIZATION_OPEN_EDGES &&
                        open_edges <= mesh.facets_count() / MIN_FACES_PER_SEAM_NORMALIZATION_OPEN_EDGE;
                    SeamNormalizationPreflightStats seam_stats;
                    if (seam_candidate_size_eligible)
                        seam_stats = seam_normalization_preflight_stats(mesh, face_neighbors, open_edges);
                    const double paired_boundary_ratio = seam_stats.total_edges == 0 ? 0.0 :
                        static_cast<double>(seam_stats.geometrically_paired_edges) /
                        static_cast<double>(seam_stats.total_edges);
                    const bool seam_normalization_eligible =
                        seam_candidate_size_eligible &&
                        seam_stats.total_edges == open_edges &&
                        paired_boundary_ratio >= MIN_PAIRED_SEAM_BOUNDARY_RATIO;
                    BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                               << " stage=internal_repair_fast_path_preflight eligible="
                                               << internal_fast_path_eligible
                                               << " total_boundary_edges=" << boundary_stats.total_edges
                                               << " component_count=" << boundary_stats.component_count
                                               << " max_component_edges=" << boundary_stats.max_component_edges
                                               << " all_components_are_cycles=" << boundary_stats.all_components_are_cycles
                                               << " non_cycle_vertices=" << boundary_stats.non_cycle_vertices
                                               << " max_vertex_degree=" << boundary_stats.max_vertex_degree
                                               << " requires_boundary_normalization=" << !boundary_stats.all_components_are_cycles
                                               << " default_component_limit=" << MAX_REPAIRABLE_MESH_HOLE_EDGES
                                               << " candidate_component_limit=" << MAX_PRE_WINDOWS_REPAIR_HOLE_EDGES;
                    BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                               << " stage=seam_normalization_preflight eligible="
                                               << seam_normalization_eligible
                                               << " size_eligible=" << seam_candidate_size_eligible
                                               << " total_boundary_edges=" << seam_stats.total_edges
                                               << " geometrically_paired_edges=" << seam_stats.geometrically_paired_edges
                                               << " paired_boundary_ratio=" << paired_boundary_ratio
                                               << " duplicate_edge_keys=" << seam_stats.duplicate_edge_keys
                                               << " max_edge_multiplicity=" << seam_stats.max_edge_multiplicity
                                               << " open_edge_limit=" << MAX_PRE_WINDOWS_SEAM_NORMALIZATION_OPEN_EDGES
                                               << " min_faces_per_open_edge=" << MIN_FACES_PER_SEAM_NORMALIZATION_OPEN_EDGE
                                               << " min_paired_ratio=" << MIN_PAIRED_SEAM_BOUNDARY_RATIO;
                    if (internal_fast_path_eligible) {
                        report(5, "Repairing mesh");
                        RepairSetting pre_windows_repair_setting;
                        pre_windows_repair_setting.max_hole_edges = MAX_PRE_WINDOWS_REPAIR_HOLE_EDGES;
                        std::shared_ptr<TriMesh> internal_repaired = run_internal_repair(
                            "pre_windows_fast_path_extended_holes", pre_windows_repair_setting);
                        if (cancelled()) return false;
                        if (internal_repaired && validate_internal_repair(
                                *internal_repaired, "pre_windows_fast_path_extended_holes")) {
                            if (!resample_face_colors(std::move(*internal_repaired)))
                                return false;
                            internal_fast_path_succeeded = true;
                            mesh_known_halfedge_compatible = true;
                        }
                    } else if (seam_normalization_eligible) {
                        report(5, "Welding duplicate mesh seams");
                        const auto weld_started_at = Clock::now();
                        TriMesh directly_welded_mesh;
                        ExactVertexWeldStats weld_stats;
                        const bool weld_succeeded = weld_exact_duplicate_vertices(
                            mesh, directly_welded_mesh, weld_stats);
                        BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                                   << " stage=exact_seam_vertex_weld elapsed_ms="
                                                   << std::chrono::duration<double, std::milli>(Clock::now() - weld_started_at).count()
                                                   << " success=" << weld_succeeded
                                                   << " input_vertices=" << mesh.vertices.size()
                                                   << " output_vertices=" << directly_welded_mesh.vertices.size()
                                                   << " faces=" << mesh.indices.size()
                                                   << " merged_vertices=" << weld_stats.merged_vertices
                                                   << " degenerate_faces=" << weld_stats.degenerate_faces
                                                   << " invalid_face_indices=" << weld_stats.invalid_face_indices;
                        if (cancelled()) return false;
                        if (weld_succeeded && validate_internal_repair(
                                directly_welded_mesh, "pre_windows_exact_seam_vertex_weld")) {
                            mesh = std::move(directly_welded_mesh);
                            BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                                       << " stage=repair_resample_total skipped=true"
                                                       << " reason=face_order_and_count_preserved"
                                                       << " faces=" << mesh.indices.size();
                            internal_fast_path_succeeded = true;
                            mesh_known_halfedge_compatible = true;
                        }

                        if (!internal_fast_path_succeeded) {
                            report(5, "Normalizing mesh seams");
                            RepairSetting normalization_only_setting;
                            normalization_only_setting.max_hole_edges = 0;
                            normalization_only_setting.max_boundary_edges = 0;
                            std::shared_ptr<TriMesh> internal_repaired = run_internal_repair(
                                "pre_windows_seam_normalization_only", normalization_only_setting);
                            if (cancelled()) return false;
                            if (internal_repaired && validate_internal_repair(
                                    *internal_repaired, "pre_windows_seam_normalization_only")) {
                                if (!resample_face_colors(std::move(*internal_repaired)))
                                    return false;
                                internal_fast_path_succeeded = true;
                                mesh_known_halfedge_compatible = true;
                            }
                        }
                    } else {
                        BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                                   << " stage=internal_repair_fast_path skipped=true reason=boundary_preflight"
                                                   << " open_edges=" << open_edges
                                                   << " total_limit=" << MAX_REPAIRABLE_MESH_BOUNDARY_EDGES
                                                   << " component_limit=" << MAX_PRE_WINDOWS_REPAIR_HOLE_EDGES
                                                   << " seam_normalization_eligible=" << seam_normalization_eligible;
                    }

                    if (!internal_fast_path_succeeded) {
                        indexed_triangle_set repaired_its;
                        std::string repair_error;
                        const size_t repair_input_vertices = mesh.vertices.size();
                        const size_t repair_input_faces    = mesh.indices.size();
                        const auto windows_repair_started_at = Clock::now();
                        bool repaired = settings.mesh_repair_callback && settings.mesh_repair_callback(
                            static_cast<const indexed_triangle_set&>(mesh), repaired_its,
                            [&](const char* message, unsigned /*percent*/) {
                                report(5, message ? message : "Repairing mesh");
                            },
                            [&]() { return cancelled(); }, &repair_error);
                        BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                                   << " stage=windows_mesh_repair elapsed_ms="
                                                   << std::chrono::duration<double, std::milli>(Clock::now() - windows_repair_started_at).count()
                                                   << " callback_available=" << static_cast<bool>(settings.mesh_repair_callback)
                                                   << " success=" << repaired
                                                   << " input_vertices=" << repair_input_vertices
                                                   << " input_faces=" << repair_input_faces
                                                   << " output_vertices=" << repaired_its.vertices.size()
                                                   << " output_faces=" << repaired_its.indices.size()
                                                   << " error=" << (repair_error.empty() ? "none" : repair_error);
                        if (repaired) {
                            if (cancelled()) return false;
                            BOOST_LOG_TRIVIAL(info) << log_prefix << ": Windows 3D mesh repair finished.";
                            if (!resample_face_colors(TriMesh(std::move(repaired_its))))
                                return false;
                        } else {
                            BOOST_LOG_TRIVIAL(warning) << log_prefix << ": Windows 3D mesh repair failed: " << repair_error;
                        }
                    }
                }
            }
        } else {
            BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                       << " stage=mesh_open_edge_check skipped=true faces=" << mesh.facets_count();
        }

        if (mesh_known_halfedge_compatible) {
            BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                       << " stage=halfedge_compatibility_check skipped=true reason=internal_fast_path_validated"
                                       << " faces=" << mesh.facets_count();
        } else {
            const auto halfedge_check_started_at = Clock::now();
            const bool halfedge_compatible = cgalutils::is_mesh_halfedge_compatible(mesh);
            BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix << " stage=halfedge_compatibility_check elapsed_ms="
                                       << std::chrono::duration<double, std::milli>(Clock::now() - halfedge_check_started_at).count()
                                       << " faces=" << mesh.facets_count() << " compatible=" << halfedge_compatible;
            if (!halfedge_compatible) {
                BOOST_LOG_TRIVIAL(info) << log_prefix << ": mesh not halfedge-compatible, attempting RepairMesh.";
                if (!repair_and_resample())
                    return false;
            }
        }
    } else {
        BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix
                                   << " stage=mesh_validation_and_repair cache_hit=true faces=" << mesh.facets_count();
    }
    log_stage("mesh_validation_and_repair");

    if (prepared_output) {
        prepared_output->mesh        = mesh;
        prepared_output->face_colors = face_colors;
    }

#ifdef OUTPUT_TEST_RESULT
    SaveToOFF(std::string(log_prefix) + "_1_repair.off", mesh, face_colors);
#endif

    report(20, "Color clustering");
    if (cancelled()) return false;

    // Clustering
    std::vector<RGB> cluster_centers;
    out_clustered_face_colors = face_colors;
    std::vector<std::size_t> clustered_face_labels(face_colors.size());
    const bool adaptive_cluster = settings.target_colors_num == 0;

    if (adaptive_cluster) {
        BOOST_LOG_TRIVIAL(debug) << log_prefix << ": use cluster adaptive method.";
        ClusterParameters para;
        para.max_color_distance = settings.max_color_distance;
        para.max_cluster_k = settings.max_cluster_k;
        para.cancel_callback = cancel_callback ? [&]() { return cancel_callback(); } : std::function<bool()>{};
        cluster_centers = cluster_adaptive(face_colors, para);
        if (cancelled()) return false;
    } else {
        BOOST_LOG_TRIVIAL(debug) << log_prefix << ": use cluster k-means method.";
        ClusterParameters para;
        para.cluster_k = settings.target_colors_num;
        para.cancel_callback = cancel_callback ? [&]() { return cancel_callback(); } : std::function<bool()>{};
        cluster_centers = cluster_k_means(face_colors, para);
        if (cancelled()) return false;
    }

    BOOST_LOG_TRIVIAL(debug) << log_prefix << ": k = " << cluster_centers.size() << ".";
    if (cluster_centers.empty()) {
        BOOST_LOG_TRIVIAL(debug) << log_prefix << ": no cluster center generated.";
        return false;
    }
    log_stage("color_clustering");

    report(40, "Assigning cluster labels");
    if (cancelled()) return false;

    // Assign each face to nearest cluster center
    {
        std::atomic<size_t> done{0};
        std::atomic<bool> cancel_requested{false};
        const size_t total = mesh.indices.size();
        const size_t interval = std::max<size_t>(total / 20, 1);
        tbb::parallel_for(tbb::blocked_range<std::size_t>(0, total), [&](const tbb::blocked_range<size_t>& range) {
            for (std::size_t fid = range.begin(); fid < range.end(); ++fid) {
                if (cancel_requested.load(std::memory_order_relaxed)) return;
                std::size_t nearest_id = 0;
                calc_nearest_color_id(cluster_centers, face_colors[fid], nearest_id);
                clustered_face_labels[fid] = nearest_id;
                out_clustered_face_colors[fid] = cluster_centers[nearest_id];
                size_t cnt = done.fetch_add(1, std::memory_order_relaxed) + 1;
                if (cnt % interval == 0) {
                    if (cancelled()) { cancel_requested.store(true, std::memory_order_relaxed); return; }
                }
            }
        });
        if (cancel_requested.load() || cancelled()) return false;
    }
    if (adaptive_cluster) {
        if (!discard_unused_cluster_centers(cluster_centers, clustered_face_labels, "cluster assignment"))
            return false;
    } else {
        ensure_all_cluster_centers_used(face_colors, cluster_centers, clustered_face_labels, "cluster assignment");
    }
    log_stage("cluster_assignment");

#ifdef OUTPUT_TEST_RESULT
    {
        std::vector<RGB> tmp = out_clustered_face_colors;
        for (std::size_t i = 0; i < tmp.size(); ++i)
            tmp[i] = cluster_centers[clustered_face_labels[i]];
        SaveToOFF(std::string(log_prefix) + "_3_cluster.off", mesh, tmp);
    }
#endif

    report(65, "Smoothing colors");
    if (cancelled()) return false;

    SmoothParameters smooth_parameters;
    smooth_parameters.smooth_weight = settings.smooth_weight;
    if (!smooth_region(mesh, clustered_face_labels, smooth_parameters)) {
        BOOST_LOG_TRIVIAL(debug) << log_prefix << ": smooth region failed.";
        return false;
    }
    if (adaptive_cluster) {
        if (!discard_unused_cluster_centers(cluster_centers, clustered_face_labels, "color smoothing"))
            return false;
    } else {
        ensure_all_cluster_centers_used(face_colors, cluster_centers, clustered_face_labels, "color smoothing");
    }
    log_stage("region_smoothing");

    report(90, "Updating face colors");
    if (cancelled()) return false;

    for (std::size_t i = 0; i < out_clustered_face_colors.size(); ++i)
        out_clustered_face_colors[i] = cluster_centers[clustered_face_labels[i]];
    log_stage("write_face_colors");

    BOOST_LOG_TRIVIAL(warning) << "[TEXTURE_IMPORT_TIMING] " << log_prefix << " stage=repair_cluster_smooth_total elapsed_ms="
                            << std::chrono::duration<double, std::milli>(Clock::now() - total_started_at).count()
                            << " faces=" << mesh.facets_count();

#ifdef OUTPUT_TEST_RESULT
    SaveToOFF(std::string(log_prefix) + "_4_smooth.off", mesh, out_clustered_face_colors);
#endif

    report(100, "Completed");
    return true;
}

bool TextureToColor(const TriMesh& texture_mesh, const std::vector<std::vector<Vec2f>>& texture_mesh_uv_coords, const cv::Mat& texture, TriMesh& color_mesh,
                    std::vector<std::array<std::size_t, 3>>& face_colors, const TextureToColorSettings& settings, AlgoProgressCallback progress_callback,
                    AlgoCancelCallback cancel_callback, PreparedTextureColorData* prepared_output) {
    auto report = [&](int pct, const char* msg) {
        if (progress_callback) {
            progress_callback({pct, msg});
        }
    };
    auto sub_report = [&](int sub_pct, int range_start, int range_end, const char* msg) {
        int pct = range_start + sub_pct * (range_end - range_start) / 100;
        report(pct, msg);
    };
    auto cancelled = [&]() -> bool {
        if (cancel_callback && cancel_callback()) {
            BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << " cancelled";
            return true;
        }
        return false;
    };

    color_mesh.clear();
    face_colors.clear();

    report(0, "Initializing");
    if (cancelled()) {
        return false;
    }

    if (texture_mesh.indices.size() == 0) {
        BOOST_LOG_TRIVIAL(debug) << "TextureToColor: texture mesh has no faces.";
        return false;
    }
    if (texture.empty()) {
        BOOST_LOG_TRIVIAL(debug) << "TextureToColor: texture is empty.";
        return false;
    }
    if (texture.channels() < 3) {
        BOOST_LOG_TRIVIAL(debug) << "TextureToColor: texture must have at least 3 channels, got " << texture.channels();
        return false;
    }
    if (texture_mesh_uv_coords.size() != texture_mesh.indices.size()) {
        BOOST_LOG_TRIVIAL(debug) << "TextureToColor: uv_coords size is not equal to texture mesh faces size.";
        return false;
    }
    for (std::size_t fid = 0; fid < texture_mesh.indices.size(); ++fid) {
        if (texture_mesh_uv_coords[fid].size() != 3) {
            BOOST_LOG_TRIVIAL(debug) << "TextureToColor: uv_coords of single face size is not equal to 3.";
            return false;
        }
    }
    color_mesh = texture_mesh;

    using Clock = std::chrono::high_resolution_clock;
    const auto t_total_start = Clock::now();
    auto t_step = t_total_start;
    auto lap = [&](const char* step_name) {
        auto now = Clock::now();
        double ms = std::chrono::duration<double, std::milli>(now - t_step).count();
        BOOST_LOG_TRIVIAL(debug) << "[timing] " << step_name << ": " << ms << "ms"
                        << " faces=" << color_mesh.facets_count();
        t_step = now;
    };

    report(5, "Oversampling");
    if (cancelled()) {
        return false;
    }

    // Step 1: Oversampling (subdivision while propagating UVs)
    // Convert external vector<vector<Vec2f>> to internal vector<array<Vec2f,3>> to eliminate inner-level heap allocations
    std::vector<FaceUVArray> color_mesh_uv_coords(texture_mesh_uv_coords.size());
    for (std::size_t i = 0; i < texture_mesh_uv_coords.size(); ++i) {
        color_mesh_uv_coords[i] = {texture_mesh_uv_coords[i][0], texture_mesh_uv_coords[i][1], texture_mesh_uv_coords[i][2]};
    }
    {
        // Estimate total iterations and map each iteration's sub-progress to the [5, 25] range
        size_t estimated_iters = 0;
        if (settings.oversampling_iters > 0) {
            estimated_iters = settings.oversampling_iters;
        } else {
            size_t fc = color_mesh.facets_count();
            while (fc < settings.oversampling_min_face_count) {
                fc *= 4;
                ++estimated_iters;
            }
            if (estimated_iters == 0) {
                estimated_iters = 1;
            }
        }

        auto make_iter_progress = [&](size_t iter) {
            return [&, iter, estimated_iters](int pct) {
                int iter_start = static_cast<int>(iter * 100 / estimated_iters);
                int iter_end = static_cast<int>((iter + 1) * 100 / estimated_iters);
                int sub_pct = iter_start + pct * (iter_end - iter_start) / 100;
                sub_report(sub_pct, 5, 25, "Oversampling");
            };
        };

        if (settings.oversampling_iters > 0) {
            for (size_t i = 0; i < settings.oversampling_iters && color_mesh.facets_count() * 4.0 < settings.oversampling_max_face_count; ++i) {
                if (cancelled()) return false;
                linear_subdivision(color_mesh, color_mesh_uv_coords, make_iter_progress(i));
            }
        } else {
            size_t iter = 0;
            while (color_mesh.facets_count() < settings.oversampling_min_face_count) {
                if (cancelled()) return false;
                linear_subdivision(color_mesh, color_mesh_uv_coords, make_iter_progress(iter++));
            }
        }
    }

    lap("Oversampling");

    face_colors.resize(color_mesh.indices.size());

    report(25, "Computing face colors");
    if (cancelled()) {
        return false;
    }

    // Step 2: Compute each face's color (7-point Gaussian quadrature + bilinear interpolation sampling)
    {
        std::atomic<size_t> done_faces{0};
        std::atomic<bool> cancel_requested{false};
        const size_t total_faces = color_mesh.indices.size();
        const size_t report_interval = std::max<size_t>(total_faces / 20, 1);
        tbb::parallel_for(tbb::blocked_range<std::size_t>(0, total_faces), [&](const tbb::blocked_range<size_t>& range) {
            for (std::size_t fid = range.begin(); fid < range.end(); ++fid) {
                if (cancel_requested.load(std::memory_order_relaxed)) return;
                face_colors[fid] = sample_face_color(color_mesh_uv_coords[fid], texture);
                size_t cnt = done_faces.fetch_add(1, std::memory_order_relaxed) + 1;
                if (cnt % report_interval == 0) {
                    if (cancelled()) { cancel_requested.store(true, std::memory_order_relaxed); return; }
                    sub_report(static_cast<int>(cnt * 100 / total_faces), 25, 40, "Computing face colors");
                }
            }
        });
        if (cancel_requested.load() || cancelled()) return false;
    }
    lap("Computing face colors");
#ifdef OUTPUT_TEST_RESULT
    SaveToOFF("texture_to_color_0_initialize.off", color_mesh, face_colors);
#endif

    // Map progress from repair_cluster_smooth's [0,100] to TextureToColor's [40,100]
    AlgoProgressCallback rcs_progress = nullptr;
    if (progress_callback) {
        rcs_progress = [&](AlgoProgress p) {
            int mapped_pct = 40 + p.percent * 60 / 100;
            progress_callback({mapped_pct, p.message});
        };
    }

    std::vector<RGB> clustered_face_colors;
    if (!repair_cluster_smooth(color_mesh, face_colors, clustered_face_colors,
                               settings, rcs_progress, cancel_callback, "TextureToColor", prepared_output))
        return false;

    face_colors = std::move(clustered_face_colors);
    lap("Repair + Clustering + Smoothing");
    double total_ms = std::chrono::duration<double, std::milli>(Clock::now() - t_total_start).count();
    BOOST_LOG_TRIVIAL(debug) << "[timing] TextureToColor total: " << total_ms << "ms"
                    << " faces=" << color_mesh.facets_count();
    report(100, "Completed");
    return true;
}

bool ClusterAndSmooth(const TriMesh& mesh,
                      const std::vector<std::array<std::size_t, 3>>& input_face_colors,
                      TriMesh& out_mesh,
                      std::vector<std::array<std::size_t, 3>>& out_face_colors,
                      const TextureToColorSettings& settings,
                      AlgoProgressCallback progress_callback,
                      AlgoCancelCallback cancel_callback,
                      const std::vector<std::array<float, 4>>& vertex_colors,
                      PreparedTextureColorData* prepared_output)
{
    auto report = [&](int pct, const char* msg) {
        if (progress_callback)
            progress_callback({pct, msg});
    };
    auto cancelled = [&]() -> bool {
        if (cancel_callback && cancel_callback()) {
            BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << " cancelled";
            return true;
        }
        return false;
    };

    out_mesh = mesh;
    out_face_colors.clear();

    if (mesh.indices.empty() || input_face_colors.empty()) {
        BOOST_LOG_TRIVIAL(debug) << "ClusterAndSmooth: empty mesh or face colors.";
        return false;
    }
    if (input_face_colors.size() != mesh.indices.size()) {
        BOOST_LOG_TRIVIAL(warning) << "ClusterAndSmooth: face_colors size ("
                                   << input_face_colors.size() << ") != indices size ("
                                   << mesh.indices.size() << "), clamping.";
    }

    report(0, "Initializing");
    if (cancelled()) return false;

    // Prepare face colors aligned to mesh size
    std::vector<RGB> face_colors(out_mesh.indices.size());
    for (size_t i = 0; i < out_mesh.indices.size(); ++i) {
        if (i < input_face_colors.size())
            face_colors[i] = input_face_colors[i];
        else
            face_colors[i] = {128, 128, 128};
    }

    // Low-poly vertex-color meshes take the legacy OBJ import route: quantize the
    // vertex colors, then split only across cluster boundaries. Colors are exact
    // cluster centers afterwards, so repair / re-clustering / smoothing are skipped
    // to match the legacy behaviour, which never touched the mesh either.
    // A vertex color count that disagrees with the mesh falls through to the generic
    // pipeline below rather than failing the import outright.
    if (!vertex_colors.empty() &&
        vertex_colors.size() == out_mesh.vertices.size() &&
        out_mesh.facets_count() < settings.oversampling_min_face_count) {
        report(10, "Quantizing vertex colors");
        std::vector<RGB>         cluster_centers;
        std::vector<std::size_t> vertex_cluster_ids;
        if (!quantize_vertex_colors(vertex_colors, settings, cancel_callback, cluster_centers, vertex_cluster_ids)) {
            BOOST_LOG_TRIVIAL(debug) << "ClusterAndSmooth: vertex color quantization failed.";
            return false;
        }
        if (cancelled()) return false;

        report(50, "Splitting color boundaries");
        if (!adaptive_split_by_vertex_clusters(out_mesh, vertex_cluster_ids, cluster_centers, face_colors)) {
            BOOST_LOG_TRIVIAL(debug) << "ClusterAndSmooth: adaptive vertex-color split failed.";
            return false;
        }
        if (cancelled()) return false;

        out_face_colors = std::move(face_colors);
        report(100, "Completed");
        return true;
    }

    std::vector<RGB> clustered_face_colors;
    if (!repair_cluster_smooth(out_mesh, face_colors, clustered_face_colors,
                               settings, progress_callback, cancel_callback,
                               "ClusterAndSmooth", prepared_output))
        return false;

    out_face_colors = std::move(clustered_face_colors);
    return true;
}

}  // namespace tex2color
}  // namespace Slic3r
