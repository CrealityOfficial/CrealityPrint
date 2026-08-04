#include "GpuOrient.hpp"

#include "libslic3r/Geometry.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <boost/log/trivial.hpp>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <glad/gl.h>
#elif defined(__linux__)
#include <EGL/egl.h>
#include <glad/gl.h>
#endif

namespace Slic3r {
namespace orientation {

namespace {

#if defined(_WIN32) || (defined(__linux__) && !defined(__APPLE__))

constexpr float kPi = 3.14159265358979323846f;
// CPU code uses an EPSILON for z-gating and tie-breaks; keep GPU behavior aligned.
constexpr float kCpuEps = 1e-6f;

// --------------------------- minimal hidden GL context ---------------------------

#if defined(_WIN32)

struct GLWinContext {
    HWND  hwnd  = nullptr;
    HDC   hdc   = nullptr;
    HGLRC hglrc = nullptr;
    bool  owning = true;

    GLWinContext() = default;
    GLWinContext(const GLWinContext&) = delete;
    GLWinContext& operator=(const GLWinContext&) = delete;

    GLWinContext(GLWinContext&& other) noexcept
    {
        hwnd   = other.hwnd;   other.hwnd = nullptr;
        hdc    = other.hdc;    other.hdc = nullptr;
        hglrc  = other.hglrc;  other.hglrc = nullptr;
        owning = other.owning; other.owning = false;
    }

    GLWinContext& operator=(GLWinContext&& other) noexcept
    {
        if (this != &other) {
            reset();
            hwnd   = other.hwnd;   other.hwnd = nullptr;
            hdc    = other.hdc;    other.hdc = nullptr;
            hglrc  = other.hglrc;  other.hglrc = nullptr;
            owning = other.owning; other.owning = false;
        }
        return *this;
    }

    ~GLWinContext() { reset(); }

    void reset() noexcept
    {
        if (!owning)
            return;

        // Guard against re-entry and double release.
        owning = false;

        // Move handles into local variables and clear the members first.
        HWND  local_hwnd  = hwnd;
        HDC   local_hdc   = hdc;
        HGLRC local_hglrc = hglrc;

        hwnd  = nullptr;
        hdc   = nullptr;
        hglrc = nullptr;

        // 1) Unbind only if this context is currently bound on this thread.
        if (local_hglrc) {
            HGLRC cur = wglGetCurrentContext();
            if (cur == local_hglrc) {
                wglMakeCurrent(nullptr, nullptr);
            }

            // Note: if this context is still current on another thread, deletion
            // may still block or fail here. Make sure the worker thread has
            // stopped before destroying the GL context.
            wglDeleteContext(local_hglrc);
        }

        // 2) Release the device context.
        if (local_hwnd && local_hdc) {
            ReleaseDC(local_hwnd, local_hdc);
        }

        // 3) Destroy the hidden window.
        if (local_hwnd) {
            DestroyWindow(local_hwnd);
        }
    }

};

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_CLOSE: PostQuitMessage(0); return 0;
    default: return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

GLWinContext create_gl_context()
{
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    const char* clsName = "gpu_orient_gl_cls";

    WNDCLASSA wc{};
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = clsName;
    ATOM cls = RegisterClassA(&wc);
    if (!cls && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        throw std::runtime_error("RegisterClass failed for GPU orient window");

    HWND hwnd = CreateWindowA(clsName, "GpuOrient", WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 64, 64,
                              nullptr, nullptr, hInstance, nullptr);
    if (!hwnd)
        throw std::runtime_error("CreateWindow failed for GPU orient");

    HDC hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = 24;
    pfd.cDepthBits   = 24;
    pfd.iLayerType   = PFD_MAIN_PLANE;

    int pf = ChoosePixelFormat(hdc, &pfd);
    if (pf == 0 || !SetPixelFormat(hdc, pf, &pfd))
        throw std::runtime_error("Failed to set pixel format for GPU orient");

    HGLRC legacy = wglCreateContext(hdc);
    if (!legacy)
        throw std::runtime_error("Failed to create legacy GL context");
    wglMakeCurrent(hdc, legacy);

    using PFNWGLCREATECONTEXTATTRIBSARBPROC = HGLRC(WINAPI*)(HDC, HGLRC, const int*);
    auto wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(
        wglGetProcAddress("wglCreateContextAttribsARB"));

    HGLRC modern = nullptr;
    if (wglCreateContextAttribsARB) {
        const int attribs[] = {
            0x2091 /*WGL_CONTEXT_MAJOR_VERSION_ARB*/, 4,
            0x2092 /*WGL_CONTEXT_MINOR_VERSION_ARB*/, 3,
            0x9126 /*WGL_CONTEXT_PROFILE_MASK_ARB*/,  0x00000001 /*CORE*/,
            0,
        };
        modern = wglCreateContextAttribsARB(hdc, nullptr, attribs);
    }
    if (modern) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(legacy);
        wglMakeCurrent(hdc, modern);
    } else {
        modern = legacy;
    }

    GLWinContext ctx;
    ctx.hwnd  = hwnd;
    ctx.hdc   = hdc;
    ctx.hglrc = modern;
    return ctx;
}

#endif // _WIN32

// --------------------------- data packing ---------------------------

struct VertexGpu {
    float x, y, z, pad; // std430 aligned
};

struct FaceGpu {
    // normal
    float nx, ny, nz;
    float area_plain;
    float area_overhang;
    uint32_t i0, i1, i2;
    float pad0, pad1; // keep 32B-ish alignment
};

struct HullFaceGpu {
    float area_plain;
    uint32_t i0, i1, i2;
    uint32_t pad0;
};

struct OrientationGpu {
    float x, y, z, pad;
};

static_assert(sizeof(VertexGpu) == 16, "VertexGpu must match GLSL vec4 packing");
static_assert(sizeof(OrientationGpu) == 16, "OrientationGpu must match GLSL vec4 packing");
static_assert(sizeof(FaceGpu) == 40, "FaceGpu must match GLSL std430 packed Face (40B stride)");
static_assert(sizeof(HullFaceGpu) == 20, "HullFaceGpu must match GLSL std430 packed HullFace (20B stride)");

// hash + quantize for candidates
struct VecHash {
    size_t operator()(const Vec3f& n1) const {
        return std::hash<int>()(int(n1(0) * 1000 + 1000)) ^
               (std::hash<int>()(int(n1(1) * 1000 + 1000)) << 1) ^
               (std::hash<int>()(int(n1(2) * 1000 + 1000)) << 2);
    }
};

// CPU mirror of the GLSL orderedIntToFloat - decode ordered-int back to float.
static inline float orderedIntToFloat(int32_t o)
{
    int32_t i = (o < 0) ? (int32_t)(0x80000000u - (uint32_t)o) : o;
    float f;
    std::memcpy(&f, &i, sizeof(f));
    return f;
}

static inline Vec3f quantize_vec3f(const Vec3f& n)
{
    return Vec3f(std::floor(n(0) * 1000.f) / 1000.f,
                 std::floor(n(1) * 1000.f) / 1000.f,
                 std::floor(n(2) * 1000.f) / 1000.f);
}

static inline void remove_duplicates(std::vector<Vec3f>& orientations, double tol = 1e-7)
{
    if (orientations.size() <= 1) return;
    for (auto it = orientations.begin() + 1; it < orientations.end();) {
        bool dup = false;
        for (auto ok = orientations.begin(); ok < it; ++ok) {
            if (ok->isApprox(*it, tol)) { dup = true; break; }
        }
        const Vec3f zero{0, 0, 0};
        if (dup || it->isApprox(zero, tol))
            it = orientations.erase(it);
        else
            ++it;
    }
}

static inline void add_supplements(std::vector<Vec3f>& orientations)
{
    static const Vec3f extra[] = {
        {0, 0, -1}, {0.70710678f, 0, -0.70710678f}, {0, 0.70710678f, -0.70710678f},
        {-0.70710678f, 0, -0.70710678f}, {0, -0.70710678f, -0.70710678f},
        {1, 0, 0}, {0.70710678f, 0.70710678f, 0}, {0, 1, 0}, {-0.70710678f, 0.70710678f, 0},
        {-1, 0, 0}, {-0.70710678f, -0.70710678f, 0}, {0, -1, 0}, {0.70710678f, -0.70710678f, 0},
        {0.70710678f, 0, 0.70710678f}, {0, 0.70710678f, 0.70710678f},
        {-0.70710678f, 0, 0.70710678f}, {0, -0.70710678f, 0.70710678f}, {0, 0, 1}
    };
    orientations.insert(orientations.end(), std::begin(extra), std::end(extra));
}

static inline float compute_ascent(float overhang_angle_deg)
{
    // matches CPU: cos(PI - (angle+1)*PI/180)
    return std::cos(kPi - (overhang_angle_deg + 1.f) * kPi / 180.f);
}

// --------------------------- compute shaders (A-full) ---------------------------

// Two-stage reductions:
//   - stage1: per-workgroup partials (no global atomics)
//   - stage2: per-orientation final reduction (one workgroup per orientation)
constexpr const char* kMinZStage1ShaderSrc = R"(#version 430
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer VertexBuffer { vec4 vertices[]; };
layout(std430, binding = 1) readonly buffer OrientationBuffer { vec4 orientations[]; };
layout(std430, binding = 2) writeonly buffer PartialMinZBuffer { float partial_minz[]; };

uniform uint vertex_count;
uniform uint partial_count; // number of workgroups in X for stage1
uniform uint group_base;    // base workgroup index in X

shared float smin[256];

void main()
{
    uint lid = gl_LocalInvocationID.x;
    uint gid_local = gl_WorkGroupID.x;
    uint gid = group_base + gid_local;
    uint oid = gl_WorkGroupID.y;

    vec3 up = normalize(orientations[oid].xyz);

    float m = 3.402823466e+38; // FLT_MAX
    uint base = gid * (256u * 4u) + lid;
    for (uint i = 0u; i < 4u; ++i) {
        uint vid = base + i * 256u;
        if (vid < vertex_count) {
            float z = dot(vertices[vid].xyz, up);
            m = min(m, z);
        }
    }

    smin[lid] = m;
    barrier();

    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride)
            smin[lid] = min(smin[lid], smin[lid + stride]);
        barrier();
    }

