#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "GpuOrientMetal.hpp"
#include "GpuOrientMetalKernels.hpp"

#include "libslic3r/Geometry.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <boost/log/trivial.hpp>

namespace Slic3r {
namespace orientation {
namespace metal {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kCpuEps = 1e-6f;
constexpr uint32_t kThreadsPerGroup = 256u;
constexpr uint32_t kItemsPerThread = 4u;
constexpr uint32_t kTargetTotalWorkgroups = 32768u;

struct VertexGpu {
    float x, y, z, pad;
};

struct FaceGpu {
    float nx, ny, nz;
    float area_plain;
    float area_overhang;
    uint32_t i0, i1, i2;
    float pad0, pad1;
};

struct HullFaceGpu {
    float area_plain;
    uint32_t i0, i1, i2;
    uint32_t pad0;
};

struct OrientationGpu {
    float x, y, z, pad;
};

struct UInt4 {
    uint32_t x, y, z, w;
};

struct RangeParams {
    uint32_t item_count;
    uint32_t partial_count;
    uint32_t group_base;
    uint32_t pad0;
};

struct CostParams {
    uint32_t partial_count;
    uint32_t face_count;
    uint32_t hull_face_count;
    uint32_t orient_mode;
    float ascent;
    float first_lay_h;
    float area_scale;
    float laf_min_cos;
    float laf_max_cos;
    float eps_z;
    uint32_t group_base;
    uint32_t pad0;
};

static_assert(sizeof(VertexGpu) == 16, "Metal VertexGpu layout mismatch");
static_assert(sizeof(FaceGpu) == 40, "Metal FaceGpu layout mismatch");
static_assert(sizeof(HullFaceGpu) == 20, "Metal HullFaceGpu layout mismatch");
static_assert(sizeof(OrientationGpu) == 16, "Metal OrientationGpu layout mismatch");
static_assert(sizeof(UInt4) == 16, "Metal UInt4 layout mismatch");
static_assert(sizeof(RangeParams) == 16, "Metal RangeParams layout mismatch");
static_assert(sizeof(CostParams) == 48, "Metal CostParams layout mismatch");

struct PreparedMesh {
    std::vector<Vec3f> candidates;
    std::vector<VertexGpu> vertices;
    std::vector<FaceGpu> faces;
    std::vector<HullFaceGpu> hull_faces;
    double total_plain_area = 0.0;
    double total_overhang_area = 0.0;
};

struct Evaluation {
    std::vector<int32_t> minz_ordered;
    std::vector<int32_t> maxz_ordered;
    std::vector<UInt4> result0;
    std::vector<UInt4> result1;
    std::vector<float> support_volume;
    std::vector<UInt4> result2;
};

static std::string ns_error_string(NSError* error)
{
    if (error == nil)
        return {};
    NSString* description = error.localizedDescription;
    return description != nil ? std::string(description.UTF8String) : std::string("Unknown Metal error");
}

struct MetalContext {
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> queue = nil;
    id<MTLLibrary> library = nil;
    id<MTLComputePipelineState> minz_stage1 = nil;
    id<MTLComputePipelineState> minz_stage2 = nil;
    id<MTLComputePipelineState> maxz_stage1 = nil;
    id<MTLComputePipelineState> maxz_stage2 = nil;
    id<MTLComputePipelineState> cost_stage1 = nil;
    id<MTLComputePipelineState> cost_stage2 = nil;
    bool ok = false;
    std::string error;

    MetalContext()
    {
        @autoreleasepool {
            device = MTLCreateSystemDefaultDevice();
            if (device == nil) {
                error = "Metal is unavailable: MTLCreateSystemDefaultDevice returned nil";
                return;
            }

            queue = [device newCommandQueue];
            if (queue == nil) {
                error = "Metal is unavailable: failed to create command queue";
                return;
            }

            NSString* source =
                [NSString stringWithUTF8String:kMetalKernelSource];
            NSError* compile_error = nil;
            library = [device newLibraryWithSource:source
                                           options:nil
                                             error:&compile_error];
            if (library == nil) {
                error = "Metal shader compilation failed: " +
                        ns_error_string(compile_error);
                return;
            }

            if (!build_pipeline(@"minz_stage1", minz_stage1) ||
                !build_pipeline(@"minz_stage2", minz_stage2) ||
                !build_pipeline(@"maxz_stage1", maxz_stage1) ||
                !build_pipeline(@"maxz_stage2", maxz_stage2) ||
                !build_pipeline(@"cost_stage1", cost_stage1) ||
                !build_pipeline(@"cost_stage2", cost_stage2))
                return;

            ok = true;
        }
    }

