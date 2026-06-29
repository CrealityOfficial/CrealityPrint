#include "ImGuiThumbnailPreview.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

#include <GL/glew.h>

#include "../../GLShader.hpp"
#include "../../GUI_App.hpp"
#include "libslic3r/GCode/ThumbnailData.hpp"

namespace Slic3r {
namespace GUI {

namespace {

static uint64_t fnv1a_64(uint64_t h, const unsigned char* data, size_t len)
{
    static constexpr uint64_t kOff   = 1469598103934665603ull;
    static constexpr uint64_t kPrime = 1099511628211ull;
    if (h == 0)
        h = kOff;
    for (size_t i = 0; i < len; ++i) {
        h ^= (uint64_t)data[i];
        h *= kPrime;
    }
    return h;
}

template <class T>
static const T* ptr_from_shader_handle(T* p)
{
    return p;
}

template <class T>
static const T* ptr_from_shader_handle(const std::shared_ptr<T>& p)
{
    return p.get();
}

template <class T>
static const T* ptr_from_shader_handle(const std::shared_ptr<const T>& p)
{
    return p.get();
}

} // namespace

ImGuiThumbnailPreview::~ImGuiThumbnailPreview()
{
    this->reset();
}

void ImGuiThumbnailPreview::reset()
{
    if (m_tex != 0) {
        GLuint tex = (GLuint)m_tex;
        glDeleteTextures(1, &tex);
        m_tex = 0;
    }
    m_w   = 0;
    m_h   = 0;
    m_sig = 0;

    if (m_lit_tex != 0) {
        GLuint tex = (GLuint)m_lit_tex;
        glDeleteTextures(1, &tex);
        m_lit_tex = 0;
    }
    m_lit_w   = 0;
    m_lit_h   = 0;
    m_lit_sig = 0;

    if (m_no_light_tex != 0) {
        GLuint tex = (GLuint)m_no_light_tex;
        glDeleteTextures(1, &tex);
        m_no_light_tex = 0;
    }
    m_no_light_w   = 0;
    m_no_light_h   = 0;
    m_no_light_sig = 0;

    if (m_recolored_tex != 0) {
        GLuint tex = (GLuint)m_recolored_tex;
        glDeleteTextures(1, &tex);
        m_recolored_tex = 0;
    }
    m_recolored_w   = 0;
    m_recolored_h   = 0;
    m_recolored_sig = 0;

    if (m_recolor_fbo != 0) {
        GLuint fbo = (GLuint)m_recolor_fbo;
        glDeleteFramebuffers(1, &fbo);
        m_recolor_fbo = 0;
    }

    if (m_recolor_vao != 0) {
        if (GLEW_VERSION_3_0 || GLEW_ARB_vertex_array_object) {
            GLuint vao = (GLuint)m_recolor_vao;
            glDeleteVertexArrays(1, &vao);
        }
        m_recolor_vao = 0;
    }

    if (m_recolor_vbo != 0) {
        GLuint vbo = (GLuint)m_recolor_vbo;
        glDeleteBuffers(1, &vbo);
        m_recolor_vbo = 0;
    }
}

uint64_t ImGuiThumbnailPreview::signature(const ThumbnailData& td)
{
    uint64_t h = 0;
    h          = fnv1a_64(h, (const unsigned char*)&td.width, sizeof(td.width));
    h          = fnv1a_64(h, (const unsigned char*)&td.height, sizeof(td.height));
    const size_t n = td.pixels.size();
    h               = fnv1a_64(h, (const unsigned char*)&n, sizeof(n));

    if (!td.pixels.empty())
        h = fnv1a_64(h, td.pixels.data(), n);
    return h;
}

uint64_t ImGuiThumbnailPreview::signature(const std::vector<RGB8>& extruder_colors, const ThumbnailRecolorParams& params)
{
    uint64_t h = 0;
    const size_t n = extruder_colors.size();
    h               = fnv1a_64(h, (const unsigned char*)&n, sizeof(n));
    if (!extruder_colors.empty())
        h = fnv1a_64(h, (const unsigned char*)extruder_colors.data(), extruder_colors.size() * sizeof(RGB8));
    h = fnv1a_64(h, (const unsigned char*)&params.emission_factor, sizeof(params.emission_factor));
    return h;
}

void ImGuiThumbnailPreview::update_texture(const ThumbnailData& td)
{
    this->update_texture(td, m_tex, m_w, m_h, m_sig, (int)GL_LINEAR);
}

void ImGuiThumbnailPreview::update_texture(const ThumbnailData& td,
                                          unsigned& tex,
                                          unsigned& w,
                                          unsigned& h,
                                          uint64_t& sig,
                                          int filter)
{
    if (!td.is_valid())
        return;

    const uint64_t new_sig = signature(td);
    if (new_sig == sig && td.width == w && td.height == h)
        return;

    if (tex == 0) {
        GLuint new_tex = 0;
        glGenTextures(1, &new_tex);
        tex = (unsigned)new_tex;
        if (tex == 0)
            return;
    }

    glBindTexture(GL_TEXTURE_2D, (GLuint)tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLenum)filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLenum)filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    const bool size_changed = (td.width != w) || (td.height != h);
    const void* px          = td.pixels.empty() ? nullptr : (const void*)td.pixels.data();
    if (size_changed) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA8,
                     (GLsizei)td.width,
                     (GLsizei)td.height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     px);
        w = td.width;
        h = td.height;
    } else {
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, 0, 0, (GLsizei)td.width, (GLsizei)td.height, GL_RGBA, GL_UNSIGNED_BYTE, px);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    sig = new_sig;
}