    if (lid == 0u)
        partial_minz[oid * partial_count + gid_local] = smin[0];
}
)";

constexpr const char* kMinZStage2ShaderSrc = R"(#version 430
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer PartialMinZBuffer { float partial_minz[]; };
layout(std430, binding = 1) buffer MinZBuffer { int minz_ord[]; };

uniform uint partial_count;

// order-preserving float->int for comparisons (total order)
int floatToOrderedInt(float v) {
    int i = floatBitsToInt(v);
    return (i < 0) ? (0x80000000 - i) : i;
}

shared float smin[256];

void main()
{
    uint lid = gl_LocalInvocationID.x;
    uint oid = gl_WorkGroupID.y;

    float m = 3.402823466e+38;
    uint base = oid * partial_count;
    for (uint i = lid; i < partial_count; i += 256u)
        m = min(m, partial_minz[base + i]);

    smin[lid] = m;
    barrier();

    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride)
            smin[lid] = min(smin[lid], smin[lid + stride]);
        barrier();
    }

    if (lid == 0u) {
        int v = floatToOrderedInt(smin[0]);
        minz_ord[oid] = min(minz_ord[oid], v);
    }
}
)";

constexpr const char* kMaxZStage1ShaderSrc = R"(#version 430
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer VertexBuffer    { vec4 vertices[]; };
layout(std430, binding = 1) readonly buffer OrientationBuffer { vec4 orientations[]; };
layout(std430, binding = 2) writeonly buffer PartialMaxZBuffer { float partial_maxz[]; };

uniform uint vertex_count;
uniform uint partial_count;
uniform uint group_base;

shared float smax[256];

void main()
{
    uint lid = gl_LocalInvocationID.x;
    uint gid_local = gl_WorkGroupID.x;
    uint gid = group_base + gid_local;
    uint oid = gl_WorkGroupID.y;

    vec3 up = normalize(orientations[oid].xyz);

    float m = -3.402823466e+38; // -FLT_MAX
    uint base = gid * (256u * 4u) + lid;
    for (uint i = 0u; i < 4u; ++i) {
        uint vid = base + i * 256u;
        if (vid < vertex_count) {
            float z = dot(vertices[vid].xyz, up);
            m = max(m, z);
        }
    }

    smax[lid] = m;
    barrier();

    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride)
            smax[lid] = max(smax[lid], smax[lid + stride]);
        barrier();
    }

    if (lid == 0u)
        partial_maxz[oid * partial_count + gid_local] = smax[0];
}
)";

constexpr const char* kMaxZStage2ShaderSrc = R"(#version 430
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer PartialMaxZBuffer { float partial_maxz[]; };
layout(std430, binding = 1) buffer MaxZBuffer { int maxz_ord[]; };

uniform uint partial_count;

int floatToOrderedInt(float v) {
    int i = floatBitsToInt(v);
    return (i < 0) ? (0x80000000 - i) : i;
}

shared float smax[256];

void main()
{
    uint lid = gl_LocalInvocationID.x;
    uint oid = gl_WorkGroupID.y;

    float m = -3.402823466e+38;
    uint base = oid * partial_count;
    for (uint i = lid; i < partial_count; i += 256u)
        m = max(m, partial_maxz[base + i]);

    smax[lid] = m;
    barrier();

    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride)
            smax[lid] = max(smax[lid], smax[lid + stride]);
        barrier();
    }

    if (lid == 0u) {
        int v = floatToOrderedInt(smax[0]);
        maxz_ord[oid] = max(maxz_ord[oid], v);
    }
}
)";

constexpr const char* kCostStage1ShaderSrc = R"(#version 430
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

struct Face {
    // Matches C++ FaceGpu packed layout (std430 array stride = 40 bytes).
    float nx, ny, nz;
    float area_plain;
    float area_overhang;
    uint  i0, i1, i2;
    float pad0, pad1;
};
struct HullFace {
    // Matches C++ HullFaceGpu packed layout (std430 array stride = 20 bytes).
    float area_plain;
    uint  i0, i1, i2;
    uint   pad0;
};

layout(std430, binding = 0) readonly buffer VertexBuffer { vec4 vertices[]; };
layout(std430, binding = 1) readonly buffer FaceBuffer   { Face faces[]; };
layout(std430, binding = 2) readonly buffer HullFaceBuffer { HullFace hfaces[]; };
layout(std430, binding = 3) readonly buffer OrientationBuffer { vec4 orientations[]; };
layout(std430, binding = 4) readonly buffer MinZBuffer { int minz_ord[]; };

layout(std430, binding = 5) writeonly buffer Partial0Buffer { uvec4 partial0[]; }; // x:overhang y:bottom1 z:bottom2 w:laf
layout(std430, binding = 6) writeonly buffer Partial1Buffer { uint  partial1[]; }; // bottom_hull
layout(std430, binding = 7) writeonly buffer PartialSvBuffer { float partial_sv[]; }; // support_vol (MinVolume+MinTime)
layout(std430, binding = 8) writeonly buffer Partial2Buffer  { uvec4 partial2[];  }; // surf/ovhg/fill/top arc (MinTime)

uniform uint  partial_count;
uniform uint  face_count;
uniform uint  hull_face_count;
uniform float ascent;
uniform float first_lay_h;
uniform float area_scale;
uniform float laf_min_cos;
uniform float laf_max_cos;
uniform float eps_z;
uniform uint  group_base;   // base workgroup index in X
uniform uint  orient_mode;  // 0=MinArea, 1=MinVolume, 2=MinTime

float orderedIntToFloat(int o) {
    int i = (o < 0) ? (0x80000000 - o) : o;
    return intBitsToFloat(i);
}

shared uvec4 s0[256];
shared uint  s1[256];
shared float s_sv[256];
shared uvec4 s2[256];

void main()
{
    uint lid = gl_LocalInvocationID.x;
    uint gid_local = gl_WorkGroupID.x;
    uint gid = group_base + gid_local;
    uint oid = gl_WorkGroupID.y;

    vec3 up   = normalize(orientations[oid].xyz);
    float minz = orderedIntToFloat(minz_ord[oid]);

    uvec4 acc0 = uvec4(0u); // x:overhang y:bottom1 z:bottom2 w:laf
    uint  acc1 = 0u;        // bottom_hull
    float acc_sv = 0.0;     // support_vol (MinVolume + MinTime)
    uvec4 acc2   = uvec4(0u); // surf_arc/ovhg_arc/fill/top (MinTime)

    uint base = gid * (256u * 4u) + lid;
    for (uint it = 0u; it < 4u; ++it) {
        uint fid = base + it * 256u;
        if (fid < face_count) {
            Face f = faces[fid];

            vec3 p0 = vertices[f.i0].xyz;
            vec3 p1 = vertices[f.i1].xyz;
            vec3 p2 = vertices[f.i2].xyz;

            float z0 = dot(p0, up);
            float z1 = dot(p1, up);
            float z2 = dot(p2, up);
            float zmax = max(z0, max(z1, z2));

            bool bottom_1st = (zmax < minz + first_lay_h - eps_z);
            bool bottom_2nd = (zmax < minz + 0.5 * first_lay_h - eps_z);

            float d = dot(vec3(f.nx, f.ny, f.nz), up);

            uint a_plain = uint(max(0.0, f.area_plain    * area_scale + 0.5));
            uint a_ov    = uint(max(0.0, f.area_overhang * area_scale + 0.5));

            if (bottom_1st) acc0.y += a_plain;
            if (bottom_2nd) acc0.z += a_plain;
            if (d < ascent && !bottom_2nd) acc0.x += a_ov;

            float ad = abs(d);
            if (ad < laf_max_cos && ad > laf_min_cos && (zmax > minz + first_lay_h))
                acc0.w += a_plain;

            // MinVolume + MinTime: support volume approximation (no ray casting, uses z_mean)
            if (orient_mode != 0u) {
                float z_mean = (z0 + z1 + z2) / 3.0;
                if (d < ascent && !bottom_2nd) {
                    float h     = max(0.0, z_mean - minz);
                    float inner = max(0.0, ascent - d);
                    acc_sv += h * f.area_overhang * inner;
                }
            }

            // MinTime: surface area components (GPU d-space equivalents of getCostTime() quantities)
            if (orient_mode == 2u) {
                float arc     = sqrt(max(0.0, 1.0 - d * d));
                bool  is_vert = (abs(d) > 0.9999);
                if (!is_vert) {
                    if (d <= -0.7072) acc2.y += uint(f.area_plain * arc * area_scale + 0.5); // ovhg_area_arc
                    else              acc2.x += uint(f.area_plain * arc * area_scale + 0.5); // surf_area_arc
                }
                if (d <= -0.54463) acc2.z += uint(f.area_plain * area_scale + 0.5); // fill_area
                if (d >  0.98)     acc2.w += uint(f.area_plain * area_scale + 0.5); // top_area
            }
        }

        uint hid = base + it * 256u;
        if (hid < hull_face_count) {
            HullFace hf = hfaces[hid];

            vec3 p0 = vertices[hf.i0].xyz;
            vec3 p1 = vertices[hf.i1].xyz;
            vec3 p2 = vertices[hf.i2].xyz;

            float z0 = dot(p0, up);
            float z1 = dot(p1, up);
            float z2 = dot(p2, up);
            float zmax = max(z0, max(z1, z2));

            bool bottom_h = (zmax < minz + first_lay_h - eps_z);
            if (bottom_h)
                acc1 += uint(max(0.0, hf.area_plain * area_scale + 0.5));
        }
    }

    s0[lid]   = acc0;
    s1[lid]   = acc1;
    s_sv[lid] = acc_sv;
    s2[lid]   = acc2;
    barrier();

    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride) {
            s0[lid]   += s0[lid + stride];
            s1[lid]   += s1[lid + stride];
            s_sv[lid] += s_sv[lid + stride];
            s2[lid]   += s2[lid + stride];
        }
        barrier();
    }

    if (lid == 0u) {
        uint out_index = oid * partial_count + gid_local;
        partial0[out_index] = s0[0];
        partial1[out_index] = s1[0];
        partial_sv[out_index] = s_sv[0];
        partial2[out_index]   = s2[0];
    }
}
)";

