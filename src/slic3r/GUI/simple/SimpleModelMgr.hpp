#pragma once

#include <map>
#include <string>
#include "libslic3r/Format/bbs_3mf.hpp"
#include "libslic3r/Model.hpp"

namespace Slic3r { namespace GUI {

class GLCanvas3D;

class SimpleModelMgr
{
public:
    static SimpleModelMgr& instance();

    SimpleModelMgr(const SimpleModelMgr&) = delete;
    SimpleModelMgr& operator=(const SimpleModelMgr&) = delete;
    SimpleModelMgr(SimpleModelMgr&&) = delete;
    SimpleModelMgr& operator=(SimpleModelMgr&&) = delete;

    void mark_3mf_plate_objects(const PlateDataPtrs& plate_data, const Slic3r::Model& model);
    void center_3mf_objects_to_plate();

    void reset();

private:
    SimpleModelMgr() = default;

    std::vector<std::vector<int>> m_3mf_ori_plate_objects;
    bool m_need_reposition = false;
};

}} // namespace Slic3r::GUI