bool ImGuiThumbnailPreview::ensure_recolor_program()
{
    const GLShaderProgram* shader = ptr_from_shader_handle(wxGetApp().get_shader("thumbnail_recolor_no_light"));
    if (shader == nullptr)
        return false;

    if (m_recolor_vbo == 0) {
        static constexpr float kTri[6] = {-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};
        GLuint vbo = 0;
        glGenBuffers(1, &vbo);
        m_recolor_vbo = (unsigned)vbo;
        if (m_recolor_vbo == 0)
            return false;

        glBindBuffer(GL_ARRAY_BUFFER, (GLuint)m_recolor_vbo);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(kTri), (const void*)kTri, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ((GLEW_VERSION_3_0 || GLEW_ARB_vertex_array_object) && m_recolor_vao == 0) {
        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        m_recolor_vao = (unsigned)vao;
        if (m_recolor_vao == 0)
            return false;
    }

    return true;
}

bool ImGuiThumbnailPreview::update_recolored_texture(const ThumbnailData& lit_td,
                                                     const ThumbnailData& no_light_td,
                                                     const std::vector<RGB8>& extruder_colors,
                                                     const ThumbnailRecolorParams& params)
{
    if (!lit_td.is_valid() || !no_light_td.is_valid())
        return false;
    if (lit_td.width != no_light_td.width || lit_td.height != no_light_td.height)
        return false;

    if (!this->ensure_recolor_program())
        return false;

    const GLShaderProgram* shader = ptr_from_shader_handle(wxGetApp().get_shader("thumbnail_recolor_no_light"));
    if (shader == nullptr)
        return false;

    // Upload inputs (no-light must be nearest to preserve alpha-based indices).
    this->update_texture(lit_td, m_lit_tex, m_lit_w, m_lit_h, m_lit_sig, (int)GL_NEAREST);
    this->update_texture(no_light_td, m_no_light_tex, m_no_light_w, m_no_light_h, m_no_light_sig, (int)GL_NEAREST);
    if (m_lit_tex == 0 || m_no_light_tex == 0)
        return false;

    const uint64_t colors_sig = signature(extruder_colors, params);

    uint64_t input_sig = 0;
    input_sig          = fnv1a_64(input_sig, (const unsigned char*)&m_lit_sig, sizeof(m_lit_sig));
    input_sig          = fnv1a_64(input_sig, (const unsigned char*)&m_no_light_sig, sizeof(m_no_light_sig));
    input_sig          = fnv1a_64(input_sig, (const unsigned char*)&colors_sig, sizeof(colors_sig));
    input_sig          = fnv1a_64(input_sig, (const unsigned char*)&lit_td.width, sizeof(lit_td.width));
    input_sig          = fnv1a_64(input_sig, (const unsigned char*)&lit_td.height, sizeof(lit_td.height));

    if (input_sig == m_recolored_sig && m_recolored_w == lit_td.width && m_recolored_h == lit_td.height &&
        m_recolored_tex != 0)
        return true;

    if (m_recolored_tex == 0) {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        m_recolored_tex = (unsigned)tex;
        if (m_recolored_tex == 0)
            return false;
    }

    glBindTexture(GL_TEXTURE_2D, (GLuint)m_recolored_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    const bool size_changed = (m_recolored_w != lit_td.width) || (m_recolored_h != lit_td.height);
    if (size_changed) {
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA8,
                     (GLsizei)lit_td.width,
                     (GLsizei)lit_td.height,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     nullptr);
        m_recolored_w = lit_td.width;
        m_recolored_h = lit_td.height;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    if (m_recolor_fbo == 0) {
        GLuint fbo = 0;
        glGenFramebuffers(1, &fbo);
        m_recolor_fbo = (unsigned)fbo;
        if (m_recolor_fbo == 0)
            return false;
    }

    GLint prev_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    GLint prev_viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, prev_viewport);
    GLint prev_prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prev_prog);
    const bool vao_supported = (GLEW_VERSION_3_0 || GLEW_ARB_vertex_array_object);
    GLint prev_vao = 0;
    if (vao_supported)
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prev_vao);
    GLint prev_array_buffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prev_array_buffer);
    const GLboolean prev_blend = glIsEnabled(GL_BLEND);
    GLint prev_active_tex = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &prev_active_tex);

    const int loc_pos = shader->get_attrib_location("Position");
    if (loc_pos < 0)
        return false;

    GLint prev_pos_enabled = 0;
    if (!vao_supported)
        glGetVertexAttribiv(loc_pos, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &prev_pos_enabled);

    GLint prev_tex0 = 0;
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex0);
    GLint prev_tex1 = 0;
    glActiveTexture(GL_TEXTURE1);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex1);

    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)m_recolor_fbo);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (GLuint)m_recolored_tex, 0);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prev_fbo);
        if (prev_blend)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
        if (vao_supported)
            glBindVertexArray((GLuint)prev_vao);
        glBindBuffer(GL_ARRAY_BUFFER, (GLuint)prev_array_buffer);
        if (!vao_supported && !prev_pos_enabled)
            glDisableVertexAttribArray((GLuint)loc_pos);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, (GLuint)prev_tex1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, (GLuint)prev_tex0);
        glActiveTexture((GLenum)prev_active_tex);
        glUseProgram((GLuint)prev_prog);
        glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
        return false;
    }

    glViewport(0, 0, (GLsizei)lit_td.width, (GLsizei)lit_td.height);
    glDisable(GL_BLEND);

    shader->start_using();
    shader->set_uniform("u_lit", 0);
    shader->set_uniform("u_no_light", 1);
    shader->set_uniform("u_inv_size", std::array<float, 2>{1.0f / (float)lit_td.width, 1.0f / (float)lit_td.height});

    const int count = (int)std::min<size_t>(extruder_colors.size(), 32);
    shader->set_uniform("u_color_count", count);
    shader->set_uniform("u_emission_factor", params.emission_factor);
    {
        const int loc_colors = shader->get_uniform_location("u_colors[0]");
        if (loc_colors >= 0 && count > 0) {
            std::vector<float> cols;
            cols.resize((size_t)count * 3);
            for (int i = 0; i < count; ++i) {
                cols[(size_t)i * 3 + 0] = (float)extruder_colors[(size_t)i].r * (1.0f / 255.0f);
                cols[(size_t)i * 3 + 1] = (float)extruder_colors[(size_t)i].g * (1.0f / 255.0f);
                cols[(size_t)i * 3 + 2] = (float)extruder_colors[(size_t)i].b * (1.0f / 255.0f);
            }
            glUniform3fv(loc_colors, count, cols.data());
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)m_lit_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, (GLuint)m_no_light_tex);

    if (vao_supported && m_recolor_vao != 0)
        glBindVertexArray((GLuint)m_recolor_vao);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)m_recolor_vbo);
    glEnableVertexAttribArray((GLuint)loc_pos);
    glVertexAttribPointer((GLuint)loc_pos, 2, GL_FLOAT, GL_FALSE, (GLsizei)(2 * sizeof(float)), (const void*)0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    if (!vao_supported && !prev_pos_enabled)
        glDisableVertexAttribArray((GLuint)loc_pos);

    // Restore minimal state.
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)prev_fbo);
    glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
    glUseProgram((GLuint)prev_prog);
    if (prev_blend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    if (vao_supported)
        glBindVertexArray((GLuint)prev_vao);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)prev_array_buffer);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_tex1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)prev_tex0);
    glActiveTexture((GLenum)prev_active_tex);

    m_recolored_sig = input_sig;
    return true;
}