constexpr const char* kCostStage2ShaderSrc = R"(#version 430
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer Partial0Buffer { uvec4 partial0[]; };
layout(std430, binding = 1) readonly buffer Partial1Buffer { uint  partial1[]; };
layout(std430, binding = 2) buffer Result0Buffer  { uvec4 result0[]; };
layout(std430, binding = 3) buffer Result1Buffer  { uvec4 result1[]; };
layout(std430, binding = 4) readonly buffer PartialSvBuffer { float partial_sv[]; };
layout(std430, binding = 5) readonly buffer Partial2Buffer  { uvec4 partial2[];  };
layout(std430, binding = 6) buffer SvBufResult    { float sv_buf[];  };
layout(std430, binding = 7) buffer Result2Buffer  { uvec4 result2[]; };

uniform uint partial_count;

shared uvec4 s0[256];
shared uint  s1[256];
shared float s_sv[256];
shared uvec4 s2[256];

void main()
{
    uint lid = gl_LocalInvocationID.x;
    uint oid = gl_WorkGroupID.y;

    uvec4 acc0  = uvec4(0u);
    uint  acc1  = 0u;
    float acc_sv = 0.0;
    uvec4 acc2   = uvec4(0u);

    uint base = oid * partial_count;
    for (uint i = lid; i < partial_count; i += 256u) {
        acc0   += partial0[base + i];
        acc1   += partial1[base + i];
        acc_sv += partial_sv[base + i];
        acc2   += partial2[base + i];
    }

    s0[lid]   = acc0;
    s1[lid]   = acc1;
    s_sv[lid] = acc_sv;
    s2[lid]   = acc2;
    barrier();

    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride) {
            s0[lid]   += s0[lid + stride];
            s1[lid]   += s1[lid + stride];
            s_sv[lid] += s_sv[lid + stride];
            s2[lid]   += s2[lid + stride];
        }
        barrier();
    }

    if (lid == 0u) {
        result0[oid]   += s0[0];
        result1[oid].x += s1[0];
        sv_buf[oid]    += s_sv[0];
        result2[oid]   += s2[0];
    }
}
)";

// --------------------------- helper compile ---------------------------

static GLuint compile_compute_program(const char* src, std::string* out_log)
{
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        if (out_log) *out_log = log;
        glDeleteShader(shader);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, shader);
    glLinkProgram(prog);
    glDeleteShader(shader);

    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len, '\0');
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        if (out_log) *out_log = log;
        glDeleteProgram(prog);
        return 0;
    }

    if (out_log) out_log->clear();
    return prog;
}

#endif // _WIN32 || __linux__

// --------------------------- Linux EGL context ---------------------------

#if defined(__linux__) && !defined(__APPLE__)

struct GLLinuxContext {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    bool       owning  = true;

    GLLinuxContext() = default;
    GLLinuxContext(const GLLinuxContext&) = delete;
    GLLinuxContext& operator=(const GLLinuxContext&) = delete;

    GLLinuxContext(GLLinuxContext&& other) noexcept
    {
        display = other.display; other.display = EGL_NO_DISPLAY;
        surface = other.surface; other.surface = EGL_NO_SURFACE;
        context = other.context; other.context = EGL_NO_CONTEXT;
        owning  = other.owning;  other.owning  = false;
    }

    GLLinuxContext& operator=(GLLinuxContext&& other) noexcept
    {
        if (this != &other) {
            reset();
            display = other.display; other.display = EGL_NO_DISPLAY;
            surface = other.surface; other.surface = EGL_NO_SURFACE;
            context = other.context; other.context = EGL_NO_CONTEXT;
            owning  = other.owning;  other.owning  = false;
        }
        return *this;
    }

    ~GLLinuxContext() { reset(); }

    void reset() noexcept
    {
        if (!owning)
            return;
        owning = false;

        EGLDisplay local_display = display;
        EGLSurface local_surface = surface;
        EGLContext local_context = context;
        display = EGL_NO_DISPLAY;
        surface = EGL_NO_SURFACE;
        context = EGL_NO_CONTEXT;

        if (local_display == EGL_NO_DISPLAY)
            return;

        // Unbind current context on this thread if it's ours.
        if (eglGetCurrentContext() == local_context)
            eglMakeCurrent(local_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (local_context != EGL_NO_CONTEXT)
            eglDestroyContext(local_display, local_context);
        if (local_surface != EGL_NO_SURFACE)
            eglDestroySurface(local_display, local_surface);
        eglTerminate(local_display);
    }
};

GLLinuxContext create_gl_context_egl()
{
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy == EGL_NO_DISPLAY)
        throw std::runtime_error("GpuOrient: eglGetDisplay returned EGL_NO_DISPLAY");

    EGLint major = 0, minor = 0;
    if (!eglInitialize(dpy, &major, &minor))
        throw std::runtime_error("GpuOrient: eglInitialize failed");

    if (!eglBindAPI(EGL_OPENGL_API))
        throw std::runtime_error("GpuOrient: eglBindAPI(EGL_OPENGL_API) failed");

    // Choose a pbuffer-capable config with OpenGL rendering.
    const EGLint cfg_attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_NONE
    };
    EGLConfig cfg = nullptr;
    EGLint    ncfg = 0;
    if (!eglChooseConfig(dpy, cfg_attribs, &cfg, 1, &ncfg) || ncfg == 0)
        throw std::runtime_error("GpuOrient: eglChooseConfig found no suitable config");

    // Create a minimal 1x1 pbuffer surface (required by some drivers).
    const EGLint pbuf_attribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    EGLSurface surf = eglCreatePbufferSurface(dpy, cfg, pbuf_attribs);
    if (surf == EGL_NO_SURFACE)
        throw std::runtime_error("GpuOrient: eglCreatePbufferSurface failed");

    // Request an OpenGL 4.3 Core Profile context.
    const EGLint ctx_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION,       4,
        EGL_CONTEXT_MINOR_VERSION,       3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctx_attribs);
    if (ctx == EGL_NO_CONTEXT)
        throw std::runtime_error("GpuOrient: eglCreateContext(4.3 core) failed");

    if (!eglMakeCurrent(dpy, surf, surf, ctx)) {
        eglDestroyContext(dpy, ctx);
        eglDestroySurface(dpy, surf);
        eglTerminate(dpy);
        throw std::runtime_error("GpuOrient: initial eglMakeCurrent failed");
    }

    GLLinuxContext result;
    result.display = dpy;
    result.surface = surf;
    result.context = ctx;
    return result;
}

#endif // __linux__

} // namespace

// --------------------------- Impl ---------------------------

struct GpuOrient::Impl {
#if defined(_WIN32) || (defined(__linux__) && !defined(__APPLE__))

#if defined(_WIN32)
    using PlatformContext = GLWinContext;
#else
    using PlatformContext = GLLinuxContext;
#endif

    PlatformContext ctx;
    std::mutex      mutex;
    bool            ok = false;
    std::string     init_error;

    GLuint program_minz_stage1 = 0;
    GLuint program_minz_stage2 = 0;
    GLuint program_maxz_stage1 = 0;
    GLuint program_maxz_stage2 = 0;
    GLuint program_cost_stage1 = 0;
    GLuint program_cost_stage2 = 0;

    Impl()
    {
        try {
#if defined(_WIN32)
            ctx = create_gl_context();
#else
            ctx = create_gl_context_egl();
#endif
            ok = rebuild(/*force_new=*/false);
        } catch (const std::exception& e) {
            init_error = e.what();
            ok = false;
            BOOST_LOG_TRIVIAL(error) << "GpuOrient init failed: " << init_error;
        }
    }

