#include "libslic3r/libslic3r.h"
#include "libslic3r/Platform.hpp"
#include "GLShadersManager.hpp"
#include "3DScene.hpp"
#include "GUI_App.hpp"

#include <cassert>
#include <algorithm>
#include <string_view>
using namespace std::literals;

#include <glad/gl.h>

namespace Slic3r {

std::pair<bool, std::string> GLShadersManager::init()
{
    std::string error;

    auto append_shader = [this, &error](const std::string& name, const GLShaderProgram::ShaderFilenames& filenames,
        const std::initializer_list<std::string_view> &defines = {}) {
        m_shaders.push_back(std::make_unique<GLShaderProgram>());
        if (!m_shaders.back()->init_from_files(name, filenames, defines)) {
            error += name + "\n";
            // if any error happens while initializating the shader, we remove it from the list
            m_shaders.pop_back();
            return false;
        }
        return true;
    };

    assert(m_shaders.empty());

    bool valid = true;

    bool gl31 = GUI::wxGetApp().is_gl_version_greater_or_equal_to(3, 1);
    const std::string prefix = gl31 ? "140/" : "110/";

	// imgui shader
    valid &= append_shader("imgui", { prefix + "imgui.vs", prefix + "imgui.fs" });
    // basic shader, used to render all what was previously rendered using the immediate mode
    valid &= append_shader("flat", { prefix + "flat.vs", prefix + "flat.fs" });
    // basic shader with plane clipping, used to render volumes in picking pass
    valid &= append_shader("flat_clip", { prefix + "flat_clip.vs", prefix + "flat_clip.fs" });
    // basic shader for textures, used to render textures
    valid &= append_shader("flat_texture", { prefix + "flat_texture.vs", prefix + "flat_texture.fs" });
    // used to render 3D scene background
    valid &= append_shader("background", { prefix + "background.vs", prefix + "background.fs" });
    // used to render bed axes and model, selection hints, gcode sequential view marker model, preview shells, options in gcode preview
    valid &= append_shader("gouraud_light", { prefix + "gouraud_light.vs", prefix + "gouraud_light.fs" });
    // used to render bed axes and model, selection hints, gcode sequential view marker model, preview shells, options in gcode preview
    valid &= append_shader("gouraud_preview", { prefix + "gouraud_preview.vs", prefix + "gouraud_preview.fs" });
    //used to render thumbnail
    valid &= append_shader("thumbnail", { prefix + "thumbnail.vs", prefix + "thumbnail.fs"});
    // Used to recolor thumbnail (lit + no-light mask) in GUI preview on easy_mode.
    valid &= append_shader("thumbnail_recolor_no_light", { prefix + "thumbnail_recolor_no_light.vs", prefix + "thumbnail_recolor_no_light.fs" });
    // used to render printbed
    valid &= append_shader("printbed", { prefix + "printbed.vs", prefix + "printbed.fs" });
    // used to render options in gcode preview
    if (GUI::wxGetApp().is_gl_version_greater_or_equal_to(3, 3)) {
        valid &= append_shader("gouraud_light_instanced", { prefix + "gouraud_light_instanced.vs", prefix + "gouraud_light_instanced.fs" });
    }

    // used to render objects in 3d editor
    valid &= append_shader("gouraud", { prefix + "gouraud.vs", prefix + "gouraud.fs" }
#if ENABLE_ENVIRONMENT_MAP
        , { "ENABLE_ENVIRONMENT_MAP"sv }
#endif // ENABLE_ENVIRONMENT_MAP
        );

    if (GUI::wxGetApp().is_gl_version_greater_or_equal_to(3, 3)) {
        // used to render clone shells
        valid &= append_shader("clone_preview_instance", { prefix + "clone_preview_instance.vs", prefix + "clone_preview_instance.fs" });
    }
    else
    {
        valid &= append_shader("clone_preview", { prefix + "clone_preview.vs", prefix + "clone_preview.fs" });
    }

    // used to render variable layers heights in 3d editor
    valid &= append_shader("variable_layer_height", { prefix + "variable_layer_height.vs", prefix + "variable_layer_height.fs" });
    // used to render highlight contour around selected triangles inside the multi-material gizmo
    valid &= append_shader("mm_contour", { prefix + "mm_contour.vs", prefix + "mm_contour.fs" });
    // Used to render painted triangles inside the multi-material gizmo. Triangle normals are computed inside fragment shader.
    // For Apple's on Arm CPU computed triangle normals inside fragment shader using dFdx and dFdy has the opposite direction.
    // Because of this, objects had darker colors inside the multi-material gizmo.
    // Based on https://stackoverflow.com/a/66206648, the similar behavior was also spotted on some other devices with Arm CPU.
    // Since macOS 12 (Monterey), this issue with the opposite direction on Apple's Arm CPU seems to be fixed, and computed
    // triangle normals inside fragment shader have the right direction.
    if (platform_flavor() == PlatformFlavor::OSXOnArm && wxPlatformInfo::Get().GetOSMajorVersion() < 12)
        valid &= append_shader("mm_gouraud", { prefix + "mm_gouraud.vs", prefix + "mm_gouraud.fs" }, { "FLIP_TRIANGLE_NORMALS"sv });
    else
        valid &= append_shader("mm_gouraud", { prefix + "mm_gouraud.vs", prefix + "mm_gouraud.fs" });

    valid &= append_shader("silhouette", { prefix + "silhouette.vs", prefix + "silhouette.fs" });
    valid &= append_shader("silhouette_composite", { prefix + "silhouette_composite.vs", prefix + "silhouette_composite.fs" });
    valid &= append_shader("mainframe_composite", { prefix + "mainframe_composite.vs", prefix + "mainframe_composite.fs" });
    valid &= append_shader("fxaa", { prefix + "fxaa.vs", prefix + "fxaa.fs" });
    valid &= append_shader("gaussian_blur33", { prefix + "gaussian_blur33.vs", prefix + "gaussian_blur33.fs" });

	valid &= append_shader("gcode_gouraud_light", {prefix + "gcode_gouraud_light.vs", prefix + "gcode_gouraud_light.fs"});
    valid &= append_shader("gcode_flat", {prefix + "gcode_flat.vs", prefix + "gcode_flat.fs"});
    
	if (gl31) {
        valid &= append_shader("gcode", {prefix + "gcode.vs", prefix + "gcode.fs"});
        valid &= append_shader("gcode_options", {prefix + "gcode_options.vs", prefix + "gcode_options.fs"});
        valid &= append_shader("gcode_custom_effect", {prefix + "gcode.vs", prefix + "gcode_custom_effect.fs"});
    }


    return { valid, error };
}

void GLShadersManager::shutdown()
{
    m_shaders.clear();
}

bool GLShadersManager::has_valid_programs() const
{
    if (m_shaders.empty())
        return false;

    return std::all_of(m_shaders.begin(), m_shaders.end(), [](const std::unique_ptr<GLShaderProgram>& shader) {
        return shader && shader->get_id() > 0 && ::glIsProgram(shader->get_id()) == GL_TRUE;
    });
}

GLShaderProgram* GLShadersManager::get_shader(const std::string& shader_name)
{
    auto it = std::find_if(m_shaders.begin(), m_shaders.end(), [&shader_name](std::unique_ptr<GLShaderProgram>& p) { return p->get_name() == shader_name; });
    return (it != m_shaders.end()) ? it->get() : nullptr;
}

GLShaderProgram* GLShadersManager::get_current_shader()
{
    GLint id = 0;
    glsafe(::glGetIntegerv(GL_CURRENT_PROGRAM, &id));
    if (id == 0)
        return nullptr;

    auto it = std::find_if(m_shaders.begin(), m_shaders.end(), [id](std::unique_ptr<GLShaderProgram>& p) { return static_cast<GLint>(p->get_id()) == id; });
    return (it != m_shaders.end()) ? it->get() : nullptr;
}

void GLShadersManager::bind_shader(const GLShaderProgram* p_shader)
{
    if (p_shader) {
        p_shader->start_using();
    } else {
        glsafe(::glUseProgram(0));
    }
}

void GLShadersManager::unbind_shader()
{
    glsafe(::glUseProgram(0));
}

} // namespace Slic3r