void ImGuiThumbnailPreview::draw(const ThumbnailData* thumbnail, const ImVec2& size, float scale)
{
    ImGui::PushID(this);
    ImGui::InvisibleButton("##thumb_preview", size);
    const ImVec2 rmin = ImGui::GetItemRectMin();
    const ImVec2 rmax = ImGui::GetItemRectMax();

    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float rounding = 6.0f * scale;
    const ImU32 bg_col   = IM_COL32(52, 52, 52, 255);
    const ImU32 br_col   = IM_COL32(120, 120, 120, 255);
    dl->AddRectFilled(rmin, rmax, bg_col, rounding);
    dl->AddRect(rmin, rmax, br_col, rounding, 0, 1.0f);

    if (thumbnail == nullptr || !thumbnail->is_valid()) {
        const char*  txt = "No preview";
        const ImVec2 ts  = ImGui::CalcTextSize(txt);
        const ImVec2 pos((rmin.x + rmax.x - ts.x) * 0.5f, (rmin.y + rmax.y - ts.y) * 0.5f);
        dl->AddText(pos, IM_COL32(200, 200, 200, 255), txt);
        ImGui::PopID();
        return;
    }

    this->update_texture(*thumbnail);
    if (m_tex == 0) {
        ImGui::PopID();
        return;
    }

    const float pad = 6.0f * scale;
    const ImVec2 inner_min(rmin.x + pad, rmin.y + pad);
    const ImVec2 inner_max(rmax.x - pad, rmax.y - pad);

    const float avail_w = inner_max.x - inner_min.x;
    const float avail_h = inner_max.y - inner_min.y;
    if (avail_w <= 0.0f || avail_h <= 0.0f) {
        ImGui::PopID();
        return;
    }

    const float img_w = (float)thumbnail->width;
    const float img_h = (float)thumbnail->height;
    if (img_w <= 0.0f || img_h <= 0.0f) {
        ImGui::PopID();
        return;
    }

    const float s = (avail_w / img_w < avail_h / img_h) ? (avail_w / img_w) : (avail_h / img_h);
    const ImVec2 draw_sz(img_w * s, img_h * s);
    const ImVec2 img_min(inner_min.x + (avail_w - draw_sz.x) * 0.5f, inner_min.y + (avail_h - draw_sz.y) * 0.5f);
    const ImVec2 img_max(img_min.x + draw_sz.x, img_min.y + draw_sz.y);

    // ThumbnailData pixels originate from glReadPixels (bottom-left origin). Flip vertically for ImGui.
    dl->AddImage((ImTextureID)(intptr_t)m_tex, img_min, img_max, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

    ImGui::PopID();
}

void ImGuiThumbnailPreview::draw_recolored(const ThumbnailData* lit_thumbnail,
                                          const ThumbnailData* no_light_thumbnail,
                                          const std::vector<RGB8>& extruder_colors,
                                          const ThumbnailRecolorParams& params,
                                          const ImVec2& size,
                                          float scale)
{
    ImGui::PushID(this);
    ImGui::InvisibleButton("##thumb_preview", size);
    const ImVec2 rmin = ImGui::GetItemRectMin();
    const ImVec2 rmax = ImGui::GetItemRectMax();

    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float rounding = 6.0f * scale;
    const bool dark_mode = wxGetApp().dark_mode();
    const ImU32 bg_col   = dark_mode ? IM_COL32(52, 52, 52, 255) : IM_COL32(240, 240, 240, 255);
    const ImU32 br_col   = IM_COL32(120, 120, 120, 255);
    dl->AddRectFilled(rmin, rmax, bg_col, rounding);
    dl->AddRect(rmin, rmax, br_col, rounding, 0, 1.0f);

    if (lit_thumbnail == nullptr || !lit_thumbnail->is_valid()) {
        const char*  txt = "No preview";
        const ImVec2 ts  = ImGui::CalcTextSize(txt);
        const ImVec2 pos((rmin.x + rmax.x - ts.x) * 0.5f, (rmin.y + rmax.y - ts.y) * 0.5f);
        dl->AddText(pos, IM_COL32(200, 200, 200, 255), txt);
        ImGui::PopID();
        return;
    }

    const bool can_recolor = (no_light_thumbnail != nullptr && no_light_thumbnail->is_valid() &&
                              no_light_thumbnail->width == lit_thumbnail->width &&
                              no_light_thumbnail->height == lit_thumbnail->height && !extruder_colors.empty());

    unsigned draw_tex = 0;
    unsigned draw_w   = 0;
    unsigned draw_h   = 0;
    if (can_recolor &&
        this->update_recolored_texture(*lit_thumbnail, *no_light_thumbnail, extruder_colors, params) &&
        m_recolored_tex != 0) {
        draw_tex = m_recolored_tex;
        draw_w   = m_recolored_w;
        draw_h   = m_recolored_h;
    } else {
        this->update_texture(*lit_thumbnail);
        draw_tex = m_tex;
        draw_w   = m_w;
        draw_h   = m_h;
    }

    if (draw_tex == 0 || draw_w == 0 || draw_h == 0) {
        ImGui::PopID();
        return;
    }

    const float pad = 6.0f * scale;
    const ImVec2 inner_min(rmin.x + pad, rmin.y + pad);
    const ImVec2 inner_max(rmax.x - pad, rmax.y - pad);

    const float avail_w = inner_max.x - inner_min.x;
    const float avail_h = inner_max.y - inner_min.y;
    if (avail_w <= 0.0f || avail_h <= 0.0f) {
        ImGui::PopID();
        return;
    }

    const float img_w = (float)draw_w;
    const float img_h = (float)draw_h;
    if (img_w <= 0.0f || img_h <= 0.0f) {
        ImGui::PopID();
        return;
    }

    const float s = (avail_w / img_w < avail_h / img_h) ? (avail_w / img_w) : (avail_h / img_h);
    const ImVec2 draw_sz(img_w * s, img_h * s);
    const ImVec2 img_min(inner_min.x + (avail_w - draw_sz.x) * 0.5f, inner_min.y + (avail_h - draw_sz.y) * 0.5f);
    const ImVec2 img_max(img_min.x + draw_sz.x, img_min.y + draw_sz.y);

    // ThumbnailData pixels originate from glReadPixels (bottom-left origin). Flip vertically for ImGui.
    dl->AddImage((ImTextureID)(intptr_t)draw_tex, img_min, img_max, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

    ImGui::PopID();
}

} // namespace GUI
} // namespace Slic3r