    ~Impl()
    {
        std::lock_guard<std::mutex> lk(mutex);

        bool has_ctx =
#if defined(_WIN32)
            ctx.hglrc != nullptr;
#else
            ctx.context != EGL_NO_CONTEXT;
#endif
        if (has_ctx && make_current()) {
            if (program_minz_stage1) { glDeleteProgram(program_minz_stage1); program_minz_stage1 = 0; }
            if (program_minz_stage2) { glDeleteProgram(program_minz_stage2); program_minz_stage2 = 0; }
            if (program_maxz_stage1) { glDeleteProgram(program_maxz_stage1); program_maxz_stage1 = 0; }
            if (program_maxz_stage2) { glDeleteProgram(program_maxz_stage2); program_maxz_stage2 = 0; }
            if (program_cost_stage1) { glDeleteProgram(program_cost_stage1); program_cost_stage1 = 0; }
            if (program_cost_stage2) { glDeleteProgram(program_cost_stage2); program_cost_stage2 = 0; }

            glFinish(); // Optional, but safer during shutdown.
#if defined(_WIN32)
            if (wglGetCurrentContext() == ctx.hglrc)
                wglMakeCurrent(nullptr, nullptr);
#else
            if (eglGetCurrentContext() == ctx.context)
                eglMakeCurrent(ctx.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
#endif
        }
    }

    bool available() const noexcept { return ok; }
    const std::string& error() const noexcept { return init_error; }

    bool make_current()
    {
#if defined(_WIN32)
        HGLRC current_ctx = wglGetCurrentContext();
        if (current_ctx && current_ctx == ctx.hglrc)
            return true;

        if (ctx.hdc && ctx.hglrc && wglMakeCurrent(ctx.hdc, ctx.hglrc))
            return true;

        DWORD err = GetLastError();
        init_error = "wglMakeCurrent failed, err=" + std::to_string(err);
        return false;
#else
        if (ctx.display == EGL_NO_DISPLAY || ctx.context == EGL_NO_CONTEXT) {
            init_error = "eglMakeCurrent: invalid EGL context";
            return false;
        }
        if (eglGetCurrentContext() == ctx.context)
            return true;
        if (eglMakeCurrent(ctx.display, ctx.surface, ctx.surface, ctx.context))
            return true;
        init_error = "eglMakeCurrent failed, error=" + std::to_string((int)eglGetError());
        return false;
#endif
    }

    bool rebuild(bool force_new)
    {
#if defined(_WIN32)
        if (force_new || !ctx.hglrc || !ctx.owning) {
            ctx = GLWinContext(create_gl_context());
        }
#else
        if (force_new || ctx.context == EGL_NO_CONTEXT || !ctx.owning) {
            ctx = GLLinuxContext(create_gl_context_egl());
        }
#endif
        if (!make_current())
            return false;
#if defined(_WIN32)
        int glad_version = gladLoaderLoadGL();
#else
        int glad_version = gladLoadGL(reinterpret_cast<GLADloadfunc>(eglGetProcAddress));
#endif
        glGetError(); // clear

        if (glad_version == 0) {
            init_error = "failed to load OpenGL functions";
            BOOST_LOG_TRIVIAL(error) << "GpuOrient: GLAD initialization failed: " << init_error;
            return false;
        }

        GLint gl_major = 0, gl_minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &gl_major);
        glGetIntegerv(GL_MINOR_VERSION, &gl_minor);
        if (gl_major < 4 || (gl_major == 4 && gl_minor < 3)) {
            init_error = "OpenGL 4.3+ required for compute shaders";
            BOOST_LOG_TRIVIAL(error) << "GpuOrient: OpenGL version too low: " << gl_major << "." << gl_minor;
            return false;
        }

        if (program_minz_stage1) glDeleteProgram(program_minz_stage1);
        if (program_minz_stage2) glDeleteProgram(program_minz_stage2);
        if (program_maxz_stage1) glDeleteProgram(program_maxz_stage1);
        if (program_maxz_stage2) glDeleteProgram(program_maxz_stage2);
        if (program_cost_stage1) glDeleteProgram(program_cost_stage1);
        if (program_cost_stage2) glDeleteProgram(program_cost_stage2);

        std::string log;
        program_minz_stage1 = compile_compute_program(kMinZStage1ShaderSrc, &log);
        if (!program_minz_stage1) {
            init_error = "MinZ stage1 shader build failed: " + log;
            BOOST_LOG_TRIVIAL(error) << "GpuOrient: " << init_error;
            return false;
        }

        program_minz_stage2 = compile_compute_program(kMinZStage2ShaderSrc, &log);
        if (!program_minz_stage2) {
            init_error = "MinZ stage2 shader build failed: " + log;
            BOOST_LOG_TRIVIAL(error) << "GpuOrient: " << init_error;
            return false;
        }

        program_cost_stage1 = compile_compute_program(kCostStage1ShaderSrc, &log);
        if (!program_cost_stage1) {
            init_error = "Cost stage1 shader build failed: " + log;
            BOOST_LOG_TRIVIAL(error) << "GpuOrient: " << init_error;
            return false;
        }

        program_cost_stage2 = compile_compute_program(kCostStage2ShaderSrc, &log);
        if (!program_cost_stage2) {
            init_error = "Cost stage2 shader build failed: " + log;
            BOOST_LOG_TRIVIAL(error) << "GpuOrient: " << init_error;
            return false;
        }

        program_maxz_stage1 = compile_compute_program(kMaxZStage1ShaderSrc, &log);
        if (!program_maxz_stage1) {
            init_error = "MaxZ stage1 shader build failed: " + log;
            BOOST_LOG_TRIVIAL(error) << "GpuOrient: " << init_error;
            return false;
        }

        program_maxz_stage2 = compile_compute_program(kMaxZStage2ShaderSrc, &log);
        if (!program_maxz_stage2) {
            init_error = "MaxZ stage2 shader build failed: " + log;
            BOOST_LOG_TRIVIAL(error) << "GpuOrient: " << init_error;
            return false;
        }

        return true;
    }

inline bool is_face_appearance(const indexed_triangle_set& its, int face_idx) {
    // Temporary workaround using const_cast.
    return const_cast<indexed_triangle_set&>(its).get_property(face_idx).type
           == EnumFaceTypes::eExteriorAppearance;
}

