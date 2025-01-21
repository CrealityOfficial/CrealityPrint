#include "libslic3r/Point.hpp"
#include "libslic3r/libslic3r.h"

#include "GLToolbarCollapse.h"

#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/Camera.hpp"
#include "slic3r/GUI/Plater.hpp"

#include <wx/event.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/settings.h>
#include <wx/glcanvas.h>

#include "../Config/DispConfig.h"
#include <cstdint> 
namespace Slic3r {
namespace GUI {
namespace CollapseBar{

    const GLToolbarItem::ActionCallback GLToolbarItem::Default_Action_Callback = [](){};
const GLToolbarItem::VisibilityCallback GLToolbarItem::Default_Visibility_Callback = []()->bool { return true; };
const GLToolbarItem::EnablingCallback GLToolbarItem::Default_Enabling_Callback = []()->bool { return true; };
const GLToolbarItem::RenderCallback GLToolbarItem::Default_Render_Callback = [](float, float, float, float){};

GLToolbarItem::Data::Option::Option()
    : toggable(false)
    , action_callback(Default_Action_Callback)
    , render_callback(nullptr)
{
}

GLToolbarItem::Data::Data():
    tooltip("")
    , additional_tooltip("")
    , visible(true)
    , visibility_callback(Default_Visibility_Callback)
    , enabling_callback(Default_Enabling_Callback)

{
}

GLToolbarItem::GLToolbarItem(GLToolbarItem::EType type, const GLToolbarItem::Data& data)
    : m_type(type)
    , m_state(Normal)
    , m_data(data)
    , m_highlight_state(NotHighlighted)
{
    render_left_pos = 0.0f;
}

bool GLToolbarItem::update_enabled_state()
{
    bool enabled = m_data.enabling_callback();
    bool ret = (is_enabled() != enabled);
    if (ret)
        m_state = enabled ? GLToolbarItem::Normal : GLToolbarItem::Disabled;

    return ret;
}

void GLToolbarItem::render(float left, float right, float bottom, float top, float scale) const
{
	GLTexture::Quad_UVs ret;
	ret.left_top = { 0.0f, 1.0f };
	ret.left_bottom = { 0.0f, 0.0f };
	ret.right_bottom = { 1.0f, 0.0f };
	ret.right_top = { 1.0f, 1.0f };

    bool sel = wxGetApp().plater()->is_sidebar_collapsed();
    bool hovel = m_state != EState::Normal;
    auto vid = DispConfig().getTexture(DispConfig::e_tt_collapse_item, hovel, sel);
    if (vid)
    {
        GLTexture::render_sub_texture(vid->get_id(), left, right, bottom, top, ret);
    }
}

GLToolbar::Layout::Layout()
    : type(Horizontal)
    , horizontal_orientation(HO_Center)
    , vertical_orientation(VO_Center)
    , top(0.0f)
    , left(0.0f)
    , border(0.0f)
    , separator_size(0.0f)
    , gap_size(0.0f)
    , scale(1.0f)
    , width(0.0f)
    , height(0.0f)
    , dirty(true)
{
}

GLToolbar::GLToolbar(GLToolbar::EType type, const std::string& name)
    : m_type(type)
    , m_name(name)
	, m_enabled(false)
    , m_pressed_toggable_id(-1)
{
}

GLToolbar::~GLToolbar()
{
    delete m_item;
}

GLToolbar::Layout::EType GLToolbar::get_layout_type() const
{
    return m_layout.type;
}

void GLToolbar::set_layout_type(GLToolbar::Layout::EType type)
{
    m_layout.type = type;
    m_layout.dirty = true;
}

void GLToolbar::set_position(float top, float left)
{
    m_layout.top = top;
    m_layout.left = left;
}

void GLToolbar::set_border(float border)
{
    m_layout.border = border;
    m_layout.dirty = true;
}

void GLToolbar::set_scale(float scale)
{
    if (m_layout.scale != scale) {
        m_layout.scale = scale;
        m_layout.dirty = true;
    }
}

//BBS: GUI refactor: GLToolbar
bool GLToolbar::init_item(const GLToolbarItem::Data& data, GLToolbarItem::EType type)
{
    if(nullptr != m_item)
    {
        return false;
    }

    m_item = new GLToolbarItem(type, data);
    
    m_layout.dirty = true;

    return true;
}

float GLToolbar::get_width()
{
    if (m_layout.dirty)
        calc_layout();

    return m_layout.width;
}

float GLToolbar::get_height()
{
    if (m_layout.dirty)
        calc_layout();

    return m_layout.height;
}

void GLToolbar::render(const GLCanvas3D& parent,GLToolbarItem::EType type)
{
    if (!m_enabled || m_item == nullptr)
        return;

    switch (m_layout.type)
    {
    default:
    case Layout::Horizontal: { render_horizontal(parent,type); break; }
    case Layout::Vertical:   { render_vertical(parent); break; }
    }
}

bool GLToolbar::on_mouse(wxMouseEvent& evt, GLCanvas3D& parent)
{
    if (!m_enabled)
        return false;

    const Vec2d mouse_pos((double)evt.GetX(), (double)evt.GetY());
    bool processed = false;

    // mouse anywhere
    if (!evt.Dragging() && !evt.Leaving() && !evt.Entering() && m_mouse_capture.parent != nullptr) {
        if (m_mouse_capture.any() && (evt.LeftUp() || evt.MiddleUp() || evt.RightUp())) {
            // prevents loosing selection into the scene if mouse down was done inside the toolbar and mouse up was down outside it,
            // as when switching between views
            m_mouse_capture.reset();
            return true;
        }
        m_mouse_capture.reset();
    }

    if (evt.Moving())
        update_hover_state(mouse_pos, parent);
    else if (evt.LeftUp()) {
        if (m_mouse_capture.left) {
            processed = true;
            m_mouse_capture.left = false;
        }
        else
            return false;
    }
    else if (evt.MiddleUp()) {
        if (m_mouse_capture.middle) {
            processed = true;
            m_mouse_capture.middle = false;
        }
        else
            return false;
    }
    else if (evt.RightUp()) {
        if (m_mouse_capture.right) {
            processed = true;
            m_mouse_capture.right = false;
        }
        else
            return false;
    }
    else if (evt.Dragging()) {
        if (m_mouse_capture.any())
            // if the button down was done on this toolbar, prevent from dragging into the scene
            processed = true;
        else
            return false;
    }

    if (m_item != nullptr && contains_mouse(mouse_pos, parent))// mouse inside toolbar
    {
        if (evt.LeftDown() || evt.LeftDClick()) {
            m_mouse_capture.left = true;
            m_mouse_capture.parent = &parent;
            processed = true;
       
            // mouse is inside an icon
            do_action(GLToolbarItem::Left, 0, parent, true);
            parent.set_as_dirty();
        }
        else if (evt.MiddleDown()) {
            m_mouse_capture.middle = true;
            m_mouse_capture.parent = &parent;
        }
        else if (evt.RightDown()) {
            m_mouse_capture.right = true;
            m_mouse_capture.parent = &parent;
            processed = true;
            
            do_action(GLToolbarItem::Right, 0, parent, true);
			parent.set_as_dirty();
        }
    }

    return processed;
}

void GLToolbar::calc_layout()
{
    switch (m_layout.type)
    {
    default:
    case Layout::Horizontal:
    {
        m_layout.width = get_width_horizontal();
        m_layout.height = get_height_horizontal();
        break;
    }
    case Layout::Vertical:
    {
        m_layout.width = get_width_vertical();
        m_layout.height = get_height_vertical();
        break;
    }
    }

    m_layout.dirty = false;
}

//BBS: GUI refactor: GLToolbar
float GLToolbar::get_width_horizontal() const
{
    auto tt = DispConfig().getTexture(DispConfig::e_tt_collapse_item);
	float size = tt->get_width();
    return size * m_layout.scale;
}

//BBS: GUI refactor: GLToolbar
float GLToolbar::get_width_vertical() const
{
    auto tt = DispConfig().getTexture(DispConfig::e_tt_collapse_item);
	float size = tt->get_width();
	return size * m_layout.scale;
}

float GLToolbar::get_height_horizontal() const
{
    auto bk = DispConfig().getTexture(DispConfig::e_tt_collapse);
    return bk->get_height()* m_layout.scale;
}

float GLToolbar::get_height_vertical() const
{
    auto bk = DispConfig().getTexture(DispConfig::e_tt_collapse);
    return bk->get_height()* m_layout.scale;
}

void GLToolbar::do_action(GLToolbarItem::EActionType type, int item_id, GLCanvas3D& parent, bool check_hover)
{
    if (m_pressed_toggable_id == -1 || m_pressed_toggable_id == item_id)
    {
		if (m_item != nullptr && (!check_hover || m_item->is_hovered()))
		{
			m_item->set_state(m_item->is_hovered() ? GLToolbarItem::HoverPressed : GLToolbarItem::Pressed);

			parent.render();
			switch (type)
			{
			default:
			case GLToolbarItem::Left: { m_item->do_left_action(); break; }
			case GLToolbarItem::Right: { m_item->do_right_action(); break; }
			}

			if (m_type == Normal && m_item->get_state() != GLToolbarItem::Disabled) {
				// the item may get disabled during the action, if not, set it back to normal state
				m_item->set_state(GLToolbarItem::Normal);
				parent.render();
			}
		}
    }
}

void GLToolbar::update_hover_state(const Vec2d& mouse_pos, GLCanvas3D& parent)
{
    if (!m_enabled)
        return;

    switch (m_layout.type)
    {
    default:
    case Layout::Horizontal: { update_hover_state_horizontal(mouse_pos, parent); break; }
    case Layout::Vertical:   { update_hover_state_vertical(mouse_pos, parent); break; }
    }
}

void GLToolbar::update_hover_state_horizontal(const Vec2d& mouse_pos, GLCanvas3D& parent)
{
    
}

void GLToolbar::update_hover_state_vertical(const Vec2d& mouse_pos, GLCanvas3D& parent)
{
    const Size cnv_size = parent.get_canvas_size();
    const Vec2d scaled_mouse_pos((mouse_pos.x() - 0.5 * (double)cnv_size.get_width()), (0.5 * (double)cnv_size.get_height() - mouse_pos.y()));

    const float border = m_layout.border * m_layout.scale;

    float left = m_layout.left + border;
    float top  = m_layout.top - border;
    auto tt = DispConfig().getTexture(DispConfig::e_tt_collapse_item);
	float right = cnv_size.get_width() * 0.5;
	left = right - tt->get_width();
	float bottom = top - tt->get_height();

	GLToolbarItem::EState state = m_item->get_state();
	const bool inside = (left <= (float)scaled_mouse_pos.x()) &&
		((float)scaled_mouse_pos.x() <= right) &&
		(bottom <= (float)scaled_mouse_pos.y()) &&
		((float)scaled_mouse_pos.y() <= top);

	switch (state)
	{
	case GLToolbarItem::Normal:
	{
		if (inside) {
            m_item->set_state(GLToolbarItem::Hover);
			parent.set_as_dirty();
		}

		break;
	}
	case GLToolbarItem::Hover:
	{
		if (!inside) {
            m_item->set_state(GLToolbarItem::Normal);
			parent.set_as_dirty();
		}

		break;
	}
	case GLToolbarItem::Pressed:
	{
		if (inside) {
            m_item->set_state(GLToolbarItem::HoverPressed);
			parent.set_as_dirty();
		}

		break;
	}
	case GLToolbarItem::HoverPressed:
	{
		if (!inside) {
            m_item->set_state(GLToolbarItem::Pressed);
			parent.set_as_dirty();
		}

		break;
	}
	case GLToolbarItem::Disabled:
	{
		if (inside) {
            m_item->set_state(GLToolbarItem::HoverDisabled);
			parent.set_as_dirty();
		}

		break;
	}
	case GLToolbarItem::HoverDisabled:
	{
		if (!inside) {
            m_item->set_state(GLToolbarItem::Disabled);
			parent.set_as_dirty();
		}

		break;
	}
	default:
	{
		break;
	}
	}
}

bool GLToolbar::contains_mouse(const Vec2d& mouse_pos, const GLCanvas3D& parent) const
{
    if (!m_enabled)
        return -1;

    switch (m_layout.type)
    {
    default:
    case Layout::Horizontal: { return contains_mouse_horizontal(mouse_pos, parent); }
    case Layout::Vertical:   { return contains_mouse_vertical(mouse_pos, parent); }
    }

    return false;
}

bool GLToolbar::contains_mouse_horizontal(const Vec2d& mouse_pos, const GLCanvas3D& parent) const
{
    return false;
}

bool GLToolbar::contains_mouse_vertical(const Vec2d& mouse_pos, const GLCanvas3D& parent) const
{
    const Size cnv_size = parent.get_canvas_size();
    const Vec2d scaled_mouse_pos((mouse_pos.x() - 0.5 * (double)cnv_size.get_width()), (0.5 * (double)cnv_size.get_height() - mouse_pos.y()));

    const float border = m_layout.border * m_layout.scale;

    const float left = m_layout.left + border;
    float top = m_layout.top - border;

	if (!m_item->is_visible())
		false;

    auto tt = DispConfig().getTexture(DispConfig::e_tt_collapse_item);
	float right = left + tt->get_width();
	float bottom = top - tt->get_height();

	// mouse inside the icon
	if (left <= (float)scaled_mouse_pos.x() &&
		(float)scaled_mouse_pos.x() <= right &&
		bottom <= (float)scaled_mouse_pos.y() &&
		(float)scaled_mouse_pos.y() <= top)
		return true;

    return false;
}

void GLToolbar::render_background(float left, float top, float right, float bottom, float border_w, float border_h) const
{
    const std::uintptr_t tex_id = reinterpret_cast<std::uintptr_t>(DispConfig().getTextureId(DispConfig::e_tt_collapse));
	const float left_uv = 0.0f;
	const float right_uv = 1.0f;
	const float top_uv = 1.0f;
	const float bottom_uv = 0.0f;

	GLTexture::Quad_UVs ret;
	ret.left_top = { left_uv, top_uv };
	ret.left_bottom = { left_uv, bottom_uv };
	ret.right_bottom = { right_uv, bottom_uv };
	ret.right_top = { right_uv, top_uv };

	GLTexture::render_sub_texture(tex_id, left, right, bottom, top, ret);
}

void GLToolbar::render_horizontal(const GLCanvas3D& parent,GLToolbarItem::EType type)
{
    const Size cnv_size = parent.get_canvas_size();
    const float cnv_w = (float)cnv_size.get_width();
    const float cnv_h = (float)cnv_size.get_height();

    if (cnv_w == 0 || cnv_h == 0)
        return;

    const float inv_cnv_w = 1.0f / cnv_w;
    const float inv_cnv_h = 1.0f / cnv_h;

    const float border_w = 2.0f * m_layout.border * m_layout.scale * inv_cnv_w;
    const float border_h = 2.0f * m_layout.border * m_layout.scale * inv_cnv_h;
    const float width = 2.0f * get_width() * inv_cnv_w;
    const float height = 2.0f * get_height() * inv_cnv_h;


    float left   = 2.0f * m_layout.left * inv_cnv_w;
    float top    = 2.0f * m_layout.top * inv_cnv_h;
    float right = left + width;
    if (type == GLToolbarItem::SeparatorLine)
        right = left + width * 0.5;
    const float bottom = top - height;

    render_background(left, top, right, bottom, border_w, border_h);

    left += border_w;
    top  -= border_h;

	//BBS GUI refactor
	m_item->render_left_pos = left;
// 	unsigned int tex_id = m_icons_texture.get_id();
// 	int tex_width = m_icons_texture.get_width();
// 	int tex_height = m_icons_texture.get_height();
// 	if ((tex_id == 0) || (tex_width <= 0) || (tex_height <= 0))
// 		return;

	m_item->render(left, top, right, bottom, m_layout.scale);
}

void GLToolbar::render_vertical(const GLCanvas3D& parent)
{
    const Size cnv_size = parent.get_canvas_size();
    const float cnv_w = (float)cnv_size.get_width();
    const float cnv_h = (float)cnv_size.get_height();

    if (cnv_w == 0 || cnv_h == 0)
        return;

    const float inv_cnv_w = 1.0f / cnv_w;
    const float inv_cnv_h = 1.0f / cnv_h;

    const float border_w = 2.0f * m_layout.border * m_layout.scale * inv_cnv_w;
    const float border_h = 2.0f * m_layout.border * m_layout.scale * inv_cnv_h;
    auto bk = DispConfig().getTexture(DispConfig::e_tt_collapse);
    const float width = 2.0f * bk->get_width() * inv_cnv_w;
    const float height = 2.0f * bk->get_height() * inv_cnv_h;

    float left         = 2.0f * m_layout.left * inv_cnv_w;
    float top          = 2.0f * m_layout.top * inv_cnv_h;
    float right  = left + width;
    float bottom = top - height;

    render_background(left, top, right, bottom, border_w, border_h);


    left += border_w;
    top  -= border_h;	
    m_item->render(left, right, bottom, top, m_layout.scale);
}


GLToolbarItem::Data::Data(const GLToolbarItem::Data& data)
{
    tooltip = data.tooltip;
    additional_tooltip = data.additional_tooltip;
    left = data.left;
    right = data.right;
    visible = data.visible;
    visibility_callback = data.visibility_callback;
    enabling_callback = data.enabling_callback;
}

}
} // namespace GUI
} // namespace Slic3r
