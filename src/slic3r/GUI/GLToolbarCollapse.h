#ifndef GLTOOLBARCOLLAPSE_H
#define GLTOOLBARCOLLAPSE_H

#include <functional>
#include <string>
#include <vector>

#include "GLTexture.hpp"
#include "Event.hpp"
#include "libslic3r/Point.hpp"

class wxEvtHandler;

namespace Slic3r {
namespace GUI {
    class GLCanvas3D;

namespace CollapseBar {
class GLToolbarItem
{
public:
    typedef std::function<void()> ActionCallback;
    typedef std::function<bool()> VisibilityCallback;
    typedef std::function<bool()> EnablingCallback;
    typedef std::function<void(float, float, float, float)> RenderCallback;

    enum EType : unsigned char
    {
        Action,
        Separator,
        //BBS: GUI refactor: GLToolbar
        ActionWithText,
        ActionWithTextImage,
        SeparatorLine,
        Num_Types
    };

    enum EActionType : unsigned char
    {
        Undefined,
        Left,
        Right,
        Num_Action_Types
    };

    enum EState : unsigned char
    {
        Normal,
        Pressed,
        Disabled,
        Hover,
        HoverPressed,
        HoverDisabled,
        Num_States
    };

    enum EHighlightState : unsigned char
    {
        HighlightedShown,
        HighlightedHidden,
        Num_Rendered_Highlight_States,
        NotHighlighted
    };

    struct Data
    {
        struct Option
        {
            bool toggable;
            ActionCallback action_callback;
            RenderCallback render_callback;

            Option();

            bool can_render() const { return toggable && (render_callback != nullptr); }
        };

        std::string tooltip;
        std::string additional_tooltip;

        // mouse click
        Option left;
        Option right;

        bool visible;
        VisibilityCallback visibility_callback;
        EnablingCallback enabling_callback;

        Data();
        //BBS: GUI refactor: GLToolbar
        Data(const GLToolbarItem::Data& data);
    };

    static const ActionCallback Default_Action_Callback;
    static const VisibilityCallback Default_Visibility_Callback;
    static const EnablingCallback Default_Enabling_Callback;
    static const RenderCallback Default_Render_Callback;

private:
    EType m_type;
    EState m_state;
    Data m_data;
    EActionType m_last_action_type;
    EHighlightState m_highlight_state;
public:
    // remember left position for rendering menu
    mutable float render_left_pos;

    GLToolbarItem(EType type, const Data& data);

    EState get_state() const { return m_state; }
    void set_state(EState state) { m_state = state; }

    EHighlightState get_highlight() const { return m_highlight_state; }
    void set_highlight(EHighlightState state) { m_highlight_state = state; }

	const std::string& get_tooltip() const { return m_data.tooltip; }
	const std::string& get_additional_tooltip() const { return m_data.additional_tooltip; }
	void set_additional_tooltip(const std::string& text) { m_data.additional_tooltip = text; }
	void set_tooltip(const std::string& text) { m_data.tooltip = text; }

    void do_left_action() { m_last_action_type = Left; m_data.left.action_callback(); }
    void do_right_action() { m_last_action_type = Right; m_data.right.action_callback(); }

    bool is_enabled() const { return (m_state != Disabled) && (m_state != HoverDisabled); }
    bool is_disabled() const { return (m_state == Disabled) || (m_state == HoverDisabled); }
    bool is_hovered() const { return (m_state == Hover) || (m_state == HoverPressed) || (m_state == HoverDisabled); }
    bool is_pressed() const { return (m_state == Pressed) || (m_state == HoverPressed); }
    bool is_visible() const { return m_data.visible; }

    bool is_left_toggable() const { return m_data.left.toggable; }
    bool is_right_toggable() const { return m_data.right.toggable; }

    bool has_left_render_callback() const { return m_data.left.render_callback != nullptr; }
    bool has_right_render_callback() const { return m_data.right.render_callback != nullptr; }
     
    // returns true if the state changes
    bool update_enabled_state();
     
    void render(float left, float right, float bottom, float top, float scale) const;
private:
    void set_visible(bool visible) { m_data.visible = visible; }

    friend class GLToolbar;
};

class GLToolbar
{
public:
    static const float Default_Icons_Size;