    // Candidate accumulation (routeC-style, low memory).
    bool build_candidates_routeC(const TriangleMesh& tmesh,
                                 const OrientParams& params,
                                 std::vector<Vec3f>& out_candidates,
                                 double* out_total_plain_area,
                                 double* out_total_overhang_area,
                                 std::vector<VertexGpu>& out_vertices,
                                 std::vector<FaceGpu>& out_faces,
                                 std::vector<HullFaceGpu>& out_hull_faces,
                                 std::string* error)
    {
        using clock = std::chrono::steady_clock;
        const auto t0 = clock::now();
        auto ms = [](clock::time_point a, clock::time_point b) -> long long {
            return std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count();
        };

        if (params.stopcondition && params.stopcondition()) {
            if (error) *error = "Canceled";
            return false;
        }

        const int face_count = tmesh.facets_count();
        const int vcount = int(tmesh.its.vertices.size());
        if (face_count <= 0 || vcount <= 0) {
            if (error) *error = "Empty mesh";
            return false;
        }

        out_vertices.clear();
        out_vertices.reserve(size_t(vcount));
        for (int i = 0; i < vcount; ++i) {
            if (params.stopcondition && (i & ((1 << 20) - 1)) == 0 && params.stopcondition()) {
                if (error) *error = "Canceled";
                return false;
            }
            const Vec3f& v = tmesh.its.vertices[size_t(i)];
            out_vertices.push_back({v.x(), v.y(), v.z(), 0.f});
        }
        const auto t_vertices_done = clock::now();

        // Limit candidate accumulation work for huge meshes. We still build face buffers for all faces.
        constexpr int kMaxCandidateSampleFaces = 2'000'000;
        int sample_stride = 1;
        if (face_count > kMaxCandidateSampleFaces)
            sample_stride = std::max(1, (face_count + kMaxCandidateSampleFaces - 1) / kMaxCandidateSampleFaces);

        // Candidate accumulation bins: octahedral map (256x256 => 65536 bins).
        // This avoids huge unordered_map overhead on curved meshes (millions of unique normals).
        constexpr int kOctRes = 256;
        constexpr uint32_t kOctBins = uint32_t(kOctRes * kOctRes);
        struct BinAccum {
            float sum_area = 0.f;
            float max_area = 0.f;
            float nx = 0.f, ny = 0.f, nz = 0.f; // representative normal (largest triangle in bin)
        };
        auto oct_bin = [](const Vec3f& n) -> uint32_t {
            constexpr int kOctResLocal = 256;

            const float x = n.x(), y = n.y(), z = n.z();
            const float ax = std::abs(x), ay = std::abs(y), az = std::abs(z);
            const float denom = ax + ay + az;
            if (denom <= 0.f) return 0u;

            float ox = x / denom;
            float oy = y / denom;

            if (z < 0.f) {
                const float sx = (ox >= 0.f) ? 1.f : -1.f;
                const float sy = (oy >= 0.f) ? 1.f : -1.f;
                const float rx = (1.f - std::abs(oy)) * sx;
                const float ry = (1.f - std::abs(ox)) * sy;
                ox = rx;
                oy = ry;
            }

            int u = int((ox * 0.5f + 0.5f) * float(kOctResLocal - 1) + 0.5f);
            int v = int((oy * 0.5f + 0.5f) * float(kOctResLocal - 1) + 0.5f);
            u = std::max(0, std::min(kOctResLocal - 1, u));
            v = std::max(0, std::min(kOctResLocal - 1, v));
            return uint32_t(v * kOctResLocal + u);
        };

        // Appearance flags (serial) to avoid calling get_property() concurrently.
        const auto t_app_start = clock::now();
        const bool need_app = (params.APPERANCE_FACE_SUPP != 0.f);
        std::vector<uint8_t> is_app;
        is_app.resize(size_t(face_count), 0u);
        if (need_app) {
            for (int fi = 0; fi < face_count; ++fi) {
                if (params.stopcondition && (fi & ((1 << 18) - 1)) == 0 && params.stopcondition()) {
                    if (error) *error = "Canceled";
                    return false;
                }
                is_app[size_t(fi)] = is_face_appearance(tmesh.its, fi) ? 1u : 0u;
            }
        }
        const auto t_app_done = clock::now();

        out_faces.clear();
        out_faces.resize(size_t(face_count));

        const auto& vs  = tmesh.its.vertices;
        const auto& idx = tmesh.its.indices;

        double total_plain_area = 0.0;
        double total_overhang_area = 0.0;

        // Build face buffer (all faces) + candidate bins (sampled) in parallel when possible.
        unsigned hw = std::thread::hardware_concurrency();
        if (hw == 0) hw = 4;
        constexpr unsigned kMaxThreads = 12;
        const unsigned num_threads =
            (params.parallel && face_count >= 500'000) ? std::min(hw, kMaxThreads) : 1u;

        std::atomic<bool> canceled{false};
        std::vector<std::vector<BinAccum>> bins_tls;
        bins_tls.resize(num_threads);
        for (unsigned t = 0; t < num_threads; ++t)
            bins_tls[t].assign(kOctBins, BinAccum{});
        std::vector<double> plain_tls(num_threads, 0.0);
        std::vector<double> over_tls(num_threads, 0.0);

        auto worker = [&](unsigned tid, int begin, int end) {
            auto& bins = bins_tls[tid];
            double plain = 0.0;
            double over  = 0.0;

            for (int fi = begin; fi < end; ++fi) {
                if ((fi & ((1 << 17) - 1)) == 0) {
                    if (canceled.load(std::memory_order_relaxed))
                        break;
                    if (params.stopcondition && params.stopcondition()) {
                        canceled.store(true, std::memory_order_relaxed);
                        break;
                    }
                }

                const auto& tri = idx[size_t(fi)];
                const Vec3f& p0 = vs[size_t(tri[0])];
                const Vec3f& p1 = vs[size_t(tri[1])];
                const Vec3f& p2 = vs[size_t(tri[2])];

                const Vec3f c = (p1 - p0).cross(p2 - p0);
                const float len = c.norm(); // 2 * area
                const float area_plain = 0.5f * len;
                const Vec3f n = (len > 0.f) ? (c / len) : Vec3f(0, 0, 0);

                const bool is_appearance = need_app && (is_app[size_t(fi)] != 0u);
                const float appearance_weight = 1.0f + (is_appearance ? params.APPERANCE_FACE_SUPP : 0.0f);
                const float area_overhang = area_plain * appearance_weight;

                plain += area_plain;
                over  += area_overhang;

                FaceGpu& fg = out_faces[size_t(fi)];
                fg.nx = n.x(); fg.ny = n.y(); fg.nz = n.z();
                fg.area_plain = area_plain;
                fg.area_overhang = area_overhang;
                fg.i0 = uint32_t(tri[0]);
                fg.i1 = uint32_t(tri[1]);
                fg.i2 = uint32_t(tri[2]);
                fg.pad0 = 0.f; fg.pad1 = 0.f;

                // candidates (sampled)
                const bool sample = (sample_stride == 1) || ((uint32_t(fi) * 2654435761u) % uint32_t(sample_stride) == 0u);
                if (sample && area_plain > 0.f) {
                    const uint32_t bi = oct_bin(n);
                    BinAccum& b = bins[bi];
                    b.sum_area += area_plain;
                    if (area_plain > b.max_area) {
                        b.max_area = area_plain;
                        b.nx = n.x(); b.ny = n.y(); b.nz = n.z();
                    }
                }
            }

            plain_tls[tid] = plain;
            over_tls[tid]  = over;
        };

        const auto t_face_start = clock::now();
        if (num_threads == 1u) {
            worker(0, 0, face_count);
        } else {
            std::vector<std::thread> threads;
            threads.reserve(num_threads);
            const int chunk = face_count / int(num_threads);
            for (unsigned tid = 0; tid < num_threads; ++tid) {
                const int begin = int(tid) * chunk;
                const int end = (tid + 1u == num_threads) ? face_count : (begin + chunk);
                threads.emplace_back(worker, tid, begin, end);
            }
            for (auto& th : threads)
                th.join();
        }
        const auto t_faces_done = clock::now();

        if (canceled.load(std::memory_order_relaxed)) {
            if (error) *error = "Canceled";
            return false;
        }

        // reduce totals
        for (unsigned tid = 0; tid < num_threads; ++tid) {
            total_plain_area += plain_tls[tid];
            total_overhang_area += over_tls[tid];
        }

        // reduce bins
        std::vector<BinAccum> bins(kOctBins);
        size_t cand_bins = 0;
        for (uint32_t bi = 0; bi < kOctBins; ++bi) {
            BinAccum out{};
            for (unsigned tid = 0; tid < num_threads; ++tid) {
                const BinAccum& in = bins_tls[tid][bi];
                out.sum_area += in.sum_area;
                if (in.max_area > out.max_area) {
                    out.max_area = in.max_area;
                    out.nx = in.nx; out.ny = in.ny; out.nz = in.nz;
                }
            }
            if (out.sum_area > 0.f)
                ++cand_bins;
            bins[bi] = out;
        }

        if (out_total_plain_area) *out_total_plain_area = total_plain_area;
        if (out_total_overhang_area) *out_total_overhang_area = total_overhang_area;

        out_candidates.clear();
        out_candidates.reserve(64);
        out_candidates.push_back({0,0,-1}); // keep CPU behavior

        // top 10
        {
            std::vector<std::pair<float, uint32_t>> vec;
            vec.reserve(cand_bins);
            for (uint32_t bi = 0; bi < kOctBins; ++bi) {
                if (bins[bi].sum_area > 0.f && bins[bi].max_area > 0.f)
                    vec.emplace_back(bins[bi].sum_area, bi);
            }
            const int kN = std::min<int>(10, int(vec.size()));
            if (kN > 0) {
                auto cmp = [](const auto& a, const auto& b){ return a.first > b.first; };
                if (vec.size() > size_t(kN))
                    std::nth_element(vec.begin(), vec.begin() + kN, vec.end(), cmp);
                std::sort(vec.begin(), vec.begin() + kN, cmp);
                vec.resize(size_t(kN));
                for (int i = 0; i < kN; ++i) {
                    const BinAccum& b = bins[vec[size_t(i)].second];
                    out_candidates.push_back({b.nx, b.ny, b.nz});
                }
            }
        }

        // convex hull (faces small)
        out_hull_faces.clear();
        clock::time_point t_hull_build_done = t_faces_done;
        clock::time_point t_hull_done = t_faces_done;
        {
            if (params.stopcondition && params.stopcondition()) {
                if (error) *error = "Canceled";
                return false;
            }
            TriangleMesh hull = tmesh.convex_hull_3d();
            t_hull_build_done = clock::now();
            t_hull_done = t_hull_build_done;
            const int hf = hull.facets_count();
            if (hf > 0) {
                std::vector<BinAccum> hb(kOctBins);

                const auto& hvs  = hull.its.vertices;
                const auto& hidx = hull.its.indices;

                for (int i = 0; i < hf; ++i) {
                    if (params.stopcondition && (i & ((1 << 16) - 1)) == 0 && params.stopcondition()) {
                        if (error) *error = "Canceled";
                        return false;
                    }

                    const auto& tri = hidx[size_t(i)];
                    const Vec3f& p0 = hvs[size_t(tri[0])];
                    const Vec3f& p1 = hvs[size_t(tri[1])];
                    const Vec3f& p2 = hvs[size_t(tri[2])];

                    const Vec3f c = (p1 - p0).cross(p2 - p0);
                    const float len = c.norm(); // 2 * area
                    Vec3f n = (len > 0.f) ? (c / len) : Vec3f(0, 0, 0);

                    const float area_plain = 0.5f * len;
                    if (area_plain > 0.f) {
                        const uint32_t bi = oct_bin(n);
                        BinAccum& b = hb[bi];
                        b.sum_area += area_plain;
                        if (area_plain > b.max_area) {
                            b.max_area = area_plain;
                            b.nx = n.x(); b.ny = n.y(); b.nz = n.z();
                        }
                    }

                    HullFaceGpu hfg;
                    hfg.area_plain = area_plain;
                    hfg.i0 = uint32_t(tri[0]);
                    hfg.i1 = uint32_t(tri[1]);
                    hfg.i2 = uint32_t(tri[2]);
                    hfg.pad0 = 0;
                    out_hull_faces.push_back(hfg);
                }
                t_hull_done = clock::now();

                std::vector<std::pair<float, uint32_t>> vec;
                vec.reserve(size_t(hf));
                for (uint32_t bi = 0; bi < kOctBins; ++bi) {
                    if (hb[bi].sum_area > 0.f && hb[bi].max_area > 0.f)
                        vec.emplace_back(hb[bi].sum_area, bi);
                }
                const int kN = std::min<int>(14, int(vec.size()));
                if (kN > 0) {
                    auto cmp = [](const auto& a, const auto& b){ return a.first > b.first; };
                    if (vec.size() > size_t(kN))
                        std::nth_element(vec.begin(), vec.begin() + kN, vec.end(), cmp);
                    std::sort(vec.begin(), vec.begin() + kN, cmp);
                    vec.resize(size_t(kN));
                    for (int i = 0; i < kN; ++i) {
                        const BinAccum& b = hb[vec[size_t(i)].second];
                        out_candidates.push_back({b.nx, b.ny, b.nz});
                    }
                }
            }
        }

        // Supplement candidates differ by mode (mirrors CPU AutoOrienter behaviour).
        if (params.orient_type == EOrientType::MinArea) {
            add_supplements(out_candidates);
        } else if (params.orient_type == EOrientType::MinVolume) {
            // CPU adds global-up as an explicit extra candidate for MinVolume.
            out_candidates.push_back({0.f, 0.f, 1.f});
        }
        // MinTime: no extra candidates - top-10 + top-14 hull directions only.
        remove_duplicates(out_candidates);
        const auto t_end = clock::now();

        const auto elapsed_ms = ms(t0, t_end);
        if (elapsed_ms > 2000 || face_count >= 2'000'000) {
            BOOST_LOG_TRIVIAL(warning) << "GpuOrient: build_candidates_routeC done"
                                    << ", faces=" << face_count
                                    << ", verts=" << vcount
                                    << ", sample_stride=" << sample_stride
                                    << ", threads=" << num_threads
                                    << ", candidates=" << out_candidates.size()
                                    << ", cand_bins=" << cand_bins
                                    << ", hull_faces=" << out_hull_faces.size()
                                    << ", verts_ms=" << ms(t0, t_vertices_done)
                                    << ", app_ms=" << ms(t_app_start, t_app_done)
                                    << ", faces_ms=" << ms(t_face_start, t_faces_done)
                                    << ", hull_build_ms=" << ms(t_faces_done, t_hull_build_done)
                                    << ", hull_ms=" << ms(t_hull_build_done, t_hull_done)
                                    << ", elapsed_ms=" << elapsed_ms;
        }
        return true;
    }

    bool dispatch_minz_and_cost(const std::vector<VertexGpu>& vertices,
                                const std::vector<FaceGpu>& faces,
                                const std::vector<HullFaceGpu>& hfaces,
                                const std::vector<OrientationGpu>& orients,
                                float ascent,
                                float first_lay_h,
                                float area_scale,
                                float laf_min,
                                float laf_max,
                                EOrientType orient_type,
                                const std::function<bool(void)>& stopcond,
                                std::vector<int32_t>& out_minz_ord,
                                std::vector<int32_t>& out_maxz_ord,
                                std::vector<uint32_t>& out_r0,
                                std::vector<uint32_t>& out_r1,
                                std::vector<float>& out_sv,
                                std::vector<uint32_t>& out_r2,
                                std::string* error)
    {
        if (vertices.empty() || faces.empty() || orients.empty()) {
            if (error) *error = "Empty buffers";
            return false;
        }

        // Declared before any goto so C++ jump-over-init rules are satisfied.
        const bool need_maxz = (orient_type != EOrientType::MinArea);

        auto wait_fence = [&](const char* label, GLuint group_base, GLuint group_count) -> bool {
            GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            glFlush();
            if (!fence)
                return true;

            using clock = std::chrono::steady_clock;
            const auto start = clock::now();
            bool logged = false;
            while (true) {
                const GLenum r = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 5000000); // 5ms
                if (r == GL_ALREADY_SIGNALED || r == GL_CONDITION_SATISFIED) break;
                if (r == GL_WAIT_FAILED) {
                    if (error) {
                        std::ostringstream oss;
                        oss << "glClientWaitSync failed (" << label << ")";
                        *error = oss.str();
                    }
                    glDeleteSync(fence);
                    return false;
                }
                if (stopcond && stopcond()) {
                    if (error) *error = "Canceled";
                    glDeleteSync(fence);
                    return false;
                }
                if (!logged) {
                    const auto elapsed_ms =
                        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start).count();
                    if (elapsed_ms > 2000) {
                        logged = true;
                        BOOST_LOG_TRIVIAL(info) << "GpuOrient: waiting GPU completion (" << label << ")"
                                                << ", faces=" << faces.size()
                                                << ", orients=" << orients.size()
                                                << ", group_base=" << group_base
                                                << ", group_count=" << group_count
                                                << ", elapsed_ms=" << elapsed_ms;
                    }
                }
            }
            glDeleteSync(fence);
            return true;
        };

