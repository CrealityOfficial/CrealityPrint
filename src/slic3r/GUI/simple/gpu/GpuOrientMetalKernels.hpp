#ifndef GPU_ORIENT_METAL_KERNELS_HPP
#define GPU_ORIENT_METAL_KERNELS_HPP

namespace Slic3r {
namespace orientation {
namespace metal {

static constexpr const char* kMetalKernelSource = R"METAL(
#include <metal_stdlib>
using namespace metal;

struct VertexGpu {
    float x, y, z, pad;
};

struct FaceGpu {
    float nx, ny, nz;
    float area_plain;
    float area_overhang;
    uint i0, i1, i2;
    float pad0, pad1;
};

struct HullFaceGpu {
    float area_plain;
    uint i0, i1, i2;
    uint pad0;
};

struct OrientationGpu {
    float x, y, z, pad;
};

struct UInt4 {
    uint x, y, z, w;
};

struct RangeParams {
    uint item_count;
    uint partial_count;
    uint group_base;
    uint pad0;
};

struct CostParams {
    uint partial_count;
    uint face_count;
    uint hull_face_count;
    uint orient_mode;
    float ascent;
    float first_lay_h;
    float area_scale;
    float laf_min_cos;
    float laf_max_cos;
    float eps_z;
    uint group_base;
    uint pad0;
};

inline UInt4 zero4()
{
    return UInt4{0u, 0u, 0u, 0u};
}

inline UInt4 add4(UInt4 a, UInt4 b)
{
    return UInt4{a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

inline int float_to_ordered_int(float value)
{
    int bits = as_type<int>(value);
    return bits < 0 ? int(0x80000000u - uint(bits)) : bits;
}

inline float ordered_int_to_float(int ordered)
{
    int bits = ordered < 0 ? int(0x80000000u - uint(ordered)) : ordered;
    return as_type<float>(bits);
}

kernel void minz_stage1(
    device const VertexGpu* vertices [[buffer(0)]],
    device const OrientationGpu* orientations [[buffer(1)]],
    device float* partial_minz [[buffer(2)]],
    constant RangeParams& params [[buffer(3)]],
    uint lid [[thread_index_in_threadgroup]],
    uint3 group_id [[threadgroup_position_in_grid]],
    threadgroup float* shared_min [[threadgroup(0)]])
{
    const uint gid_local = group_id.x;
    const uint gid = params.group_base + gid_local;
    const uint oid = group_id.y;
    const OrientationGpu orientation = orientations[oid];
    const float3 up = normalize(float3(orientation.x, orientation.y, orientation.z));

    float value = 3.402823466e+38f;
    const uint base = gid * (256u * 4u) + lid;
    for (uint i = 0u; i < 4u; ++i) {
        const uint vertex_id = base + i * 256u;
        if (vertex_id < params.item_count) {
            const VertexGpu vertex_data = vertices[vertex_id];
            value = min(value, dot(float3(vertex_data.x, vertex_data.y, vertex_data.z), up));
        }
    }

    shared_min[lid] = value;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride)
            shared_min[lid] = min(shared_min[lid], shared_min[lid + stride]);
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (lid == 0u)
        partial_minz[oid * params.partial_count + gid_local] = shared_min[0];
}

kernel void minz_stage2(
    device const float* partial_minz [[buffer(0)]],
    device int* minz_ordered [[buffer(1)]],
    constant uint& partial_count [[buffer(2)]],
    uint lid [[thread_index_in_threadgroup]],
    uint3 group_id [[threadgroup_position_in_grid]],
    threadgroup float* shared_min [[threadgroup(0)]])
{
    const uint oid = group_id.y;
    const uint base = oid * partial_count;
    float value = 3.402823466e+38f;
    for (uint i = lid; i < partial_count; i += 256u)
        value = min(value, partial_minz[base + i]);

    shared_min[lid] = value;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride)
            shared_min[lid] = min(shared_min[lid], shared_min[lid + stride]);
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (lid == 0u)
        minz_ordered[oid] = min(minz_ordered[oid], float_to_ordered_int(shared_min[0]));
}

kernel void maxz_stage1(
    device const VertexGpu* vertices [[buffer(0)]],
    device const OrientationGpu* orientations [[buffer(1)]],
    device float* partial_maxz [[buffer(2)]],
    constant RangeParams& params [[buffer(3)]],
    uint lid [[thread_index_in_threadgroup]],
    uint3 group_id [[threadgroup_position_in_grid]],
    threadgroup float* shared_max [[threadgroup(0)]])
{
    const uint gid_local = group_id.x;
    const uint gid = params.group_base + gid_local;
    const uint oid = group_id.y;
    const OrientationGpu orientation = orientations[oid];
    const float3 up = normalize(float3(orientation.x, orientation.y, orientation.z));

    float value = -3.402823466e+38f;
    const uint base = gid * (256u * 4u) + lid;
    for (uint i = 0u; i < 4u; ++i) {
        const uint vertex_id = base + i * 256u;
        if (vertex_id < params.item_count) {
            const VertexGpu vertex_data = vertices[vertex_id];
            value = max(value, dot(float3(vertex_data.x, vertex_data.y, vertex_data.z), up));
        }
    }

    shared_max[lid] = value;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride)
            shared_max[lid] = max(shared_max[lid], shared_max[lid + stride]);
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (lid == 0u)
        partial_maxz[oid * params.partial_count + gid_local] = shared_max[0];
}

kernel void maxz_stage2(
    device const float* partial_maxz [[buffer(0)]],
    device int* maxz_ordered [[buffer(1)]],
    constant uint& partial_count [[buffer(2)]],
    uint lid [[thread_index_in_threadgroup]],
    uint3 group_id [[threadgroup_position_in_grid]],
    threadgroup float* shared_max [[threadgroup(0)]])
{
    const uint oid = group_id.y;
    const uint base = oid * partial_count;
    float value = -3.402823466e+38f;
    for (uint i = lid; i < partial_count; i += 256u)
        value = max(value, partial_maxz[base + i]);

    shared_max[lid] = value;
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride)
            shared_max[lid] = max(shared_max[lid], shared_max[lid + stride]);
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (lid == 0u)
        maxz_ordered[oid] = max(maxz_ordered[oid], float_to_ordered_int(shared_max[0]));
}

kernel void cost_stage1(
    device const VertexGpu* vertices [[buffer(0)]],
    device const FaceGpu* faces [[buffer(1)]],
    device const HullFaceGpu* hull_faces [[buffer(2)]],
    device const OrientationGpu* orientations [[buffer(3)]],
    device const int* minz_ordered [[buffer(4)]],
    device UInt4* partial0 [[buffer(5)]],
    device uint* partial1 [[buffer(6)]],
    device float* partial_support_volume [[buffer(7)]],
    device UInt4* partial2 [[buffer(8)]],
    constant CostParams& params [[buffer(9)]],
    uint lid [[thread_index_in_threadgroup]],
    uint3 group_id [[threadgroup_position_in_grid]],
    threadgroup UInt4* shared0 [[threadgroup(0)]],
    threadgroup uint* shared1 [[threadgroup(1)]],
    threadgroup float* shared_support_volume [[threadgroup(2)]],
    threadgroup UInt4* shared2 [[threadgroup(3)]])
{
    const uint gid_local = group_id.x;
    const uint gid = params.group_base + gid_local;
    const uint oid = group_id.y;
    const OrientationGpu orientation = orientations[oid];
    const float3 up = normalize(float3(orientation.x, orientation.y, orientation.z));
    const float minz = ordered_int_to_float(minz_ordered[oid]);

    UInt4 acc0 = zero4();
    uint acc1 = 0u;
    float acc_support_volume = 0.0f;
    UInt4 acc2 = zero4();

    const uint base = gid * (256u * 4u) + lid;
    for (uint iteration = 0u; iteration < 4u; ++iteration) {
        const uint face_id = base + iteration * 256u;
        if (face_id < params.face_count) {
            const FaceGpu face = faces[face_id];
            const VertexGpu vertex0 = vertices[face.i0];
            const VertexGpu vertex1 = vertices[face.i1];
            const VertexGpu vertex2 = vertices[face.i2];
            const float3 point0 = float3(vertex0.x, vertex0.y, vertex0.z);
            const float3 point1 = float3(vertex1.x, vertex1.y, vertex1.z);
            const float3 point2 = float3(vertex2.x, vertex2.y, vertex2.z);

            const float z0 = dot(point0, up);
            const float z1 = dot(point1, up);
            const float z2 = dot(point2, up);
            const float zmax = max(z0, max(z1, z2));
            const bool bottom_first =
                zmax < minz + params.first_lay_h - params.eps_z;
            const bool bottom_second =
                zmax < minz + 0.5f * params.first_lay_h - params.eps_z;
            const float direction = dot(float3(face.nx, face.ny, face.nz), up);

            const uint plain_area =
                uint(max(0.0f, face.area_plain * params.area_scale + 0.5f));
            const uint overhang_area =
                uint(max(0.0f, face.area_overhang * params.area_scale + 0.5f));

            if (bottom_first)
                acc0.y += plain_area;
            if (bottom_second)
                acc0.z += plain_area;
            if (direction < params.ascent && !bottom_second)
                acc0.x += overhang_area;

            const float absolute_direction = abs(direction);
            if (absolute_direction < params.laf_max_cos &&
                absolute_direction > params.laf_min_cos &&
                zmax > minz + params.first_lay_h)
                acc0.w += plain_area;

            if (params.orient_mode != 0u) {
                const float zmean = (z0 + z1 + z2) / 3.0f;
                if (direction < params.ascent && !bottom_second) {
                    const float height = max(0.0f, zmean - minz);
                    const float inner = max(0.0f, params.ascent - direction);
                    acc_support_volume +=
                        height * face.area_overhang * inner;
                }
            }

            if (params.orient_mode == 2u) {
                const float arc =
                    sqrt(max(0.0f, 1.0f - direction * direction));
                const bool vertical = abs(direction) > 0.9999f;
                if (!vertical) {
                    const uint arc_area =
                        uint(face.area_plain * arc * params.area_scale + 0.5f);
                    if (direction <= -0.7072f)
                        acc2.y += arc_area;
                    else
                        acc2.x += arc_area;
                }
                if (direction <= -0.54463f)
                    acc2.z += uint(face.area_plain * params.area_scale + 0.5f);
                if (direction > 0.98f)
                    acc2.w += uint(face.area_plain * params.area_scale + 0.5f);
            }
        }

        const uint hull_face_id = base + iteration * 256u;
        if (hull_face_id < params.hull_face_count) {
            const HullFaceGpu face = hull_faces[hull_face_id];
            const VertexGpu vertex0 = vertices[face.i0];
            const VertexGpu vertex1 = vertices[face.i1];
            const VertexGpu vertex2 = vertices[face.i2];
            const float z0 = dot(float3(vertex0.x, vertex0.y, vertex0.z), up);
            const float z1 = dot(float3(vertex1.x, vertex1.y, vertex1.z), up);
            const float z2 = dot(float3(vertex2.x, vertex2.y, vertex2.z), up);
            const float zmax = max(z0, max(z1, z2));
            if (zmax < minz + params.first_lay_h - params.eps_z)
                acc1 += uint(max(
                    0.0f, face.area_plain * params.area_scale + 0.5f));
        }
    }

    shared0[lid] = acc0;
    shared1[lid] = acc1;
    shared_support_volume[lid] = acc_support_volume;
    shared2[lid] = acc2;
    threadgroup_barrier(mem_flags::mem_threadgroup);

    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride) {
            shared0[lid] = add4(shared0[lid], shared0[lid + stride]);
            shared1[lid] += shared1[lid + stride];
            shared_support_volume[lid] +=
                shared_support_volume[lid + stride];
            shared2[lid] = add4(shared2[lid], shared2[lid + stride]);
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (lid == 0u) {
        const uint output_index =
            oid * params.partial_count + gid_local;
        partial0[output_index] = shared0[0];
        partial1[output_index] = shared1[0];
        partial_support_volume[output_index] = shared_support_volume[0];
        partial2[output_index] = shared2[0];
    }
}

kernel void cost_stage2(
    device const UInt4* partial0 [[buffer(0)]],
    device const uint* partial1 [[buffer(1)]],
    device UInt4* result0 [[buffer(2)]],
    device UInt4* result1 [[buffer(3)]],
    device const float* partial_support_volume [[buffer(4)]],
    device const UInt4* partial2 [[buffer(5)]],
    device float* support_volume_result [[buffer(6)]],
    device UInt4* result2 [[buffer(7)]],
    constant uint& partial_count [[buffer(8)]],
    uint lid [[thread_index_in_threadgroup]],
    uint3 group_id [[threadgroup_position_in_grid]],
    threadgroup UInt4* shared0 [[threadgroup(0)]],
    threadgroup uint* shared1 [[threadgroup(1)]],
    threadgroup float* shared_support_volume [[threadgroup(2)]],
    threadgroup UInt4* shared2 [[threadgroup(3)]])
{
    const uint oid = group_id.y;
    const uint base = oid * partial_count;
    UInt4 acc0 = zero4();
    uint acc1 = 0u;
    float acc_support_volume = 0.0f;
    UInt4 acc2 = zero4();

    for (uint i = lid; i < partial_count; i += 256u) {
        acc0 = add4(acc0, partial0[base + i]);
        acc1 += partial1[base + i];
        acc_support_volume += partial_support_volume[base + i];
        acc2 = add4(acc2, partial2[base + i]);
    }

    shared0[lid] = acc0;
    shared1[lid] = acc1;
    shared_support_volume[lid] = acc_support_volume;
    shared2[lid] = acc2;
    threadgroup_barrier(mem_flags::mem_threadgroup);

    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride) {
            shared0[lid] = add4(shared0[lid], shared0[lid + stride]);
            shared1[lid] += shared1[lid + stride];
            shared_support_volume[lid] +=
                shared_support_volume[lid + stride];
            shared2[lid] = add4(shared2[lid], shared2[lid + stride]);
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }

    if (lid == 0u) {
        result0[oid] = add4(result0[oid], shared0[0]);
        result1[oid].x += shared1[0];
        support_volume_result[oid] += shared_support_volume[0];
        result2[oid] = add4(result2[oid], shared2[0]);
    }
}
)METAL";

} // namespace metal
} // namespace orientation
} // namespace Slic3r

#endif // GPU_ORIENT_METAL_KERNELS_HPP