    enum EType : unsigned char
    {
        Normal,
        Radio,
        Num_Types
    };

    struct Layout
    {
        enum EType : unsigned char
        {
            Horizontal,
            Vertical,
            Num_Types
        };

        enum EHorizontalOrientation : unsigned char
        {
            HO_Left,
            HO_Center,
            HO_Right,
            Num_Horizontal_Orientations
        };

        enum EVerticalOrientation : unsigned char
        {
            VO_Top,
            VO_Center,
            VO_Bottom,
            Num_Vertical_Orientations
        };

        EType type;
        EHorizontalOrientation horizontal_orientation;
        EVerticalOrientation vertical_orientation;
        float top;
        float left;
        float border;
        float separator_size;
        float gap_size;
        float text_size;
        float image_width;
        float image_height;
        float scale;

        float width;
        float height;
        bool dirty;

        Layout();
    };

private: 

    EType m_type;
    std::string m_name;
    bool m_enabled;
    Layout m_layout;     
    GLToolbarItem *m_item={nullptr};
    struct MouseCapture
    {
        bool left;
        bool middle;
        bool right;
        GLCanvas3D* parent;

        MouseCapture() { reset(); }

        bool any() const { return left || middle || right; }
        void reset() { left = middle = right = false; parent = nullptr; }
    };

    MouseCapture m_mouse_capture;
    int m_pressed_toggable_id;
    bool m_horizontal_expand{false};//Whether to pave to the right in horizontal layout mode

public:
    GLToolbar(EType type, const std::string& name);
    ~GLToolbar();

	bool init_item(const GLToolbarItem::Data& data, GLToolbarItem::EType type = GLToolbarItem::Action);
	bool is_valid() { return m_item != nullptr; }

    GLToolbarItem* get_item(){ return m_item; }

    Layout::EType get_layout_type() const;
    void set_layout_type(Layout::EType type);

    Layout::EHorizontalOrientation get_horizontal_orientation() const { return m_layout.horizontal_orientation; }
    void set_horizontal_orientation(Layout::EHorizontalOrientation orientation) { m_layout.horizontal_orientation = orientation; }
    Layout::EVerticalOrientation get_vertical_orientation() const { return m_layout.vertical_orientation; }
    void set_vertical_orientation(Layout::EVerticalOrientation orientation) { m_layout.vertical_orientation = orientation; }

    void set_position(float top, float left);
    void set_border(float border);
    void set_scale(float scale);

	bool is_enabled() const { return m_enabled; }
	void set_enabled(bool enable) { m_enabled = enable; }
     
    float get_width();
    float get_height();
    bool update_items_state(){ return true;}  

    void render(const GLCanvas3D& parent,GLToolbarItem::EType type = GLToolbarItem::Action);
    bool on_mouse(wxMouseEvent& evt, GLCanvas3D& parent);


private:

    void calc_layout();
    void do_action(GLToolbarItem::EActionType type, int item_id, GLCanvas3D& parent, bool check_hover);

    float get_width_horizontal() const;
    float get_width_vertical() const;
    float get_height_horizontal() const;
    float get_height_vertical() const;

    void update_hover_state(const Vec2d& mouse_pos, GLCanvas3D& parent);
    void update_hover_state_horizontal(const Vec2d& mouse_pos, GLCanvas3D& parent);
    void update_hover_state_vertical(const Vec2d& mouse_pos, GLCanvas3D& parent);

    // returns the id of the item under the given mouse position or -1 if none
    bool contains_mouse(const Vec2d& mouse_pos, const GLCanvas3D& parent) const;
    bool contains_mouse_horizontal(const Vec2d& mouse_pos, const GLCanvas3D& parent) const;
    bool contains_mouse_vertical(const Vec2d& mouse_pos, const GLCanvas3D& parent) const;

    void render_background(float left, float top, float right, float bottom, float border_w, float border_h) const;
    void render_horizontal(const GLCanvas3D &parent, GLToolbarItem::EType type);
    void render_vertical(const GLCanvas3D& parent);
};
}
} // namespace GUI
} // namespace Slic3r

#endif // slic3r_GLToolbar_hpp_