        // SSBOs
        GLuint v_ssbo = 0, f_ssbo = 0, hf_ssbo = 0, o_ssbo = 0;
        GLuint partial_minz_ssbo = 0, minz_ssbo = 0;
        GLuint partial_maxz_ssbo = 0, maxz_ssbo = 0;
        GLuint partial0_ssbo = 0, partial1_ssbo = 0;
        GLuint partial_sv_ssbo = 0, partial2_ssbo = 0;
        GLuint r0_ssbo = 0, r1_ssbo = 0, sv_ssbo = 0, r2_ssbo = 0;

        auto cleanup_buffers = [&]() {
            glDeleteBuffers(1, &v_ssbo);
            glDeleteBuffers(1, &f_ssbo);
            glDeleteBuffers(1, &hf_ssbo);
            glDeleteBuffers(1, &o_ssbo);
            glDeleteBuffers(1, &partial_minz_ssbo);
            glDeleteBuffers(1, &minz_ssbo);
            glDeleteBuffers(1, &partial_maxz_ssbo);
            glDeleteBuffers(1, &maxz_ssbo);
            glDeleteBuffers(1, &partial0_ssbo);
            glDeleteBuffers(1, &partial1_ssbo);
            glDeleteBuffers(1, &partial_sv_ssbo);
            glDeleteBuffers(1, &partial2_ssbo);
            glDeleteBuffers(1, &r0_ssbo);
            glDeleteBuffers(1, &r1_ssbo);
            glDeleteBuffers(1, &sv_ssbo);
            glDeleteBuffers(1, &r2_ssbo);
        };

        glGenBuffers(1, &v_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, v_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, vertices.size() * sizeof(VertexGpu), vertices.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &f_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, f_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, faces.size() * sizeof(FaceGpu), faces.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &hf_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, hf_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, hfaces.size() * sizeof(HullFaceGpu), hfaces.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &o_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, o_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, orients.size() * sizeof(OrientationGpu), orients.data(), GL_STATIC_DRAW);

        const GLuint orient_count = (GLuint)orients.size();
        constexpr GLuint kTargetTotalWorkgroups = 32768u;
        const auto pick_chunk_groups_x = [](GLuint total_groups_x, GLuint orient_count, GLuint target_total_wg) -> GLuint {
            if (total_groups_x == 0u) return 0u;
            if (orient_count == 0u) return total_groups_x;
            GLuint chunk = target_total_wg / orient_count;
            if (chunk < 1u) chunk = 1u;
            if (chunk > total_groups_x) chunk = total_groups_x;
            return chunk;
        };

        // ---------- minz stage1+stage2 ----------
        constexpr GLuint kMinZLocal = 256;
        constexpr GLuint kMinZVertsPerThread = 4;
        const size_t minz_block = size_t(kMinZLocal) * size_t(kMinZVertsPerThread);
        const GLuint minz_groups_x = (GLuint)((vertices.size() + minz_block - 1) / minz_block);
        const GLuint minz_chunk_groups_x = pick_chunk_groups_x(minz_groups_x, orient_count, kTargetTotalWorkgroups);