    bool build_pipeline(NSString* name,
                        id<MTLComputePipelineState> __strong& output)
    {
        id<MTLFunction> function = [library newFunctionWithName:name];
        if (function == nil) {
            error = "Metal shader function not found: " +
                    std::string(name.UTF8String);
            return false;
        }

        NSError* pipeline_error = nil;
        output = [device newComputePipelineStateWithFunction:function
                                                       error:&pipeline_error];
        if (output == nil) {
            error = "Metal pipeline creation failed for " +
                    std::string(name.UTF8String) + ": " +
                    ns_error_string(pipeline_error);
            return false;
        }

        if (output.maxTotalThreadsPerThreadgroup < kThreadsPerGroup) {
            std::ostringstream stream;
            stream << "Metal pipeline " << name.UTF8String
                   << " supports only "
                   << output.maxTotalThreadsPerThreadgroup
                   << " threads per group; 256 required";
            error = stream.str();
            output = nil;
            return false;
        }
        return true;
    }
};

static MetalContext& metal_context()
{
    static MetalContext context;
    return context;
}

static id<MTLBuffer> make_buffer(id<MTLDevice> device,
                                 const void* data,
                                 size_t length,
                                 NSString* label)
{
    const NSUInteger allocation_length =
        static_cast<NSUInteger>(std::max<size_t>(length, 16u));
    id<MTLBuffer> buffer =
        [device newBufferWithLength:allocation_length
                            options:MTLResourceStorageModeShared];
    if (buffer != nil) {
        buffer.label = label;
        if (data != nullptr && length != 0u)
            std::memcpy(buffer.contents, data, length);
        else
            std::memset(buffer.contents, 0, allocation_length);
    }
    return buffer;
}

static bool command_buffer_succeeded(
    id<MTLCommandBuffer> command_buffer,
    const std::function<bool(void)>& stop_condition,
    const char* label,
    std::string* error)
{
    [command_buffer commit];
    bool canceled = false;
    while (command_buffer.status != MTLCommandBufferStatusCompleted &&
           command_buffer.status != MTLCommandBufferStatusError) {
        if (stop_condition && stop_condition())
            canceled = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (canceled) {
        if (error)
            *error = "Canceled";
        return false;
    }

    if (command_buffer.status == MTLCommandBufferStatusError) {
        if (error) {
            *error = std::string("Metal command buffer failed (") + label +
                     "): " + ns_error_string(command_buffer.error);
        }
        return false;
    }
    return true;
}

static uint32_t pick_chunk_groups(uint32_t total_groups,
                                  uint32_t orientation_count)
{
    if (total_groups == 0u)
        return 0u;
    if (orientation_count == 0u)
        return total_groups;
    uint32_t chunk = kTargetTotalWorkgroups / orientation_count;
    chunk = std::max(1u, chunk);
    return std::min(chunk, total_groups);
}

static inline float ordered_int_to_float(int32_t ordered)
{
    const int32_t bits =
        ordered < 0
            ? static_cast<int32_t>(0x80000000u -
                                   static_cast<uint32_t>(ordered))
            : ordered;
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

static inline float compute_ascent(float overhang_angle_degrees)
{
    return std::cos(
        kPi - (overhang_angle_degrees + 1.0f) * kPi / 180.0f);
}

static void remove_duplicates(std::vector<Vec3f>& orientations,
                              double tolerance = 1e-7)
{
    if (orientations.size() <= 1)
        return;

    const Vec3f zero{0.0f, 0.0f, 0.0f};
    for (auto current = orientations.begin() + 1;
         current != orientations.end();) {
        bool duplicate = false;
        for (auto accepted = orientations.begin();
             accepted != current;
             ++accepted) {
            if (accepted->isApprox(*current, tolerance)) {
                duplicate = true;
                break;
            }
        }
        if (duplicate || current->isApprox(zero, tolerance))
            current = orientations.erase(current);
        else
            ++current;
    }
}

static void add_supplements(std::vector<Vec3f>& orientations)
{
    static const Vec3f extra[] = {
        {0, 0, -1},
        {0.70710678f, 0, -0.70710678f},
        {0, 0.70710678f, -0.70710678f},
        {-0.70710678f, 0, -0.70710678f},
        {0, -0.70710678f, -0.70710678f},
        {1, 0, 0},
        {0.70710678f, 0.70710678f, 0},
        {0, 1, 0},
        {-0.70710678f, 0.70710678f, 0},
        {-1, 0, 0},
        {-0.70710678f, -0.70710678f, 0},
        {0, -1, 0},
        {0.70710678f, -0.70710678f, 0},
        {0.70710678f, 0, 0.70710678f},
        {0, 0.70710678f, 0.70710678f},
        {-0.70710678f, 0, 0.70710678f},
        {0, -0.70710678f, 0.70710678f},
        {0, 0, 1},
    };
    orientations.insert(
        orientations.end(), std::begin(extra), std::end(extra));
}

static bool is_face_appearance(const indexed_triangle_set& mesh,
                               int face_index)
{
    return const_cast<indexed_triangle_set&>(mesh)
               .get_property(face_index)
               .type == EnumFaceTypes::eExteriorAppearance;
}

struct BinAccum {
    float sum_area = 0.0f;
    float max_area = 0.0f;
    float nx = 0.0f;
    float ny = 0.0f;
    float nz = 0.0f;
};

static uint32_t octahedral_bin(const Vec3f& normal)
{
    constexpr int resolution = 256;
    const float x = normal.x();
    const float y = normal.y();
    const float z = normal.z();
    const float denominator =
        std::abs(x) + std::abs(y) + std::abs(z);
    if (denominator <= 0.0f)
        return 0u;

    float oct_x = x / denominator;
    float oct_y = y / denominator;
    if (z < 0.0f) {
        const float sign_x = oct_x >= 0.0f ? 1.0f : -1.0f;
        const float sign_y = oct_y >= 0.0f ? 1.0f : -1.0f;
        const float reflected_x =
            (1.0f - std::abs(oct_y)) * sign_x;
        const float reflected_y =
            (1.0f - std::abs(oct_x)) * sign_y;
        oct_x = reflected_x;
        oct_y = reflected_y;
    }

    int u = int((oct_x * 0.5f + 0.5f) *
                    float(resolution - 1) +
                0.5f);
    int v = int((oct_y * 0.5f + 0.5f) *
                    float(resolution - 1) +
                0.5f);
    u = std::max(0, std::min(resolution - 1, u));
    v = std::max(0, std::min(resolution - 1, v));
    return uint32_t(v * resolution + u);
}

static void append_top_bins(const std::vector<BinAccum>& bins,
                            int requested_count,
                            std::vector<Vec3f>& candidates)
{
    std::vector<std::pair<float, uint32_t>> ranked;
    ranked.reserve(bins.size());
    for (uint32_t index = 0; index < bins.size(); ++index) {
        if (bins[index].sum_area > 0.0f &&
            bins[index].max_area > 0.0f)
            ranked.emplace_back(bins[index].sum_area, index);
    }

    const int count =
        std::min<int>(requested_count, int(ranked.size()));
    if (count == 0)
        return;

    const auto greater_area = [](const auto& lhs, const auto& rhs) {
        return lhs.first > rhs.first;
    };
    if (ranked.size() > size_t(count)) {
        std::nth_element(
            ranked.begin(),
            ranked.begin() + count,
            ranked.end(),
            greater_area);
    }
    std::sort(
        ranked.begin(), ranked.begin() + count, greater_area);
    for (int index = 0; index < count; ++index) {
        const BinAccum& bin = bins[ranked[size_t(index)].second];
        candidates.emplace_back(bin.nx, bin.ny, bin.nz);
    }
}

static bool prepare_mesh(const TriangleMesh& triangle_mesh,
                         const OrientParams& params,
                         PreparedMesh& output,
                         std::string* error)
{
    if (params.stopcondition && params.stopcondition()) {
        if (error)
            *error = "Canceled";
        return false;
    }

    const int face_count = triangle_mesh.facets_count();
    const int vertex_count =
        int(triangle_mesh.its.vertices.size());
    if (face_count <= 0 || vertex_count <= 0) {
        if (error)
            *error = "Empty mesh";
        return false;
    }

    output = PreparedMesh{};
    output.vertices.reserve(size_t(vertex_count));
    for (int index = 0; index < vertex_count; ++index) {
        if (params.stopcondition &&
            (index & ((1 << 20) - 1)) == 0 &&
            params.stopcondition()) {
            if (error)
                *error = "Canceled";
            return false;
        }
        const Vec3f& vertex =
            triangle_mesh.its.vertices[size_t(index)];
        output.vertices.push_back(
            {vertex.x(), vertex.y(), vertex.z(), 0.0f});
    }

    constexpr int max_candidate_sample_faces = 2'000'000;
    const int sample_stride =
        face_count > max_candidate_sample_faces
            ? std::max(
                  1,
                  (face_count + max_candidate_sample_faces - 1) /
                      max_candidate_sample_faces)
            : 1;
    constexpr uint32_t bin_count = 256u * 256u;

    const bool need_appearance =
        params.APPERANCE_FACE_SUPP != 0.0f;
    std::vector<uint8_t> appearance(size_t(face_count), 0u);
    if (need_appearance) {
        for (int face_index = 0;
             face_index < face_count;
             ++face_index) {
            if (params.stopcondition &&
                (face_index & ((1 << 18) - 1)) == 0 &&
                params.stopcondition()) {
                if (error)
                    *error = "Canceled";
                return false;
            }
            appearance[size_t(face_index)] =
                is_face_appearance(
                    triangle_mesh.its, face_index)
                    ? 1u
                    : 0u;
        }
    }

    output.faces.resize(size_t(face_count));
    const auto& vertices = triangle_mesh.its.vertices;
    const auto& indices = triangle_mesh.its.indices;

    unsigned hardware_threads =
        std::thread::hardware_concurrency();
    if (hardware_threads == 0)
        hardware_threads = 4;
    const unsigned thread_count =
        params.parallel && face_count >= 500'000
            ? std::min(hardware_threads, 12u)
            : 1u;

    std::atomic<bool> canceled{false};
    std::vector<std::vector<BinAccum>> thread_bins(thread_count);
    for (auto& bins : thread_bins)
        bins.assign(bin_count, BinAccum{});
    std::vector<double> plain_areas(thread_count, 0.0);
    std::vector<double> overhang_areas(thread_count, 0.0);

    const auto worker =
        [&](unsigned thread_index, int begin, int end) {
            std::vector<BinAccum>& bins =
                thread_bins[thread_index];
            double total_plain = 0.0;
            double total_overhang = 0.0;
            for (int face_index = begin;
                 face_index < end;
                 ++face_index) {
                if ((face_index & ((1 << 17) - 1)) == 0) {
                    if (canceled.load(
                            std::memory_order_relaxed))
                        break;
                    if (params.stopcondition &&
                        params.stopcondition()) {
                        canceled.store(
                            true, std::memory_order_relaxed);
                        break;
                    }
                }

                const auto& triangle =
                    indices[size_t(face_index)];
                const Vec3f& point0 =
                    vertices[size_t(triangle[0])];
                const Vec3f& point1 =
                    vertices[size_t(triangle[1])];
                const Vec3f& point2 =
                    vertices[size_t(triangle[2])];
                const Vec3f cross =
                    (point1 - point0).cross(point2 - point0);
                const float length = cross.norm();
                const float plain_area = 0.5f * length;
                const Vec3f normal =
                    length > 0.0f
                        ? cross / length
                        : Vec3f(0.0f, 0.0f, 0.0f);
                const bool is_appearance =
                    need_appearance &&
                    appearance[size_t(face_index)] != 0u;
                const float appearance_weight =
                    1.0f +
                    (is_appearance
                         ? params.APPERANCE_FACE_SUPP
                         : 0.0f);
                const float overhang_area =
                    plain_area * appearance_weight;

                total_plain += plain_area;
                total_overhang += overhang_area;

                FaceGpu& face =
                    output.faces[size_t(face_index)];
                face.nx = normal.x();
                face.ny = normal.y();
                face.nz = normal.z();
                face.area_plain = plain_area;
                face.area_overhang = overhang_area;
                face.i0 = uint32_t(triangle[0]);
                face.i1 = uint32_t(triangle[1]);
                face.i2 = uint32_t(triangle[2]);
                face.pad0 = 0.0f;
                face.pad1 = 0.0f;

                const bool sample =
                    sample_stride == 1 ||
                    (uint32_t(face_index) * 2654435761u) %
                            uint32_t(sample_stride) ==
                        0u;
                if (sample && plain_area > 0.0f) {
                    BinAccum& bin =
                        bins[octahedral_bin(normal)];
                    bin.sum_area += plain_area;
                    if (plain_area > bin.max_area) {
                        bin.max_area = plain_area;
                        bin.nx = normal.x();
                        bin.ny = normal.y();
                        bin.nz = normal.z();
                    }
                }
            }
            plain_areas[thread_index] = total_plain;
            overhang_areas[thread_index] =
                total_overhang;
        };

    if (thread_count == 1u) {
        worker(0u, 0, face_count);
    } else {
        std::vector<std::thread> workers;
        workers.reserve(thread_count);
        const int chunk = face_count / int(thread_count);
        for (unsigned thread_index = 0;
             thread_index < thread_count;
             ++thread_index) {
            const int begin = int(thread_index) * chunk;
            const int end =
                thread_index + 1u == thread_count
                    ? face_count
                    : begin + chunk;
            workers.emplace_back(
                worker, thread_index, begin, end);
        }
        for (std::thread& thread : workers)
            thread.join();
    }

    if (canceled.load(std::memory_order_relaxed)) {
        if (error)
            *error = "Canceled";
        return false;
    }

    std::vector<BinAccum> bins(bin_count);
    for (unsigned thread_index = 0;
         thread_index < thread_count;
         ++thread_index) {
        output.total_plain_area +=
            plain_areas[thread_index];
        output.total_overhang_area +=
            overhang_areas[thread_index];
    }
    for (uint32_t bin_index = 0;
         bin_index < bin_count;
         ++bin_index) {
        BinAccum& destination = bins[bin_index];
        for (unsigned thread_index = 0;
             thread_index < thread_count;
             ++thread_index) {
            const BinAccum& source =
                thread_bins[thread_index][bin_index];
            destination.sum_area += source.sum_area;
            if (source.max_area > destination.max_area) {
                destination.max_area = source.max_area;
                destination.nx = source.nx;
                destination.ny = source.ny;
                destination.nz = source.nz;
            }
        }
    }

    output.candidates.reserve(64);
    output.candidates.emplace_back(0.0f, 0.0f, -1.0f);
    append_top_bins(bins, 10, output.candidates);

    if (params.stopcondition && params.stopcondition()) {
        if (error)
            *error = "Canceled";
        return false;
    }

    TriangleMesh hull = triangle_mesh.convex_hull_3d();
    const auto& hull_vertices = hull.its.vertices;
    const auto& hull_indices = hull.its.indices;
    const uint32_t hull_vertex_base =
        uint32_t(output.vertices.size());
    output.vertices.reserve(
        output.vertices.size() + hull_vertices.size());
    for (const Vec3f& vertex : hull_vertices) {
        output.vertices.push_back(
            {vertex.x(), vertex.y(), vertex.z(), 0.0f});
    }

    std::vector<BinAccum> hull_bins(bin_count);
    output.hull_faces.reserve(hull_indices.size());
    for (size_t face_index = 0;
         face_index < hull_indices.size();
         ++face_index) {
        if (params.stopcondition &&
            (face_index & ((1u << 16) - 1u)) == 0u &&
            params.stopcondition()) {
            if (error)
                *error = "Canceled";
            return false;
        }

        const auto& triangle = hull_indices[face_index];
        const Vec3f& point0 =
            hull_vertices[size_t(triangle[0])];
        const Vec3f& point1 =
            hull_vertices[size_t(triangle[1])];
        const Vec3f& point2 =
            hull_vertices[size_t(triangle[2])];
        const Vec3f cross =
            (point1 - point0).cross(point2 - point0);
        const float length = cross.norm();
        const float plain_area = 0.5f * length;
        const Vec3f normal =
            length > 0.0f
                ? cross / length
                : Vec3f(0.0f, 0.0f, 0.0f);

        if (plain_area > 0.0f) {
            BinAccum& bin =
                hull_bins[octahedral_bin(normal)];
            bin.sum_area += plain_area;
            if (plain_area > bin.max_area) {
                bin.max_area = plain_area;
                bin.nx = normal.x();
                bin.ny = normal.y();
                bin.nz = normal.z();
            }
        }

        output.hull_faces.push_back(
            {plain_area,
             hull_vertex_base + uint32_t(triangle[0]),
             hull_vertex_base + uint32_t(triangle[1]),
             hull_vertex_base + uint32_t(triangle[2]),
             0u});
    }
    append_top_bins(hull_bins, 14, output.candidates);

    if (params.orient_type == EOrientType::MinArea)
        add_supplements(output.candidates);
    else if (params.orient_type == EOrientType::MinVolume)
        output.candidates.emplace_back(0.0f, 0.0f, 1.0f);

    remove_duplicates(output.candidates);
    return true;
}

static bool evaluate_on_metal(
    const PreparedMesh& prepared,
    const std::vector<OrientationGpu>& orientations,
    const OrientMesh& mesh,
    const OrientParams& params,
    float area_scale,
    Evaluation& output,
    std::string* error)
{
    @autoreleasepool {
        MetalContext& context = metal_context();
        if (!context.ok) {
            if (error)
                *error = context.error;
            return false;
        }
        if (prepared.vertices.empty() ||
            prepared.faces.empty() ||
            orientations.empty()) {
            if (error)
                *error = "Empty Metal orientation buffers";
            return false;
        }

        const uint32_t orientation_count =
            uint32_t(orientations.size());
        const uint32_t item_block =
            kThreadsPerGroup * kItemsPerThread;
        const uint32_t minz_group_count =
            uint32_t((prepared.vertices.size() +
                      item_block - 1u) /
                     item_block);
        const uint32_t minz_chunk_count =
            pick_chunk_groups(
                minz_group_count, orientation_count);
        const size_t max_cost_items =
            std::max(prepared.faces.size(),
                     prepared.hull_faces.size());
        const uint32_t cost_group_count =
            uint32_t((max_cost_items +
                      item_block - 1u) /
                     item_block);
        const uint32_t cost_chunk_count =
            pick_chunk_groups(
                cost_group_count, orientation_count);

        output.minz_ordered.assign(
            orientation_count, int32_t(0x7f7fffff));
        output.maxz_ordered.assign(
            orientation_count,
            std::numeric_limits<int32_t>::min());
        output.result0.assign(
            orientation_count, UInt4{0u, 0u, 0u, 0u});
        output.result1.assign(
            orientation_count, UInt4{0u, 0u, 0u, 0u});
        output.support_volume.assign(
            orientation_count, 0.0f);
        output.result2.assign(
            orientation_count, UInt4{0u, 0u, 0u, 0u});

        id<MTLBuffer> vertex_buffer = make_buffer(
            context.device,
            prepared.vertices.data(),
            prepared.vertices.size() * sizeof(VertexGpu),
            @"GpuOrient vertices");
        id<MTLBuffer> face_buffer = make_buffer(
            context.device,
            prepared.faces.data(),
            prepared.faces.size() * sizeof(FaceGpu),
            @"GpuOrient faces");
        id<MTLBuffer> hull_face_buffer = make_buffer(
            context.device,
            prepared.hull_faces.data(),
            prepared.hull_faces.size() *
                sizeof(HullFaceGpu),
            @"GpuOrient hull faces");
        id<MTLBuffer> orientation_buffer = make_buffer(
            context.device,
            orientations.data(),
            orientations.size() * sizeof(OrientationGpu),
            @"GpuOrient orientations");
        id<MTLBuffer> minz_partial_buffer = make_buffer(
            context.device,
            nullptr,
            size_t(orientation_count) *
                minz_chunk_count * sizeof(float),
            @"GpuOrient partial minz");
        id<MTLBuffer> minz_buffer = make_buffer(
            context.device,
            output.minz_ordered.data(),
            output.minz_ordered.size() *
                sizeof(int32_t),
            @"GpuOrient minz");
        id<MTLBuffer> maxz_partial_buffer = make_buffer(
            context.device,
            nullptr,
            size_t(orientation_count) *
                minz_chunk_count * sizeof(float),
            @"GpuOrient partial maxz");
        id<MTLBuffer> maxz_buffer = make_buffer(
            context.device,
            output.maxz_ordered.data(),
            output.maxz_ordered.size() *
                sizeof(int32_t),
            @"GpuOrient maxz");

        const size_t cost_partial_count =
            size_t(orientation_count) * cost_chunk_count;
        id<MTLBuffer> partial0_buffer = make_buffer(
            context.device,
            nullptr,
            cost_partial_count * sizeof(UInt4),
            @"GpuOrient partial cost0");
        id<MTLBuffer> partial1_buffer = make_buffer(
            context.device,
            nullptr,
            cost_partial_count * sizeof(uint32_t),
            @"GpuOrient partial cost1");
        id<MTLBuffer> partial_support_buffer = make_buffer(
            context.device,
            nullptr,
            cost_partial_count * sizeof(float),
            @"GpuOrient partial support volume");
        id<MTLBuffer> partial2_buffer = make_buffer(
            context.device,
            nullptr,
            cost_partial_count * sizeof(UInt4),
            @"GpuOrient partial cost2");
        id<MTLBuffer> result0_buffer = make_buffer(
            context.device,
            output.result0.data(),
            output.result0.size() * sizeof(UInt4),
            @"GpuOrient result0");
        id<MTLBuffer> result1_buffer = make_buffer(
            context.device,
            output.result1.data(),
            output.result1.size() * sizeof(UInt4),
            @"GpuOrient result1");
        id<MTLBuffer> support_buffer = make_buffer(
            context.device,
            output.support_volume.data(),
            output.support_volume.size() * sizeof(float),
            @"GpuOrient support volume");
        id<MTLBuffer> result2_buffer = make_buffer(
            context.device,
            output.result2.data(),
            output.result2.size() * sizeof(UInt4),
            @"GpuOrient result2");

        id<MTLBuffer> buffers[] = {
            vertex_buffer,
            face_buffer,
            hull_face_buffer,
            orientation_buffer,
            minz_partial_buffer,
            minz_buffer,
            maxz_partial_buffer,
            maxz_buffer,
            partial0_buffer,
            partial1_buffer,
            partial_support_buffer,
            partial2_buffer,
            result0_buffer,
            result1_buffer,
            support_buffer,
            result2_buffer,
        };
        for (id<MTLBuffer> buffer : buffers) {
            if (buffer == nil) {
                if (error)
                    *error = "Failed to allocate Metal orientation buffer";
                return false;
            }
        }

        const MTLSize threads =
            MTLSizeMake(kThreadsPerGroup, 1u, 1u);

        for (uint32_t group_base = 0u;
             group_base < minz_group_count;
             group_base += minz_chunk_count) {
            if (params.stopcondition &&
                params.stopcondition()) {
                if (error)
                    *error = "Canceled";
                return false;
            }
            const uint32_t chunk = std::min(
                minz_chunk_count,
                minz_group_count - group_base);
            const RangeParams range{
                uint32_t(prepared.vertices.size()),
                chunk,
                group_base,
                0u};
            id<MTLCommandBuffer> command_buffer =
                [context.queue commandBuffer];
            if (command_buffer == nil) {
                if (error)
                    *error = "Failed to create Metal minz command buffer";
                return false;
            }
            command_buffer.label = @"GpuOrient minz";

            id<MTLComputeCommandEncoder> stage1 =
                [command_buffer computeCommandEncoder];
            if (stage1 == nil) {
                if (error)
                    *error = "Failed to create Metal minz stage1 encoder";
                return false;
            }
            [stage1 setComputePipelineState:
                        context.minz_stage1];
            [stage1 setBuffer:vertex_buffer
                       offset:0
                      atIndex:0];
            [stage1 setBuffer:orientation_buffer
                       offset:0
                      atIndex:1];
            [stage1 setBuffer:minz_partial_buffer
                       offset:0
                      atIndex:2];
            [stage1 setBytes:&range
                      length:sizeof(range)
                     atIndex:3];
            [stage1 setThreadgroupMemoryLength:
                        kThreadsPerGroup * sizeof(float)
                                      atIndex:0];
            [stage1 dispatchThreadgroups:
                        MTLSizeMake(
                            chunk,
                            orientation_count,
                            1u)
                    threadsPerThreadgroup:threads];
            [stage1 endEncoding];

            id<MTLComputeCommandEncoder> stage2 =
                [command_buffer computeCommandEncoder];
            if (stage2 == nil) {
                if (error)
                    *error = "Failed to create Metal minz stage2 encoder";
                return false;
            }
            [stage2 setComputePipelineState:
                        context.minz_stage2];
            [stage2 setBuffer:minz_partial_buffer
                       offset:0
                      atIndex:0];
            [stage2 setBuffer:minz_buffer
                       offset:0
                      atIndex:1];
            [stage2 setBytes:&chunk
                      length:sizeof(chunk)
                     atIndex:2];
            [stage2 setThreadgroupMemoryLength:
                        kThreadsPerGroup * sizeof(float)
                                      atIndex:0];
            [stage2 dispatchThreadgroups:
                        MTLSizeMake(
                            1u,
                            orientation_count,
                            1u)
                    threadsPerThreadgroup:threads];
            [stage2 endEncoding];

            if (!command_buffer_succeeded(
                    command_buffer,
                    params.stopcondition,
                    "minz",
                    error))
                return false;
        }

        const bool need_maxz =
            params.orient_type != EOrientType::MinArea;
        if (need_maxz) {
            for (uint32_t group_base = 0u;
                 group_base < minz_group_count;
                 group_base += minz_chunk_count) {
                if (params.stopcondition &&
                    params.stopcondition()) {
                    if (error)
                        *error = "Canceled";
                    return false;
                }
                const uint32_t chunk = std::min(
                    minz_chunk_count,
                    minz_group_count - group_base);
                const RangeParams range{
                    uint32_t(prepared.vertices.size()),
                    chunk,
                    group_base,
                    0u};
                id<MTLCommandBuffer> command_buffer =
                    [context.queue commandBuffer];
                if (command_buffer == nil) {
                    if (error)
                        *error = "Failed to create Metal maxz command buffer";
                    return false;
                }
                command_buffer.label = @"GpuOrient maxz";

                id<MTLComputeCommandEncoder> stage1 =
                    [command_buffer computeCommandEncoder];
                if (stage1 == nil) {
                    if (error)
                        *error = "Failed to create Metal maxz stage1 encoder";
                    return false;
                }
                [stage1 setComputePipelineState:
                            context.maxz_stage1];
                [stage1 setBuffer:vertex_buffer
                           offset:0
                          atIndex:0];
                [stage1 setBuffer:orientation_buffer
                           offset:0
                          atIndex:1];
                [stage1 setBuffer:maxz_partial_buffer
                           offset:0
                          atIndex:2];
                [stage1 setBytes:&range
                          length:sizeof(range)
                         atIndex:3];
                [stage1 setThreadgroupMemoryLength:
                            kThreadsPerGroup *
                                sizeof(float)
                                          atIndex:0];
                [stage1 dispatchThreadgroups:
                            MTLSizeMake(
                                chunk,
                                orientation_count,
                                1u)
                        threadsPerThreadgroup:threads];
                [stage1 endEncoding];

                id<MTLComputeCommandEncoder> stage2 =
                    [command_buffer computeCommandEncoder];
                if (stage2 == nil) {
                    if (error)
                        *error = "Failed to create Metal maxz stage2 encoder";
                    return false;
                }
                [stage2 setComputePipelineState:
                            context.maxz_stage2];
                [stage2 setBuffer:maxz_partial_buffer
                           offset:0
                          atIndex:0];
                [stage2 setBuffer:maxz_buffer
                           offset:0
                          atIndex:1];
                [stage2 setBytes:&chunk
                          length:sizeof(chunk)
                         atIndex:2];
                [stage2 setThreadgroupMemoryLength:
                            kThreadsPerGroup *
                                sizeof(float)
                                          atIndex:0];
                [stage2 dispatchThreadgroups:
                            MTLSizeMake(
                                1u,
                                orientation_count,
                                1u)
                        threadsPerThreadgroup:threads];
                [stage2 endEncoding];

                if (!command_buffer_succeeded(
                        command_buffer,
                        params.stopcondition,
                        "maxz",
                        error))
                    return false;
            }
        }

        const float ascent =
            compute_ascent(float(mesh.overhang_angle));
        for (uint32_t group_base = 0u;
             group_base < cost_group_count;
             group_base += cost_chunk_count) {
            if (params.stopcondition &&
                params.stopcondition()) {
                if (error)
                    *error = "Canceled";
                return false;
            }
            const uint32_t chunk = std::min(
                cost_chunk_count,
                cost_group_count - group_base);
            const CostParams cost_params{
                chunk,
                uint32_t(prepared.faces.size()),
                uint32_t(prepared.hull_faces.size()),
                uint32_t(params.orient_type),
                ascent,
                params.FIRST_LAY_H,
                area_scale,
                params.LAF_MIN,
                params.LAF_MAX,
                kCpuEps,
                group_base,
                0u};
            id<MTLCommandBuffer> command_buffer =
                [context.queue commandBuffer];
            if (command_buffer == nil) {
                if (error)
                    *error = "Failed to create Metal cost command buffer";
                return false;
            }
            command_buffer.label = @"GpuOrient cost";

            id<MTLComputeCommandEncoder> stage1 =
                [command_buffer computeCommandEncoder];
            if (stage1 == nil) {
                if (error)
                    *error = "Failed to create Metal cost stage1 encoder";
                return false;
            }
            [stage1 setComputePipelineState:
                        context.cost_stage1];
            [stage1 setBuffer:vertex_buffer
                       offset:0
                      atIndex:0];
            [stage1 setBuffer:face_buffer
                       offset:0
                      atIndex:1];
            [stage1 setBuffer:hull_face_buffer
                       offset:0
                      atIndex:2];
            [stage1 setBuffer:orientation_buffer
                       offset:0
                      atIndex:3];
            [stage1 setBuffer:minz_buffer
                       offset:0
                      atIndex:4];
            [stage1 setBuffer:partial0_buffer
                       offset:0
                      atIndex:5];
            [stage1 setBuffer:partial1_buffer
                       offset:0
                      atIndex:6];
            [stage1 setBuffer:partial_support_buffer
                       offset:0
                      atIndex:7];
            [stage1 setBuffer:partial2_buffer
                       offset:0
                      atIndex:8];
            [stage1 setBytes:&cost_params
                      length:sizeof(cost_params)
                     atIndex:9];
            [stage1 setThreadgroupMemoryLength:
                        kThreadsPerGroup * sizeof(UInt4)
                                      atIndex:0];
            [stage1 setThreadgroupMemoryLength:
                        kThreadsPerGroup *
                            sizeof(uint32_t)
                                      atIndex:1];
            [stage1 setThreadgroupMemoryLength:
                        kThreadsPerGroup * sizeof(float)
                                      atIndex:2];
            [stage1 setThreadgroupMemoryLength:
                        kThreadsPerGroup * sizeof(UInt4)
                                      atIndex:3];
            [stage1 dispatchThreadgroups:
                        MTLSizeMake(
                            chunk,
                            orientation_count,
                            1u)
                    threadsPerThreadgroup:threads];
            [stage1 endEncoding];

            id<MTLComputeCommandEncoder> stage2 =
                [command_buffer computeCommandEncoder];
            if (stage2 == nil) {
                if (error)
                    *error = "Failed to create Metal cost stage2 encoder";
                return false;
            }
            [stage2 setComputePipelineState:
                        context.cost_stage2];
            [stage2 setBuffer:partial0_buffer
                       offset:0
                      atIndex:0];
            [stage2 setBuffer:partial1_buffer
                       offset:0
                      atIndex:1];
            [stage2 setBuffer:result0_buffer
                       offset:0
                      atIndex:2];
            [stage2 setBuffer:result1_buffer
                       offset:0
                      atIndex:3];
            [stage2 setBuffer:partial_support_buffer
                       offset:0
                      atIndex:4];
            [stage2 setBuffer:partial2_buffer
                       offset:0
                      atIndex:5];
            [stage2 setBuffer:support_buffer
                       offset:0
                      atIndex:6];
            [stage2 setBuffer:result2_buffer
                       offset:0
                      atIndex:7];
            [stage2 setBytes:&chunk
                      length:sizeof(chunk)
                     atIndex:8];
            [stage2 setThreadgroupMemoryLength:
                        kThreadsPerGroup * sizeof(UInt4)
                                      atIndex:0];
            [stage2 setThreadgroupMemoryLength:
                        kThreadsPerGroup *
                            sizeof(uint32_t)
                                      atIndex:1];
            [stage2 setThreadgroupMemoryLength:
                        kThreadsPerGroup * sizeof(float)
                                      atIndex:2];
            [stage2 setThreadgroupMemoryLength:
                        kThreadsPerGroup * sizeof(UInt4)
                                      atIndex:3];
            [stage2 dispatchThreadgroups:
                        MTLSizeMake(
                            1u,
                            orientation_count,
                            1u)
                    threadsPerThreadgroup:threads];
            [stage2 endEncoding];

            if (!command_buffer_succeeded(
                    command_buffer,
                    params.stopcondition,
                    "cost",
                    error))
                return false;
        }

        std::memcpy(
            output.minz_ordered.data(),
            minz_buffer.contents,
            output.minz_ordered.size() * sizeof(int32_t));
        if (need_maxz) {
            std::memcpy(
                output.maxz_ordered.data(),
                maxz_buffer.contents,
                output.maxz_ordered.size() *
                    sizeof(int32_t));
        } else {
            output.maxz_ordered.clear();
        }
        std::memcpy(
            output.result0.data(),
            result0_buffer.contents,
            output.result0.size() * sizeof(UInt4));
        std::memcpy(
            output.result1.data(),
            result1_buffer.contents,
            output.result1.size() * sizeof(UInt4));
        std::memcpy(
            output.support_volume.data(),
            support_buffer.contents,
            output.support_volume.size() * sizeof(float));
        std::memcpy(
            output.result2.data(),
            result2_buffer.contents,
            output.result2.size() * sizeof(UInt4));
        return true;
    }
}

static bool orient_one_mesh(OrientMesh& mesh,
                            const OrientParams& params,
                            std::string* error)
{
    if (mesh.mesh.facets_count() <= 0) {
        if (error)
            *error = "Mesh has no faces";
        return false;
    }

    PreparedMesh prepared;
    if (!prepare_mesh(mesh.mesh, params, prepared, error))
        return false;
    if (prepared.candidates.empty()) {
        if (error)
            *error = "No candidate orientations";
        return false;
    }

    std::vector<OrientationGpu> orientations;
    orientations.reserve(prepared.candidates.size());
    for (const Vec3f& candidate : prepared.candidates) {
        const Vec3f up = (-candidate).normalized();
        orientations.push_back(
            {up.x(), up.y(), up.z(), 0.0f});
    }

    const double maximum_area =
        std::max(prepared.total_plain_area,
                 prepared.total_overhang_area);
    const double maximum_u32 =
        double(std::numeric_limits<uint32_t>::max()) -
        1024.0;
    float area_scale = 1.0f;
    if (maximum_area > 0.0) {
        const double maximum_scale =
            maximum_u32 / maximum_area;
        area_scale = float(std::min(
            10000.0,
            std::max(1.0, std::floor(maximum_scale))));
    }

    Evaluation evaluation;
    if (!evaluate_on_metal(
            prepared,
            orientations,
            mesh,
            params,
            area_scale,
            evaluation,
            error))
        return false;

    struct Pick {
        float cost;
        size_t index;
    };
    std::vector<Pick> picks;
    picks.reserve(orientations.size());

    for (size_t index = 0;
         index < orientations.size();
         ++index) {
        float cost = std::numeric_limits<float>::infinity();
        if (params.orient_type == EOrientType::MinArea) {
            const UInt4 result0 = evaluation.result0[index];
            const float overhang =
                result0.x / area_scale;
            const float bottom_first =
                result0.y / area_scale;
            const float bottom_second =
                result0.z / area_scale;
            const float low_angle_face =
                result0.w / area_scale;
            const float bottom_hull =
                evaluation.result1[index].x / area_scale;
            const float bottom =
                0.5f * bottom_first + bottom_second;
            const float contour =
                4.0f *
                std::sqrt(std::max(bottom, 0.0f));
            const float denominator =
                params.TAR_D +
                params.CONTOUR_F * contour +
                params.BOTTOM_F * bottom +
                params.BOTTOM_HULL_F * bottom_hull;
            if (denominator > 0.0f) {
                cost =
                    params.RELATIVE_F *
                    (overhang * params.TAR_C +
                     params.TAR_D +
                     params.TAR_LAF *
                         low_angle_face *
                         (params.use_low_angle_face
                              ? 1.0f
                              : 0.0f)) /
                    denominator;
            }
            if (bottom < params.BOTTOM_MIN)
                cost += 100.0f;
        } else if (
            params.orient_type == EOrientType::MinVolume) {
            cost = evaluation.support_volume[index];
        } else {
            constexpr float layer_height = 0.2f;
            const float mesh_height =
                ordered_int_to_float(
                    evaluation.maxz_ordered[index]) -
                ordered_int_to_float(
                    evaluation.minz_ordered[index]);
            const float layer_count =
                std::max(1.0f, mesh_height / layer_height);
            const UInt4 result2 = evaluation.result2[index];
            const float surface_arc =
                result2.x / area_scale;
            const float overhang_arc =
                result2.y / area_scale;
            const float fill_area =
                result2.z / area_scale;
            const float top_area =
                result2.w / area_scale;
            const float wall_time =
                (surface_arc + overhang_arc) /
                (layer_height * 200.0f);
            const float fill_time =
                fill_area / (layer_height * 250.0f);
            const float top_time =
                top_area / (0.42f * 200.0f);
            const float support_volume =
                evaluation.support_volume[index];

            const float fill_quantity =
                0.0002f * fill_time * fill_time +
                0.8141f * fill_time + 4.9651f;
            const float wall_quantity =
                -2e-05f * wall_time * wall_time +
                0.6938f * wall_time - 12.877f;
            const float top_quantity =
                6e-05f * top_time * top_time +
                0.6852f * top_time + 0.7016f;
            const float support_quantity =
                5e-09f * support_volume * support_volume +
                0.0049f * support_volume + 26.955f;
            float layer_quantity = 0.0f;
            if (wall_quantity + fill_quantity + top_quantity <
                layer_count) {
                layer_quantity =
                    (layer_count -
                     (wall_quantity +
                      fill_quantity +
                      top_quantity)) /
                    layer_count;
            }
            cost =
                wall_quantity +
                fill_quantity +
                top_quantity +
                layer_count * layer_quantity +
                support_quantity;
        }
        picks.push_back({cost, index});
    }

    std::sort(
        picks.begin(),
        picks.end(),
        [](const Pick& lhs, const Pick& rhs) {
            if (lhs.cost != rhs.cost)
                return lhs.cost < rhs.cost;
            return lhs.index < rhs.index;
        });
    if (picks.empty()) {
        if (error)
            *error = "No orientation result";
        return false;
    }

    size_t best_index = picks.front().index;
    const float best_cost = picks.front().cost;
    const Vec3f global_up{0.0f, 0.0f, 1.0f};
    const OrientationGpu first =
        orientations[best_index];
    const Vec3f first_up{first.x, first.y, first.z};
    if (std::abs(first_up.dot(global_up) - 1.0f) >
        kCpuEps) {
        for (size_t pick_index = 1;
             pick_index < picks.size();
             ++pick_index) {
            if (std::abs(
                    picks[pick_index].cost - best_cost) >
                kCpuEps)
                break;
            const OrientationGpu orientation =
                orientations[picks[pick_index].index];
            const Vec3f up{
                orientation.x,
                orientation.y,
                orientation.z};
            if (std::abs(up.dot(global_up) - 1.0f) <
                kCpuEps * kCpuEps) {
                best_index = picks[pick_index].index;
                break;
            }
        }
    }

    const OrientationGpu best = orientations[best_index];
    const Vec3f best_up{best.x, best.y, best.z};
    mesh.orientation = (-best_up).cast<double>();
    Geometry::rotation_from_two_vectors(
        mesh.orientation,
        {0.0, 0.0, -1.0},
        mesh.axis,
        mesh.angle,
        &mesh.rotation_matrix);
    mesh.euler_angles =
        Geometry::extract_euler_angles(mesh.rotation_matrix);

    BOOST_LOG_TRIVIAL(info)
        << "GpuOrient Metal picked idx=" << best_index
        << " mode=" << int(params.orient_type)
        << " cost=" << best_cost;
    return true;
}

} // namespace

bool available(std::string* error)
{
    @autoreleasepool {
        MetalContext& context = metal_context();
        if (error)
            *error = context.ok ? std::string{} : context.error;
        return context.ok;
    }
}

bool orient(OrientMeshs& items,
            const OrientParams& params,
            std::string* error)
{
    @autoreleasepool {
        if (error)
            error->clear();
        MetalContext& context = metal_context();
        if (!context.ok) {
            if (error)
                *error = context.error;
            return false;
        }

        for (size_t index = 0; index < items.size(); ++index) {
            if (params.stopcondition &&
                params.stopcondition()) {
                if (error)
                    *error = "Canceled";
                return false;
            }
            if (params.progressind)
                params.progressind(
                    unsigned(index), items[index].name);
            if (!orient_one_mesh(items[index], params, error))
                return false;
        }

        if (params.progressind) {
            params.progressind(
                unsigned(items.size()),
                items.empty() ? "" : items.back().name);
        }
        return true;
    }
}

} // namespace metal
} // namespace orientation
} // namespace Slic3r