        // partial minz
        glGenBuffers(1, &partial_minz_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, partial_minz_ssbo);
        const size_t partial_minz_count = size_t(orient_count) * size_t(minz_chunk_groups_x);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     (GLsizeiptr)(partial_minz_count * sizeof(float)),
                     nullptr,
                     GL_DYNAMIC_DRAW);

        // final minz (ordered int)
        glGenBuffers(1, &minz_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, minz_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     (GLsizeiptr)(orient_count * sizeof(int32_t)),
                     nullptr,
                     GL_DYNAMIC_DRAW);
        {
            const int32_t init = 0x7f7fffff; // floatBitsToInt(FLT_MAX)
            glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, &init);
        }

        glUseProgram(program_minz_stage1);
        const GLint loc_minz_vertex_count = glGetUniformLocation(program_minz_stage1, "vertex_count");
        const GLint loc_minz_partial_count = glGetUniformLocation(program_minz_stage1, "partial_count");
        const GLint loc_minz_group_base = glGetUniformLocation(program_minz_stage1, "group_base");
        glUniform1ui(loc_minz_vertex_count, (GLuint)vertices.size());

        glUseProgram(program_minz_stage2);
        const GLint loc_minz2_partial_count = glGetUniformLocation(program_minz_stage2, "partial_count");

        for (GLuint base = 0; base < minz_groups_x; base += minz_chunk_groups_x) {
            if (stopcond && stopcond()) {
                if (error) *error = "Canceled";
                cleanup_buffers();
                return false;
            }
            const GLuint chunk = std::min(minz_chunk_groups_x, minz_groups_x - base);

            glUseProgram(program_minz_stage1);
            // Stage1 bindings (stage2 uses different bindings, so re-bind per chunk)
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, v_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, o_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, partial_minz_ssbo);
            glUniform1ui(loc_minz_partial_count, chunk);
            glUniform1ui(loc_minz_group_base, base);
            glDispatchCompute(chunk, orient_count, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

            glUseProgram(program_minz_stage2);
            // Stage2 bindings
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, partial_minz_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, minz_ssbo);
            glUniform1ui(loc_minz2_partial_count, chunk);
            glDispatchCompute(1, orient_count, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

            if (!wait_fence("minz", base, chunk)) {
                cleanup_buffers();
                return false;
            }
        }

        // ---------- maxz stage1+stage2 (MinVolume/MinTime: mesh_height = maxz - minz) ----------
        if (need_maxz) {
            // Re-use the same workgroup layout as MinZ (vertex-based).
            glGenBuffers(1, &partial_maxz_ssbo);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, partial_maxz_ssbo);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                         (GLsizeiptr)(partial_minz_count * sizeof(float)),
                         nullptr,
                         GL_DYNAMIC_DRAW);

            glGenBuffers(1, &maxz_ssbo);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, maxz_ssbo);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                         (GLsizeiptr)(orient_count * sizeof(int32_t)),
                         nullptr,
                         GL_DYNAMIC_DRAW);
            {
                const int32_t init = (int32_t)0x80000000; // INT_MIN - lower than any floatToOrderedInt
                glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, &init);
            }

            glUseProgram(program_maxz_stage1);
            const GLint loc_maxz_vertex_count  = glGetUniformLocation(program_maxz_stage1, "vertex_count");
            const GLint loc_maxz_partial_count = glGetUniformLocation(program_maxz_stage1, "partial_count");
            const GLint loc_maxz_group_base    = glGetUniformLocation(program_maxz_stage1, "group_base");
            glUniform1ui(loc_maxz_vertex_count, (GLuint)vertices.size());

            glUseProgram(program_maxz_stage2);
            const GLint loc_maxz2_partial_count = glGetUniformLocation(program_maxz_stage2, "partial_count");

            for (GLuint base = 0; base < minz_groups_x; base += minz_chunk_groups_x) {
                if (stopcond && stopcond()) {
                    if (error) *error = "Canceled";
                    cleanup_buffers();
                    return false;
                }
                const GLuint chunk = std::min(minz_chunk_groups_x, minz_groups_x - base);

                glUseProgram(program_maxz_stage1);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, v_ssbo);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, o_ssbo);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, partial_maxz_ssbo);
                glUniform1ui(loc_maxz_partial_count, chunk);
                glUniform1ui(loc_maxz_group_base, base);
                glDispatchCompute(chunk, orient_count, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

                glUseProgram(program_maxz_stage2);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, partial_maxz_ssbo);
                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, maxz_ssbo);
                glUniform1ui(loc_maxz2_partial_count, chunk);
                glDispatchCompute(1, orient_count, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

                if (!wait_fence("maxz", base, chunk)) {
                    cleanup_buffers();
                    return false;
                }
            }
        }

        // ---------- cost stage1+stage2 ----------
        constexpr GLuint kCostLocal = 256;
        constexpr GLuint kFacesPerThread = 4;
        const size_t cost_block = size_t(kCostLocal) * size_t(kFacesPerThread);
        const size_t max_items = std::max(faces.size(), hfaces.size());
        const GLuint cost_groups_x = (GLuint)((max_items + cost_block - 1) / cost_block);
        const GLuint cost_chunk_groups_x = pick_chunk_groups_x(cost_groups_x, orient_count, kTargetTotalWorkgroups);

        const size_t partial_cost_count = size_t(orient_count) * size_t(cost_chunk_groups_x);

        // partial sums
        glGenBuffers(1, &partial0_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, partial0_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     (GLsizeiptr)(partial_cost_count * 4 * sizeof(uint32_t)),
                     nullptr,
                     GL_DYNAMIC_DRAW);

        glGenBuffers(1, &partial1_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, partial1_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     (GLsizeiptr)(partial_cost_count * sizeof(uint32_t)),
                     nullptr,
                     GL_DYNAMIC_DRAW);

        // final results
        glGenBuffers(1, &r0_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, r0_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     (GLsizeiptr)(orient_count * 4 * sizeof(uint32_t)),
                     nullptr,
                     GL_DYNAMIC_DRAW);
        {
            const uint32_t zero[4] = {0u, 0u, 0u, 0u};
            glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, zero);
        }

        glGenBuffers(1, &r1_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, r1_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     (GLsizeiptr)(orient_count * 4 * sizeof(uint32_t)),
                     nullptr,
                     GL_DYNAMIC_DRAW);
        {
            const uint32_t zero[4] = {0u, 0u, 0u, 0u};
            glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, zero);
        }

        // sv_ssbo: float support_vol per orientation (MinVolume + MinTime)
        glGenBuffers(1, &sv_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sv_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     (GLsizeiptr)(orient_count * sizeof(float)),
                     nullptr,
                     GL_DYNAMIC_DRAW);
        {
            const float fzero = 0.0f;
            glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, &fzero);
        }

        // r2_ssbo: uvec4[surf_arc, ovhg_arc, fill, top] per orientation (MinTime)
        glGenBuffers(1, &r2_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, r2_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     (GLsizeiptr)(orient_count * 4 * sizeof(uint32_t)),
                     nullptr,
                     GL_DYNAMIC_DRAW);
        {
            const uint32_t zero[4] = {0u, 0u, 0u, 0u};
            glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, zero);
        }

        // partial_sv and partial2 buffers for Cost Stage1
        glGenBuffers(1, &partial_sv_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, partial_sv_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     (GLsizeiptr)(partial_cost_count * sizeof(float)),
                     nullptr,
                     GL_DYNAMIC_DRAW);

        glGenBuffers(1, &partial2_ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, partial2_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     (GLsizeiptr)(partial_cost_count * 4 * sizeof(uint32_t)),
                     nullptr,
                     GL_DYNAMIC_DRAW);

        glUseProgram(program_cost_stage1);
        const GLint loc_cost_partial_count = glGetUniformLocation(program_cost_stage1, "partial_count");
        const GLint loc_cost_group_base = glGetUniformLocation(program_cost_stage1, "group_base");
        glUniform1ui(glGetUniformLocation(program_cost_stage1, "face_count"), (GLuint)faces.size());
        glUniform1ui(glGetUniformLocation(program_cost_stage1, "hull_face_count"), (GLuint)hfaces.size());
        glUniform1f(glGetUniformLocation(program_cost_stage1, "ascent"), ascent);
        glUniform1f(glGetUniformLocation(program_cost_stage1, "first_lay_h"), first_lay_h);
        glUniform1f(glGetUniformLocation(program_cost_stage1, "area_scale"), area_scale);
        glUniform1f(glGetUniformLocation(program_cost_stage1, "laf_min_cos"), laf_min);
        glUniform1f(glGetUniformLocation(program_cost_stage1, "laf_max_cos"), laf_max);
        glUniform1f(glGetUniformLocation(program_cost_stage1, "eps_z"), kCpuEps);
        glUniform1ui(glGetUniformLocation(program_cost_stage1, "orient_mode"), (GLuint)orient_type);

        // stage2
        glUseProgram(program_cost_stage2);
        const GLint loc_cost2_partial_count = glGetUniformLocation(program_cost_stage2, "partial_count");

        for (GLuint base = 0; base < cost_groups_x; base += cost_chunk_groups_x) {
            if (stopcond && stopcond()) {
                if (error) *error = "Canceled";
                cleanup_buffers();
                return false;
            }
            const GLuint chunk = std::min(cost_chunk_groups_x, cost_groups_x - base);

            glUseProgram(program_cost_stage1);
            // Stage1 bindings (stage2 uses different bindings, so re-bind per chunk)
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, v_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, f_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, hf_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, o_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, minz_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, partial0_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, partial1_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, partial_sv_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, partial2_ssbo);
            glUniform1ui(loc_cost_partial_count, chunk);
            glUniform1ui(loc_cost_group_base, base);
            glDispatchCompute(chunk, orient_count, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

            glUseProgram(program_cost_stage2);
            // Stage2 bindings
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, partial0_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, partial1_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, r0_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, r1_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, partial_sv_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, partial2_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, sv_ssbo);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, r2_ssbo);
            glUniform1ui(loc_cost2_partial_count, chunk);
            glDispatchCompute(1, orient_count, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

            if (!wait_fence("cost", base, chunk)) {
                cleanup_buffers();
                return false;
            }
        }

        // ---------- readback ----------
        out_minz_ord.resize(orient_count);
        out_r0.resize(size_t(orient_count) * 4);
        out_r1.resize(size_t(orient_count) * 4);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, minz_ssbo);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                           (GLsizeiptr)(orient_count * sizeof(int32_t)),
                           out_minz_ord.data());

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, r0_ssbo);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                           (GLsizeiptr)(orient_count * 4 * sizeof(uint32_t)),
                           out_r0.data());

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, r1_ssbo);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                           (GLsizeiptr)(orient_count * 4 * sizeof(uint32_t)),
                           out_r1.data());

        out_maxz_ord.clear();
        if (need_maxz && maxz_ssbo) {
            out_maxz_ord.resize(orient_count);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, maxz_ssbo);
            glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                               (GLsizeiptr)(orient_count * sizeof(int32_t)),
                               out_maxz_ord.data());
        }

        out_sv.resize(orient_count);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, sv_ssbo);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                           (GLsizeiptr)(orient_count * sizeof(float)),
                           out_sv.data());

        out_r2.resize(size_t(orient_count) * 4);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, r2_ssbo);
        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                           (GLsizeiptr)(orient_count * 4 * sizeof(uint32_t)),
                           out_r2.data());

        // cleanup success
        cleanup_buffers();
        return true;
    }

    bool orient_one_mesh(OrientMesh& mesh, const OrientParams& params, std::string* error)
    {
        TriangleMesh& tmesh = mesh.mesh;
        const int face_count = tmesh.facets_count();
        if (face_count <= 0) {
            if (error) *error = "Mesh has no faces";
            return false;
        }

        // Build candidates + packed buffers
        std::vector<Vec3f> candidates;
        std::vector<VertexGpu> vertices;
        std::vector<FaceGpu> faces;
        std::vector<HullFaceGpu> hfaces;
        double total_plain_area = 0.0, total_overhang_area = 0.0;

        if (!build_candidates_routeC(tmesh, params, candidates,
                                     &total_plain_area, &total_overhang_area,
                                     vertices, faces, hfaces,
                                     error)) {
            return false;
        }

        if (candidates.empty()) {
            if (error) *error = "No candidate orientations";
            return false;
        }

        // Orientations buffer stores "up" = -candidate (matches CPU process: orientation = -orientations[i])
        std::vector<OrientationGpu> orients;
        orients.reserve(candidates.size());
        for (const Vec3f& c : candidates) {
            Vec3f up = (-c).normalized();
            orients.push_back({up.x(), up.y(), up.z(), 0.f});
        }

        // Scale (fixed-point) to avoid uint overflow
        const double max_sum_area = std::max(total_plain_area, total_overhang_area);
        const double max_u32 = double(std::numeric_limits<uint32_t>::max()) - 1024.0;
        float area_scale = 1.0f;
        if (max_sum_area > 0.0) {
            const double max_scale = max_u32 / max_sum_area;
            area_scale = (float)std::min(10000.0, std::max(1.0, std::floor(max_scale)));
        }

        const float ascent = compute_ascent((float)mesh.overhang_angle);
        const float first_lay_h = params.FIRST_LAY_H;

        // Dispatch GPU full exact
        std::vector<int32_t> minz_ord, maxz_ord;
        std::vector<uint32_t> r0, r1, r2;
        std::vector<float> sv;
        std::string tmp;
        if (!dispatch_minz_and_cost(vertices, faces, hfaces, orients,
                                    ascent, first_lay_h, area_scale,
                                    params.LAF_MIN, params.LAF_MAX,
                                    params.orient_type,
                                    params.stopcondition,
                                    minz_ord, maxz_ord, r0, r1, sv, r2,
                                    error ? error : &tmp)) {
            return false;
        }

        struct Pick {
            float cost;
            size_t idx;
        };
        std::vector<Pick> picks;
        picks.reserve(orients.size());

        for (size_t i = 0; i < orients.size(); ++i) {
            float cost = std::numeric_limits<float>::infinity();

            if (params.orient_type == EOrientType::MinArea) {
                // --- MinArea cost (same as original GPU formula) ---
                const float overhang    = r0[i*4 + 0] / area_scale;
                const float bottom1     = r0[i*4 + 1] / area_scale;
                const float bottom2     = r0[i*4 + 2] / area_scale;
                const float laf         = r0[i*4 + 3] / area_scale;
                const float bottom_hull = r1[i*4 + 0] / area_scale;

                const float bottom  = 0.5f * bottom1 + bottom2;
                const float contour = 4.0f * std::sqrt(std::max(bottom, 0.0f));

                const float denom =
                    params.TAR_D
                    + params.CONTOUR_F * contour
                    + params.BOTTOM_F * bottom
                    + params.BOTTOM_HULL_F * bottom_hull
                    + params.TAR_PROJ_AREA * 0.0f;

                if (denom > 0.0f) {
                    cost = params.RELATIVE_F
                         * (overhang * params.TAR_C
                            + params.TAR_D
                            + params.TAR_LAF * laf * (params.use_low_angle_face ? 1.0f : 0.0f))
                         / denom;
                }
                if (bottom < params.BOTTOM_MIN)
                    cost += 100.0f;

            } else if (params.orient_type == EOrientType::MinVolume) {
                // --- MinVolume cost: support volume directly (mirrors CPU target_function) ---
                cost = sv[i];

            } else {
                // --- MinTime cost: polynomial formula (mirrors CPU getCostTime) ---
                const float height = 0.2f; // layer height used in CPU getCostTime

                float mesh_height = orderedIntToFloat(maxz_ord[i]) - orderedIntToFloat(minz_ord[i]);
                float layer_num   = std::max(1.0f, mesh_height / height);

                float surf_arc_f  = r2[i*4 + 0] / area_scale;
                float ovhg_arc_f  = r2[i*4 + 1] / area_scale;
                float fill_area_f = r2[i*4 + 2] / area_scale;
                float top_area_f  = r2[i*4 + 3] / area_scale;

                float wall_time = (surf_arc_f + ovhg_arc_f) / (height * 200.0f);
                float fill_time = fill_area_f / (height * 250.0f);
                float top_time  = top_area_f  / (0.42f * 200.0f);
                float vol       = sv[i];

                // Polynomial coefficients from CPU getCostTime()
                float fill_quan = 0.0002f * fill_time * fill_time + 0.8141f * fill_time + 4.9651f;
                float wall_quan = -2e-05f * wall_time * wall_time + 0.6938f * wall_time - 12.877f;
                float top_quan  = 6e-05f  * top_time  * top_time  + 0.6852f * top_time  + 0.7016f;
                float sup_quan  = 5e-09f  * vol * vol             + 0.0049f * vol       + 26.955f;

                float layer_quan = 0.0f;
                if (wall_quan + fill_quan + top_quan < layer_num * 1.0f)
                    layer_quan = (layer_num - (wall_quan + fill_quan + top_quan)) / layer_num;

                cost = wall_quan + fill_quan + top_quan + layer_num * layer_quan + sup_quan;
            }

            picks.push_back({cost, i});
        }

        std::sort(picks.begin(), picks.end(), [](const Pick& a, const Pick& b) {
            if (a.cost != b.cost) return a.cost < b.cost;
            return a.idx < b.idx;
        });

        if (picks.empty()) {
            if (error) *error = "No picks";
            return false;
        }

        size_t best_idx = picks.front().idx;
        const float best_cost = picks.front().cost;

        // Tie-break to avoid flipping: mimic CPU behavior (prefer global up if costs tie).
        const Vec3f global_up{0, 0, 1};
        const Vec3f best_up0(orients[best_idx].x, orients[best_idx].y, orients[best_idx].z);
        if (std::abs(best_up0.dot(global_up) - 1.0f) > kCpuEps) {
            for (size_t t = 1; t < picks.size(); ++t) {
                if (std::abs(picks[t].cost - best_cost) > kCpuEps) break;
                Vec3f up(orients[picks[t].idx].x, orients[picks[t].idx].y, orients[picks[t].idx].z);
                if (std::abs(up.dot(global_up) - 1.0f) < (kCpuEps * kCpuEps)) {
                    best_idx = picks[t].idx;
                    break;
                }
            }
        }

        const Vec3f best_up(orients[best_idx].x, orients[best_idx].y, orients[best_idx].z);
        const Vec3f best_down = -best_up;

        mesh.orientation = best_down.cast<double>();
        Geometry::rotation_from_two_vectors(mesh.orientation, {0,0,-1}, mesh.axis, mesh.angle, &mesh.rotation_matrix);
        mesh.euler_angles = Geometry::extract_euler_angles(mesh.rotation_matrix);

        // debug log (optional)
        {
            BOOST_LOG_TRIVIAL(warning) << "GpuOrient picked idx=" << best_idx
                << " mode=" << (int)params.orient_type
                << " up=(" << best_up.x() << "," << best_up.y() << "," << best_up.z() << ")"
                << " down=(" << best_down.x() << "," << best_down.y() << "," << best_down.z() << ")"
                << " cost=" << best_cost;
            if (params.orient_type == EOrientType::MinArea) {
                BOOST_LOG_TRIVIAL(warning) << "  MinArea: over=" << r0[best_idx*4+0]/area_scale
                    << " b1=" << r0[best_idx*4+1]/area_scale
                    << " b2=" << r0[best_idx*4+2]/area_scale
                    << " bh=" << r1[best_idx*4+0]/area_scale
                    << " laf=" << r0[best_idx*4+3]/area_scale;
            } else if (params.orient_type == EOrientType::MinVolume) {
                BOOST_LOG_TRIVIAL(warning) << "  MinVolume: sv=" << sv[best_idx];
            } else {
                float mh = orderedIntToFloat(maxz_ord[best_idx]) - orderedIntToFloat(minz_ord[best_idx]);
                BOOST_LOG_TRIVIAL(warning) << "  MinTime: sv=" << sv[best_idx]
                    << " surf_arc=" << r2[best_idx*4+0]/area_scale
                    << " ovhg_arc=" << r2[best_idx*4+1]/area_scale
                    << " fill=" << r2[best_idx*4+2]/area_scale
                    << " top=" << r2[best_idx*4+3]/area_scale
                    << " mesh_h=" << mh;
            }
        }

        return true;
    }

    bool orient(OrientMeshs& items, const OrientParams& params, std::string* error)
    {
        std::lock_guard<std::mutex> lock(mutex);

        if (!ok) {
            if (error) *error = init_error;
            return false;
        }

        if (!make_current()) {
            ok = rebuild(/*force_new=*/true);
            if (!ok) {
                if (error) *error = init_error;
                return false;
            }
        }

        for (size_t i = 0; i < items.size(); ++i) {
            if (params.stopcondition && params.stopcondition())
                return false;
            if (params.progressind)
                params.progressind((unsigned)i, items[i].name);

            if (!orient_one_mesh(items[i], params, error))
                return false;
        }
        if (params.progressind)
            params.progressind((unsigned)items.size(), items.empty() ? "" : items.back().name);

        return true;
    }

#else
    // macOS and other unsupported platforms - stub
    Impl() = default;
    ~Impl() = default;
    bool available() const noexcept { return false; }
    const std::string& error() const noexcept { static const std::string empty; return empty; }
    bool orient(OrientMeshs&, const OrientParams&, std::string*) { return false; }
#endif // _WIN32 || __linux__
};

// --------------------------- public facade ---------------------------

GpuOrient::GpuOrient() : impl_(new Impl) {}
GpuOrient::~GpuOrient() = default;

bool GpuOrient::available() const noexcept
{
#if defined(_WIN32) || (defined(__linux__) && !defined(__APPLE__))
    return impl_ && impl_->available();
#else
    return false;
#endif
}

bool GpuOrient::orient(OrientMeshs &items,
                       const OrientMeshs &excludes,
                       const OrientParams &params,
                       bool fallback_to_cpu,
                       std::string *error)
{
    (void)excludes;
    if (error) error->clear();

#if defined(_WIN32) || (defined(__linux__) && !defined(__APPLE__))
    if (impl_ && impl_->available()) {
        std::string tmp;
        std::string* out_err = error ? error : &tmp;

        if (impl_->orient(items, params, out_err))
            return true;
        if ((params.stopcondition && params.stopcondition()) || (out_err && *out_err == "Canceled")) {
            if (out_err && out_err->empty())
                *out_err = "Canceled";
            return false;
        }

        if (fallback_to_cpu && out_err && !out_err->empty())
            BOOST_LOG_TRIVIAL(warning) << "GpuOrient(A-full) failed, falling back to CPU: " << *out_err;

        if (!fallback_to_cpu)
            return false;
    }
#endif

    if (fallback_to_cpu) {
        ::Slic3r::orientation::orient(items, excludes, params);
        return true;
    }

    if (error)
        *error = impl_ ? impl_->error() : "GPU orient unavailable";
    return false;
}

} // namespace orientation
} // namespace Slic3r
