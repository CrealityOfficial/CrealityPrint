#include "ArrangeJob.hpp"

#include "libslic3r/BuildVolume.hpp"
#include "libslic3r/SVG.hpp"
#include "libslic3r/MTUtils.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/ModelArrange.hpp"
#include "libslic3r/ModelInstance.hpp"

#include "slic3r/GUI/PartPlate.hpp"
#include "slic3r/GUI/GLCanvas3D.hpp"
#include "slic3r/GUI/GUI.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/NotificationManager.hpp"
#include "slic3r/GUI/format.hpp"
#include "slic3r/GUI/GUI_ObjectList.hpp"
#include "slic3r/GUI/simple/MCPChatPanel.hpp"

#include "libnest2d/common.hpp"

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#define SAVE_ARRANGE_POLY 0

namespace Slic3r { namespace GUI {
    using ArrangePolygon = arrangement::ArrangePolygon;

// Cache the wti info
class WipeTower: public GLCanvas3D::WipeTowerInfo {
public:
    explicit WipeTower(const GLCanvas3D::WipeTowerInfo &wti)
        : GLCanvas3D::WipeTowerInfo(wti)
    {}

    explicit WipeTower(GLCanvas3D::WipeTowerInfo &&wti)
        : GLCanvas3D::WipeTowerInfo(std::move(wti))
    {}

    void apply_arrange_result(const Vec2d& tr, double rotation, int item_id)
    {
        m_pos = unscaled(tr); m_rotation = rotation;
        apply_wipe_tower();
    }

    ArrangePolygon get_arrange_polygon() const
    {
        Polygon ap({
            {scaled(m_bb.min)},
            {scaled(m_bb.max.x()), scaled(m_bb.min.y())},
            {scaled(m_bb.max)},
            {scaled(m_bb.min.x()), scaled(m_bb.max.y())}
            });

        ArrangePolygon ret;
        ret.poly.contour = std::move(ap);
        ret.translation  = scaled(m_pos);
        ret.rotation     = m_rotation;
        //BBS
        ret.name = "WipeTower";
        ret.is_virt_object = true;
        ret.is_wipe_tower = true;
        ++ret.priority;

        BOOST_LOG_TRIVIAL(debug) << " arrange: wipe tower info:" << m_bb << ", m_pos: " << m_pos.transpose();

        return ret;
    }
};

// BBS: add partplate logic
static WipeTower get_wipe_tower(const Plater &plater, int plate_idx)
{
    return WipeTower{plater.canvas3D()->get_wipe_tower_info(plate_idx)};
}

arrangement::ArrangePolygon get_wipetower_arrange_poly(WipeTower* tower)
{
    ArrangePolygon ap = tower->get_arrange_polygon();
    ap.bed_idx = 0;
    ap.setter = NULL; // do not move wipe tower
    return ap;
}

void ArrangeJob::clear_input()
{
    const Model &model = m_plater->model();

    size_t count = 0, cunprint = 0; // To know how much space to reserve
    for (auto obj : model.objects)
        for (auto mi : obj->instances)
            mi->printable ? count++ : cunprint++;

    params.nonprefered_regions.clear();
    m_selected.clear();
    m_unselected.clear();
    m_unprintable.clear();
    m_unprintable_instances.clear();
    m_locked.clear();
    m_unarranged.clear();
    m_move_top_left_ids.clear();
    m_instance_id_to_instance.clear();
    m_uncompatible_plates.clear();
    m_selected.reserve(count + 1 /* for optional wti */);
    m_unselected.reserve(count + 1 /* for optional wti */);
    m_unprintable.reserve(cunprint /* for optional wti */);
    m_unprintable_instances.reserve(cunprint);
    m_locked.reserve(count + 1 /* for optional wti */);
    current_plate_index = 0;
}

ArrangePolygon ArrangeJob::prepare_arrange_polygon(void* model_instance)
{
    ModelInstance* instance = (ModelInstance*)model_instance;
    const Slic3r::DynamicPrintConfig& config = wxGetApp().preset_bundle->full_config();
    return get_instance_arrange_poly(instance, config);
}

void ArrangeJob::prepare_selected(bool consider_lock) {
    PartPlateList& plate_list = m_plater->get_partplate_list();

    clear_input();

    Model& model = m_plater->model();
    bool selected_is_locked = false;
    //BBS: remove logic for unselected object
    //double stride = bed_stride_x(m_plater);

    PartPlate* cur_plate = plate_list.get_curr_plate();
    current_plate_index = plate_list.get_curr_plate_index();

    std::vector<const Selection::InstanceIdxsList*>
        obj_sel(model.objects.size(), nullptr);

    for (auto& s : m_plater->get_selection().get_content())
        if (s.first < int(obj_sel.size()))
            obj_sel[size_t(s.first)] = &s.second;

    // Go through the objects and check if inside the selection
    for (size_t oidx = 0; oidx < model.objects.size(); ++oidx) {
        const Selection::InstanceIdxsList* instlist = obj_sel[oidx];
        ModelObject* mo = model.objects[oidx];

        std::vector<bool> inst_sel(mo->instances.size(), false);

        if (instlist)
            for (auto inst_id : *instlist)
                inst_sel[size_t(inst_id)] = true;

        for (size_t i = 0; i < inst_sel.size(); ++i) {

            bool in_plate = cur_plate->contain_instance(oidx, i) || cur_plate->intersect_instance(oidx, i);
            if(!inst_sel[i] && !in_plate) {
                continue;
            }

#if AUTO_CONVERT_3MF
            if (!instlist)
                continue;
#endif

            ModelInstance* mi = mo->instances[i];
            ArrangePolygon&& ap = prepare_arrange_polygon(mo->instances[i]);

            bool locked = false;
            if(consider_lock) {
                //BBS: partplate_list preprocess
                //remove the locked plate's instances, neither in selected, nor in un-selected
                locked = plate_list.preprocess_arrange_polygon(oidx, i, ap, inst_sel[i]);
            }
            
            if (!locked) {
                bool is_printable = mo->instances[i]->printable;
                ArrangePolygons& cont = is_printable ?
                    (inst_sel[i] ? m_selected :
                        m_unselected) :
                    m_unprintable;

                ap.itemid = cont.size();

                // if do selection layout, the selected arrange_poly's setter callback need to be reset to deal with  arrange failing situation
                if (m_plater->get_prepare_state() == Job::JobPrepareState::PREPARE_STATE_EXTRA) {

                    ap.instance_id = cont.size(); // use instance_id to for [arrange selected] logic, because the itemid can be changed by the arrange and used for other logic

                    if (is_printable && inst_sel[i]) {
                        m_instance_id_to_instance[ap.instance_id] = mo->instances[i];
                    }

                    if(!is_printable && inst_sel[i]) {
                        m_unprintable_instances.emplace_back(mo->instances[i]);
                    }
                }

                cont.emplace_back(std::move(ap));
            }
            else {
                //skip this object due to be locked in plate
                ap.itemid = m_locked.size();
                m_locked.emplace_back(std::move(ap));
                if (inst_sel[i])
                    selected_is_locked = true;
                BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << boost::format(": skip locked instance, obj_id %1%, instance_id %2%, name %3%") % oidx % i % mo->name;
            }
        }
    }


    // If the selection was empty arrange everything
    //if (m_selected.empty()) m_selected.swap(m_unselected);
    if (m_selected.empty()) {
        if (!selected_is_locked)
            m_selected.swap(m_unselected);
        else {
            m_plater->get_notification_manager()->push_notification(NotificationType::BBLPlateInfo,
                NotificationManager::NotificationLevel::WarningNotificationLevel, into_u8(_L("All the selected objects are on the locked plate,\nWe can not do auto-arrange on these objects.")));
            }
        }

    if (m_plater->get_prepare_state() == Job::JobPrepareState::PREPARE_STATE_EXTRA) {
            // adjust some offsets between the current plate and the first plate(start point located at (0,0))
            PartPlateList& plate_list       = m_plater->get_partplate_list();
            PartPlate*     plate0           = plate_list.get_plate(0);
            Vec3d          cur_plate_origin = plate_list.get_current_plate_origin();
            Vec3d          plate_offset     = cur_plate_origin - plate0->get_origin();
            for (auto& polygon : m_unselected) {
                polygon.translation.x() -= scaled<double>(plate_offset.x());
                polygon.translation.y() -= scaled<double>(plate_offset.y());
            }

    }

    prepare_wipe_tower();

    if (m_plater->get_prepare_state() == Job::JobPrepareState::PREPARE_STATE_EXTRA) {
        // this param is set to false when arrange selected(if have objects contained in the plate)
        params.do_final_align = m_unselected.empty();
    }


    // The strides have to be removed from the fixed items. For the
    // arrangeable (selected) items bed_idx is ignored and the
    // translation is irrelevant.
    //BBS: remove logic for unselected object
    //for (auto &p : m_unselected) p.translation(X) -= p.bed_idx * stride;
}

void ArrangeJob::prepare_all() {
    clear_input();

    PartPlateList& plate_list = m_plater->get_partplate_list();

    // if only one plate exist, skip this uncompatible plate check
    if(plate_list.get_plate_count() > 1) {
        for (size_t i = 0; i < plate_list.get_plate_count(); i++) {
            PartPlate* plate = plate_list.get_plate(i);
            bool same_as_global_print_seq = true;
            plate->get_real_print_seq(&same_as_global_print_seq);
            if (plate->is_locked() == false && !same_as_global_print_seq) {
                plate->lock(true);
                m_uncompatible_plates.push_back(i);
            }
        }
    }

    Model &model = m_plater->model();
    bool selected_is_locked = false;

    // Go through the objects and check if inside the selection
    for (size_t oidx = 0; oidx < model.objects.size(); ++oidx) {
        ModelObject *mo = model.objects[oidx];

        for (size_t i = 0; i < mo->instances.size(); ++i) {
            ModelInstance * mi = mo->instances[i];
            ArrangePolygon&& ap = prepare_arrange_polygon(mo->instances[i]);
            //BBS: partplate_list preprocess
            //remove the locked plate's instances, neither in selected, nor in un-selected
            bool locked = plate_list.preprocess_arrange_polygon(oidx, i, ap, true);
            if (!locked)
            {
                ArrangePolygons& cont = mo->instances[i]->printable ? m_selected :m_unprintable;

                ap.itemid = cont.size();

                // fix: the very big size object(larger then bed size), need to move to top left area of the first plate
                ap.instance_id = cont.size(); // use instance_id to for [arrange selected] logic, because the itemid can be changed by the arrange and used for other logic
                if (mo->instances[i]->printable) {
                    m_instance_id_to_instance[ap.instance_id] = mo->instances[i];
                }
                else
                {
                    m_unprintable_instances.emplace_back(mo->instances[i]);
                }

                cont.emplace_back(std::move(ap));
            }
            else
            {
                //skip this object due to be locked in plate
                ap.itemid = m_locked.size();
                m_locked.emplace_back(std::move(ap));
                selected_is_locked = true;
                BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << boost::format(": skip locked instance, obj_id %1%, instance_id %2%") % oidx % i;
            }
        }
    }


    // If the selection was empty arrange everything
    //if (m_selected.empty()) m_selected.swap(m_unselected);
    if (m_selected.empty()) {
        if (!selected_is_locked) {
            m_plater->get_notification_manager()->push_notification(NotificationType::BBLPlateInfo,
                NotificationManager::NotificationLevel::WarningNotificationLevel, into_u8(_L("No arrangable objects are selected.")));
        }
        else {
            m_plater->get_notification_manager()->push_notification(NotificationType::BBLPlateInfo,
                NotificationManager::NotificationLevel::WarningNotificationLevel, into_u8(_L("All the selected objects are on the locked plate,\nWe can not do auto-arrange on these objects.")));
        }
    }
    if (!m_uncompatible_plates.empty()) {
        auto msg = _L("The following plates are skipped due to different arranging settings from global:");
        for (int i : m_uncompatible_plates) { msg += "\n"+_L("Plate") + " " + std::to_string(i + 1);
        }
        m_plater->get_notification_manager()->push_notification(NotificationType::BBLPlateInfo,
                       NotificationManager::NotificationLevel::WarningNotificationLevel, into_u8(msg));
    }
    prepare_wipe_tower();

    // add the virtual object into unselect list if has
    plate_list.preprocess_exclude_areas(m_unselected, MAX_NUM_PLATES);
}

arrangement::ArrangePolygon estimate_wipe_tower_info(int plate_index, std::set<int>& extruder_ids)
{
    PartPlateList& ppl = wxGetApp().plater()->get_partplate_list();
    const auto full_config = wxGetApp().preset_bundle->full_config();
    int plate_count = ppl.get_plate_count();
    int plate_index_valid = std::min(plate_index, plate_count - 1);

    // we have to estimate the depth using the extruder number of all plates
    int extruder_size = extruder_ids.size();

    auto arrange_poly = ppl.get_plate(plate_index_valid)->estimate_wipe_tower_polygon(full_config, plate_index, extruder_size);
    arrange_poly.bed_idx = plate_index;
    return arrange_poly;
}

// Prepare wipe tower state for arrange. The logic covers:
// 1. Cases where a wipe tower is not needed.
// 2. Cases where a wipe tower must be estimated for arrange.
void ArrangeJob::prepare_wipe_tower()
{
    bool need_wipe_tower = false;

    // if wipe tower is explicitly disabled, no need to estimate
    DynamicPrintConfig& current_config = wxGetApp().preset_bundle->prints.get_edited_preset().config;
    auto                op = current_config.option("enable_prime_tower");
    bool enable_prime_tower = op && op->getBool();
    if (!enable_prime_tower || params.is_seq_print) return;

    bool smooth_timelapse = false;
    auto sop = current_config.option("timelapse_type");
    if (sop) { smooth_timelapse = sop->getInt() == TimelapseType::tlSmooth; }
    if (smooth_timelapse) { need_wipe_tower = true; }

    {
        PartPlateList& ppl = wxGetApp().plater()->get_partplate_list();
        for (int plate_id = 0; plate_id < ppl.get_plate_count(); ++plate_id) {
            if (ppl.get_plate(plate_id)->get_extruders(true).size() > 1) {
                need_wipe_tower = true;
                BOOST_LOG_TRIVIAL(info) << "arrange: need wipe tower because plate " << plate_id << " uses multiple role extruders";
                break;
            }
        }
    }

    // estimate if we need wipe tower for all plates:
    // need wipe tower if some object has multiple extruders (has paint-on colors or support material)
    for (const auto& item : m_selected) {
        std::set<int> obj_extruders;
        obj_extruders.insert(item.extrude_ids.begin(), item.extrude_ids.end());
        if (obj_extruders.size() > 1) {
            need_wipe_tower = true;
            BOOST_LOG_TRIVIAL(info) << "arrange: need wipe tower because object " << item.name << " has multiple extruders (has paint-on colors)";
            break;
        }
    }

    // if multile extruders have same bed temp, we need wipe tower
    // If multiple extruders share the same bed temperature, keep the wipe tower.
    if (params.allow_multi_materials_on_same_plate) {
        std::map<int, std::set<int>> bedTemp2extruderIds;
        for (const auto& item : m_selected)
            for (auto id : item.extrude_ids) { bedTemp2extruderIds[item.bed_temp].insert(id); }
        for (const auto& be : bedTemp2extruderIds) {
            if (be.second.size() > 1) {
                need_wipe_tower = true;
                BOOST_LOG_TRIVIAL(info) << "arrange: need wipe tower because allow_multi_materials_on_same_plate=true and we have multiple extruders of same type";
                break;
            }
        }
    }
    BOOST_LOG_TRIVIAL(info) << "arrange: need_wipe_tower=" << need_wipe_tower;


    ArrangePolygon    wipe_tower_ap;
    wipe_tower_ap.name = "WipeTower";
    wipe_tower_ap.is_virt_object = true;
    wipe_tower_ap.is_wipe_tower = true;
    const GLCanvas3D* canvas3D = static_cast<const GLCanvas3D*>(m_plater->canvas3D());

    std::set<int> extruder_ids;
    PartPlateList& ppl = wxGetApp().plater()->get_partplate_list();
    int plate_count = ppl.get_plate_count();
    if (!only_on_partplate) {
        extruder_ids = ppl.get_extruders(true);
    }

    int bedid_unlocked = 0;
    for (int bedid = 0; bedid < MAX_NUM_PLATES; bedid++) {
        int plate_index_valid = std::min(bedid, plate_count - 1);
        PartPlate* pl = ppl.get_plate(plate_index_valid);
        if(bedid<plate_count && pl->is_locked())
            continue;
        if (auto wti = get_wipe_tower(*m_plater, bedid)) {
            // wipe tower is already there
            wipe_tower_ap = get_wipetower_arrange_poly(&wti);
            //wipe_tower_ap.bed_idx = bedid_unlocked;
            wipe_tower_ap.bed_idx = 0;
            m_unselected.emplace_back(wipe_tower_ap);
        }
        else if (need_wipe_tower) {
            if (only_on_partplate) {
                auto plate_extruders = pl->get_extruders(true);
                extruder_ids.clear();
                extruder_ids.insert(plate_extruders.begin(), plate_extruders.end());
            }
            wipe_tower_ap = estimate_wipe_tower_info(bedid, extruder_ids);
            wipe_tower_ap.bed_idx = bedid_unlocked;
            m_unselected.emplace_back(wipe_tower_ap);
        }
        bedid_unlocked++;
    }
}

#if AUTO_CONVERT_3MF
void ArrangeJob::prepare_wipe_tower_ex(int plate_index)
{
    bool need_wipe_tower = false;

    // if wipe tower is explicitly disabled, no need to estimate
    DynamicPrintConfig& current_config     = wxGetApp().preset_bundle->prints.get_edited_preset().config;
    auto                op                 = current_config.option("enable_prime_tower");
    bool                enable_prime_tower = op && op->getBool();
    if (!enable_prime_tower || params.is_seq_print)
        return;

    bool smooth_timelapse = false;
    auto sop              = current_config.option("timelapse_type");
    if (sop) {
        smooth_timelapse = sop->getInt() == TimelapseType::tlSmooth;
    }
    if (smooth_timelapse) {
        need_wipe_tower = true;
    }

    {
        PartPlateList& ppl = wxGetApp().plater()->get_partplate_list();
        if (plate_index >= 0 && plate_index < ppl.get_plate_count() && ppl.get_plate(plate_index)->get_extruders(true).size() > 1) {
            need_wipe_tower = true;
            BOOST_LOG_TRIVIAL(info) << "arrange: need wipe tower because plate " << plate_index << " uses multiple role extruders";
        }
    }

    // estimate if we need wipe tower for all plates:
    // need wipe tower if some object has multiple extruders (has paint-on colors or support material)
    for (const auto& item : m_selected) {
        std::set<int> obj_extruders;
        obj_extruders.insert(item.extrude_ids.begin(), item.extrude_ids.end());
        if (obj_extruders.size() > 1) {
            need_wipe_tower = true;
            BOOST_LOG_TRIVIAL(info) << "arrange: need wipe tower because object " << item.name
                                    << " has multiple extruders (has paint-on colors)";
            break;
        }
    }

    // if multile extruders have same bed temp, we need wipe tower
    // If multiple extruders share the same bed temperature, keep the wipe tower.
    if (params.allow_multi_materials_on_same_plate) {
        std::map<int, std::set<int>> bedTemp2extruderIds;
        for (const auto& item : m_selected)
            for (auto id : item.extrude_ids) {
                bedTemp2extruderIds[item.bed_temp].insert(id);
            }
        for (const auto& be : bedTemp2extruderIds) {
            if (be.second.size() > 1) {
                need_wipe_tower = true;
                BOOST_LOG_TRIVIAL(info) << "arrange: need wipe tower because allow_multi_materials_on_same_plate=true and we have multiple "
                                           "extruders of same type";
                break;
            }
        }
    }
    BOOST_LOG_TRIVIAL(info) << "arrange: need_wipe_tower=" << need_wipe_tower;

    ArrangePolygon wipe_tower_ap;
    wipe_tower_ap.name           = "WipeTower";
    wipe_tower_ap.is_virt_object = true;
    wipe_tower_ap.is_wipe_tower  = true;
    const GLCanvas3D* canvas3D   = static_cast<const GLCanvas3D*>(m_plater->canvas3D());

    std::set<int>  extruder_ids;
    PartPlateList& ppl         = wxGetApp().plater()->get_partplate_list();
    int            plate_count = ppl.get_plate_count();
    if (!only_on_partplate) {
        extruder_ids = ppl.get_extruders(true);
    }

    int bedid_unlocked = 0;
    for (int bedid = 0; bedid < MAX_NUM_PLATES; bedid++) {
        if (plate_index != bedid)
            continue;

        int        plate_index_valid = std::min(bedid, plate_count - 1);
        PartPlate* pl                = ppl.get_plate(plate_index_valid);
        if (bedid < plate_count && pl->is_locked())
            continue;
        if (auto wti = get_wipe_tower(*m_plater, bedid)) {
            // wipe tower is already there
            wipe_tower_ap = get_wipetower_arrange_poly(&wti);
            // wipe_tower_ap.bed_idx = bedid_unlocked;
            wipe_tower_ap.bed_idx = 0;
            m_unselected.emplace_back(wipe_tower_ap);
        } else if (need_wipe_tower) {
            if (only_on_partplate) {
                auto plate_extruders = pl->get_extruders(true);
                extruder_ids.clear();
                extruder_ids.insert(plate_extruders.begin(), plate_extruders.end());
            }
            wipe_tower_ap         = estimate_wipe_tower_info(bedid, extruder_ids);
            wipe_tower_ap.bed_idx = bedid_unlocked;
            m_unselected.emplace_back(wipe_tower_ap);
        }
        bedid_unlocked++;
    }
}
#endif

//BBS: prepare current part plate for arranging
void ArrangeJob::prepare_partplate() {
    clear_input();

    PartPlateList& plate_list = m_plater->get_partplate_list();
    PartPlate* plate = plate_list.get_curr_plate();
    current_plate_index = plate_list.get_curr_plate_index();
    assert(plate != nullptr);

    if (plate->empty())
    {
        //no instances on this plate
        BOOST_LOG_TRIVIAL(info) << __FUNCTION__ << boost::format(": no instances in current plate!");

        return;
    }

    if (plate->is_locked()) {
        m_plater->get_notification_manager()->push_notification(NotificationType::BBLPlateInfo,
            NotificationManager::NotificationLevel::WarningNotificationLevel, into_u8(_L("This plate is locked,\nWe can not do auto-arrange on this plate.")));
        return;
    }

    Model& model = m_plater->model();

    // Go through the objects and check if inside the selection
    for (size_t oidx = 0; oidx < model.objects.size(); ++oidx)
    {
        ModelObject* mo = model.objects[oidx];
        for (size_t inst_idx = 0; inst_idx < mo->instances.size(); ++inst_idx)
        {
            bool             in_plate = plate->contain_instance(oidx, inst_idx) || plate->intersect_instance(oidx, inst_idx);
            ArrangePolygon&& ap = prepare_arrange_polygon(mo->instances[inst_idx]);

            bool is_printable = mo->instances[inst_idx]->printable;
            ArrangePolygons& cont = is_printable ?
                (in_plate ? m_selected : m_unselected) :
                m_unprintable;
            bool locked = plate_list.preprocess_arrange_polygon_other_locked(oidx, inst_idx, ap, in_plate);
            if (!locked)
            {
                ap.itemid = cont.size();
                ap.instance_id = cont.size();
                if(is_printable)
                    m_instance_id_to_instance[ap.instance_id] = mo->instances[inst_idx];  
                else
                    m_unprintable_instances.emplace_back(mo->instances[inst_idx]);

                cont.emplace_back(std::move(ap));
            }
            else
            {
                //skip this object due to be not in current plate, treated as locked
                ap.itemid = m_locked.size();
                m_locked.emplace_back(std::move(ap));
                //BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << boost::format(": skip locked instance, obj_id %1%, name %2%") % oidx % mo->name;
            }
        }
    }

    // BBS
    if (auto wti = get_wipe_tower(*m_plater, current_plate_index)) {
        ArrangePolygon&& ap = get_wipetower_arrange_poly(&wti);
        m_unselected.emplace_back(std::move(ap));
    }

    // add the virtual object into unselect list if has
    plate_list.preprocess_exclude_areas(m_unselected, current_plate_index + 1);
}

//BBS: add partplate logic
void ArrangeJob::prepare()
{
    m_plater->get_notification_manager()->cleanup_arrange_notifications();

    m_plater->get_notification_manager()->push_notification(NotificationType::ArrangeOngoing,
        NotificationManager::NotificationLevel::RegularNotificationLevel, _u8L("Arranging..."));
    m_plater->get_notification_manager()->bbl_close_plateinfo_notification();

    params = init_arrange_params(m_plater);

    //BBS update extruder params and speed table before arranging
    const Slic3r::DynamicPrintConfig& config = wxGetApp().preset_bundle->full_config();
    auto& print = wxGetApp().plater()->get_partplate_list().get_current_fff_print();
    auto print_config = print.config();
    int numExtruders = wxGetApp().preset_bundle->filament_presets.size();

    Model::setExtruderParams(config, numExtruders);
    Model::setPrintSpeedTable(config, print_config);

    int state = m_plater->get_prepare_state();
    if (state == Job::JobPrepareState::PREPARE_STATE_DEFAULT) {
        only_on_partplate = false;
        prepare_all();
    }
    else if (state == Job::JobPrepareState::PREPARE_STATE_MENU) {
        only_on_partplate = true;   // only arrange items on current plate
        prepare_partplate();
    }
    else if (state == Job::JobPrepareState::PREPARE_STATE_EXTRA) {
        only_on_partplate = true;
#if AUTO_CONVERT_3MF
        prepare_auto_convert_3mf_selected(false);
#else
        prepare_selected(false);
#endif // AUTO_CONVERT_3MF
    }

#if SAVE_ARRANGE_POLY
    if (1)
    { // subtract excluded region and get a polygon bed
        auto& print = wxGetApp().plater()->get_partplate_list().get_current_fff_print();
        auto print_config = print.config();
        bed_poly.points = get_bed_shape(*m_plater->config());
        Pointfs excluse_area_points = print_config.bed_exclude_area.values;
        Polygons exclude_polys;
        Polygon exclude_poly;
        for (int i = 0; i < excluse_area_points.size(); i++) {
            auto pt = excluse_area_points[i];
            exclude_poly.points.emplace_back(scale_(pt.x()), scale_(pt.y()));
            if (i % 4 == 3) {  // exclude areas are always rectangle
                exclude_polys.push_back(exclude_poly);
                exclude_poly.points.clear();
            }
        }
        bed_poly = diff({ bed_poly }, exclude_polys)[0];
    }

    BoundingBox bbox = bed_poly.bounding_box();
    Point center = bbox.center();
    auto polys_to_draw = m_selected;
    for (auto it = polys_to_draw.begin(); it != polys_to_draw.end(); it++) {
        it->poly.translate(center);
        bbox.merge(it->poly);
    }
    SVG svg("SVG/arrange_poly.svg", bbox);
    if (svg.is_opened()) {
        svg.draw_outline(bed_poly);
        //svg.draw_grid(bbox, "gray", scale_(0.05));
        std::vector<std::string> color_array = { "red","black","yellow","gree","blue" };
        for (auto it = polys_to_draw.begin(); it != polys_to_draw.end(); it++) {
            std::string color = color_array[(it - polys_to_draw.begin()) % color_array.size()];
            svg.add_comment(it->name);
            svg.draw_text(get_extents(it->poly).min, it->name.c_str(), color.c_str());
            svg.draw_outline(it->poly, color);
        }
    }
#endif

    check_unprintable();
}

void ArrangeJob::check_unprintable()
{
    for (auto it = m_selected.begin(); it != m_selected.end();) {
        if (it->poly.area() < 0.001 || it->height>params.printable_height)
        {
#if SAVE_ARRANGE_POLY
            SVG svg(data_dir() + "/SVG/arrange_unprintable_"+it->name+".svg", get_extents(it->poly));
            if (svg.is_opened())
                svg.draw_outline(it->poly);
#endif
            if (it->poly.area() < 0.001) {
                auto msg = (boost::format(
                    _utf8("Object %s has zero size and can't be arranged."))
                    % _utf8(it->name)).str();
                m_plater->get_notification_manager()->push_notification(NotificationType::BBLPlateInfo,
                    NotificationManager::NotificationLevel::WarningNotificationLevel, msg);
            }
            m_unprintable.push_back(*it);
            it = m_selected.erase(it);
        }
        else
            it++;
    }
}

void ArrangeJob::process(Ctl &ctl)
{
    static const auto arrangestr = _u8L("Arranging");
   // ctl.update_status(0, arrangestr);
    ctl.call_on_main_thread([this]{ prepare(); }).wait();;

    auto & partplate_list = m_plater->get_partplate_list();

    const Slic3r::DynamicPrintConfig& global_config = wxGetApp().preset_bundle->full_config();
    PresetBundle* preset_bundle = wxGetApp().preset_bundle;
    const bool is_bbl = wxGetApp().preset_bundle->is_bbl_vendor();
    if (is_bbl && params.avoid_extrusion_cali_region && global_config.opt_bool("scan_first_layer"))
        partplate_list.preprocess_nonprefered_areas(m_unselected, MAX_NUM_PLATES);

    update_arrange_params(params, m_plater->config(), m_selected);
    update_selected_items_inflation(m_selected, m_plater->config(), params);
    update_unselected_items_inflation(m_unselected, m_plater->config(), params);
    update_selected_items_axis_align(m_selected, m_plater->config(), params);

    Points      bedpts = get_shrink_bedpts(m_plater->config(),params);

    partplate_list.preprocess_exclude_areas(params.excluded_regions, 1, scale_(1));

    BOOST_LOG_TRIVIAL(debug) << "arrange bedpts:" << bedpts[0].transpose() << ", " << bedpts[1].transpose() << ", " << bedpts[2].transpose() << ", " << bedpts[3].transpose();

    params.stopcondition = [&ctl]() { return ctl.was_canceled(); };

    params.progressind = [this, &ctl](unsigned num_finished, std::string str = "") {
   //     ctl.update_status(num_finished * 100 / status_range(), _u8L("Arranging") + str);
    };

    {
        BOOST_LOG_TRIVIAL(warning)<< "Arrange full params: "<< params.to_json();
        BOOST_LOG_TRIVIAL(info) << boost::format("arrange: items selected before arranging: %1%") % m_selected.size();
        for (auto selected : m_selected) {
            BOOST_LOG_TRIVIAL(debug) << selected.name << ", extruder: " << selected.extrude_ids.back() << ", bed: " << selected.bed_idx << ", filemant_type:" << selected.filament_temp_type
                << ", trans: " << selected.translation.transpose();
        }
        BOOST_LOG_TRIVIAL(debug) << "arrange: items unselected before arrange: " << m_unselected.size();
        for (auto item : m_unselected)
            BOOST_LOG_TRIVIAL(debug) << item.name << ", bed: " << item.bed_idx << ", trans: " << item.translation.transpose()
            <<", bbox:"<<get_extents(item.poly).min.transpose()<<","<<get_extents(item.poly).max.transpose();
    }
    
    if (wxGetApp().preset_bundle->machine_is_belt())
        arrangement::cr30_arrange(m_selected, m_unselected, bedpts);
    else
        arrangement::arrange(m_selected, m_unselected, bedpts, params);

    // sort by item id
    std::sort(m_selected.begin(), m_selected.end(), [](auto a, auto b) {return a.itemid < b.itemid; });
    {
        BOOST_LOG_TRIVIAL(info) << boost::format("arrange: items selected after arranging: %1%") % m_selected.size();
        for (auto selected : m_selected)
            BOOST_LOG_TRIVIAL(debug) << selected.name << ", extruder: " << selected.extrude_ids.back() << ", bed: " << selected.bed_idx
                                     << ", bed_temp: " << selected.first_bed_temp << ", print_temp: " << selected.print_temp
                                     << ", trans: " << unscale<double>(selected.translation(X)) << ","<< unscale<double>(selected.translation(Y));
        BOOST_LOG_TRIVIAL(debug) << "arrange: items unselected after arrange: "<< m_unselected.size();
        for (auto item : m_unselected)
            BOOST_LOG_TRIVIAL(debug) << item.name << ", bed: " << item.bed_idx << ", trans: " << item.translation.transpose();
    }

    // put unpackable items to m_unprintable so they goes outside
    bool we_have_unpackable_items = false;
    for (auto item : m_selected) {
        if (item.bed_idx < 0) {
            //BBS: already processed in m_selected
            //m_unprintable.push_back(std::move(item));
            we_have_unpackable_items = true;
        }
    }

    // finalize just here.
  /*  ctl.update_status(100,
        ctl.was_canceled() ? _u8L("Arranging canceled.") :
        we_have_unpackable_items ? _u8L("Arranging is done but there are unpacked items. Reduce spacing and try again.") : _u8L("Arranging done."));*/

    ctl.call_on_main_thread([this]{
        m_plater->get_notification_manager()->cleanup_arrange_notifications();
    });
}

ArrangeJob::ArrangeJob() : m_plater{wxGetApp().plater()} { }

static std::string concat_strings(const std::set<std::string> &strings,
                                  const std::string &delim = "\n")
{
    return std::accumulate(
        strings.begin(), strings.end(), std::string(""),
        [delim](const std::string &s, const std::string &name) {
            return s + name + delim;
        });
}

void ArrangeJob::finalize(bool canceled, std::exception_ptr &eptr) {
    auto report_arrange_completion = [&](bool success,
                                         const std::string& completion_message,
                                         const nlohmann::json& details = nlohmann::json::object()) {
        if (wxGetApp().easy_mode()) {
            if (auto* panel = GetEmbeddedAIChatPanel())
                panel->CompletePendingAsyncToolCall("job:auto_arrange", success, completion_message, details);
        }
    };
    try {
        if (eptr)
            std::rethrow_exception(eptr);
    } catch (libnest2d::GeometryException &) {
        show_error(m_plater, _(L("Arrange failed. "
                                 "Found some exceptions when processing object geometries.")));
        eptr = nullptr;
        BOOST_LOG_TRIVIAL(warning) << "ArrangeJob::finalize geometry exception";
        boost::log::core::get()->flush();
    } catch (...) {
        eptr = std::current_exception();
        BOOST_LOG_TRIVIAL(warning) << "ArrangeJob::finalize unknown exception";
        boost::log::core::get()->flush();
    }

    if (canceled || eptr) {
        BOOST_LOG_TRIVIAL(warning) << "ArrangeJob::finalize early exit"
                                   << ", canceled=" << (canceled ? "true" : "false")
                                   << ", has_eptr=" << (eptr ? "true" : "false");
        boost::log::core::get()->flush();
        report_arrange_completion(
            false,
            canceled ? std::string("Arrange canceled") : std::string("Arrange failed before finalize"),
            {{"code", canceled ? "AUTO_ARRANGE_CANCELED" : "AUTO_ARRANGE_FINALIZE_FAILED"}, {"canceled", canceled}, {"has_exception", static_cast<bool>(eptr)}});
        return;
    }

    try {
        int beds = 0;

        PartPlateList& plate_list = m_plater->get_partplate_list();
        if (only_on_partplate) {
            plate_list.clear(false, false, true, current_plate_index);
        }
        else
            plate_list.clear(false, false, true, -1);

        if (only_on_partplate) {
            for (ArrangePolygon& ap : m_selected) {
                if (ap.bed_idx > 0) {
                    m_move_top_left_ids.emplace_back(ap.instance_id);
                }
            }
        }

        for (ArrangePolygon& ap : m_selected) {
            if (only_on_partplate)
                plate_list.postprocess_bed_index_for_current_plate(ap);
            else
                plate_list.postprocess_bed_index_for_selected(ap);

            beds = std::max(ap.bed_idx, beds);

            BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << boost::format(": arrange selected %4%: bed_id %1%, trans {%2%,%3%}") % ap.bed_idx % unscale<double>(ap.translation(X)) % unscale<double>(ap.translation(Y)) % ap.name;
        }

        for (ArrangePolygon& ap : m_unselected)
        {
            if (ap.is_virt_object)
                continue;

            if (!only_on_partplate)
                plate_list.postprocess_bed_index_for_unselected(ap);

            beds = std::max(ap.bed_idx, beds);
            BOOST_LOG_TRIVIAL(debug) << __FUNCTION__ << boost::format(":arrange unselected %4%: bed_id %1%, trans {%2%,%3%}") % ap.bed_idx % unscale<double>(ap.translation(X)) % unscale<double>(ap.translation(Y)) % ap.name;
        }

        for (ArrangePolygon& ap : m_locked) {
            beds = std::max(ap.bed_idx, beds);

            plate_list.postprocess_arrange_polygon(ap, false);

            ap.apply();
        }

        bool move_object_top_left = false;

        for (ArrangePolygon& ap : m_selected) {

            if ((only_on_partplate && std::find(m_move_top_left_ids.begin(), m_move_top_left_ids.end(), ap.instance_id) != m_move_top_left_ids.end()) || (ap.bed_idx == -1)) {
                auto it = m_instance_id_to_instance.find(ap.instance_id);
                if (it == m_instance_id_to_instance.end()) {
                    BOOST_LOG_TRIVIAL(warning) << "ArrangeJob::finalize missing instance for instance_id=" << ap.instance_id;
                    continue;
                }
                ModelInstance* mi = it->second;
                if (!mi) {
                    BOOST_LOG_TRIVIAL(warning) << "ArrangeJob::finalize null ModelInstance for instance_id=" << ap.instance_id;
                    continue;
                }

                PartPlate* first_plate = plate_list.get_plate(0);
                if (!first_plate) {
                    BOOST_LOG_TRIVIAL(warning) << "ArrangeJob::finalize first plate is null when moving objects to top left";
                    continue;
                }

                BoundingBoxf3 plate_bbox   = first_plate->get_build_volume();
                Vec3d   instance_bbox_size = mi->get_object()->instance_bounding_box(0).size();
                auto          offset             = mi->get_offset();
                Vec3d top_left = {plate_bbox.min.x() + instance_bbox_size.x(), plate_bbox.max.y() + instance_bbox_size.y(), offset(2)};
                mi->set_offset(top_left);

                move_object_top_left = true;

            } else {
                plate_list.postprocess_arrange_polygon(ap, true);
                ap.apply();
            }
        }

        if(m_plater->get_prepare_state() != Job::JobPrepareState::PREPARE_STATE_EXTRA) {
            for (ArrangePolygon& ap : m_unselected)
            {
                if (ap.is_virt_object)
                    continue;

                plate_list.postprocess_arrange_polygon(ap, false);

                ap.apply();
            }
        }

        if(m_unprintable.size() > 0) {
            PartPlate* first_plate = plate_list.get_plate(0);
            if (first_plate) {
                BoundingBoxf3 plate_bbox   = first_plate->get_build_volume();
                for(ModelInstance* mi : m_unprintable_instances) {
                    if(mi) {
                        Vec3d   instance_bbox_size = mi->get_object()->instance_bounding_box(0).size();
                        auto          offset             = mi->get_offset();
                        Vec3d top_left = {plate_bbox.min.x() + instance_bbox_size.x(), plate_bbox.max.y() + instance_bbox_size.y(), offset(2)};
                        mi->set_offset(top_left);
                    }
                }
            }

            m_plater->get_notification_manager()->push_notification(NotificationType::BBLPlateInfo,
                                                                    NotificationManager::NotificationLevel::WarningNotificationLevel,
                                        into_u8(_L("The UnPrintable Objects are placed above Tray 1.")));
        }

        m_plater->update();

        if (!m_unarranged.empty()) {
            std::set<std::string> names;
            for (ModelInstance *mi : m_unarranged)
                names.insert(mi->get_object()->name);

            m_plater->get_notification_manager()->push_notification(GUI::format(
                _L("Arrangement ignored the following objects which can't fit into a single bed:\n%s"),
                concat_strings(names, "\n")));
        }

        if (move_object_top_left) {
            m_plater->get_notification_manager()->push_notification(NotificationType::BBLPlateInfo,
                                                                    NotificationManager::NotificationLevel::WarningNotificationLevel,
                                        into_u8(_L("Current tray is full; Unarranged models are placed above Tray 1.")));
        }

        m_plater->get_notification_manager()->close_notification_of_type(NotificationType::ArrangeOngoing);

        if (only_on_partplate) {
            plate_list.rebuild_plates_after_arrangement(!only_on_partplate, true, current_plate_index);
        }
        else {
            plate_list.rebuild_plates_after_arrangement(!only_on_partplate, true);
        }

        if (!m_uncompatible_plates.empty()) {
            int plate_count = plate_list.get_plate_count();
            for (int i : m_uncompatible_plates) {
                if (i < 0 || i >= plate_count) {
                    BOOST_LOG_TRIVIAL(warning) << "ArrangeJob::finalize invalid uncompatible plate index i=" << i << ", plate_count=" << plate_count;
                    continue;
                }
                PartPlate* plate = plate_list.get_plate(i);
                if (!plate) {
                    BOOST_LOG_TRIVIAL(warning) << "ArrangeJob::finalize null plate for uncompatible index i=" << i;
                    continue;
                }
                plate->lock(false);
            }
        }

        m_plater->update_slicing_context_to_current_partplate();

        // FIX: rebuild_plates_after_arrangement() may reorder m_model->objects via std::sort,
        // which invalidates the obj_idx stored in GLVolumes. We must reload the 3D scene first
        // to sync GLVolumes with the new Model state, otherwise update_selections() inside
        // reload_all_plates() will access stale indices and crash.
        m_plater->get_view3D_canvas3D()->reload_scene(false);

        wxGetApp().obj_list()->reload_all_plates();

        m_plater->update();

        m_plater->m_arrange_running.store(false);

#if AUTO_CONVERT_3MF
        wxGetApp().auto_convert_3mf_mgr.on_arrange_job_finished();
#endif
        report_arrange_completion(
            true,
            "Arrange completed",
            {{"unarranged_count", m_unarranged.size()}, {"unprintable_count", m_unprintable_instances.size()}, {"current_plate_only", only_on_partplate}});
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(warning) << "ArrangeJob::finalize apply exception"
                                   << ", ex=" << ex.what();
        boost::log::core::get()->flush();
        if (m_plater) {
            show_error(m_plater, _L("An unexpected error occured"));
        }
        report_arrange_completion(
            false,
            "Arrange failed while applying results",
            {{"code", "AUTO_ARRANGE_FINALIZE_FAILED"}, {"exception", ex.what()}});
    } catch (...) {
        BOOST_LOG_TRIVIAL(warning) << "ArrangeJob::finalize apply unknown exception";
        boost::log::core::get()->flush();
        if (m_plater) {
            show_error(m_plater, _L("An unexpected error occured"));
        }
        report_arrange_completion(
            false,
            "Arrange failed while applying results",
            {{"code", "AUTO_ARRANGE_FINALIZE_FAILED"}});
    }
}

#if AUTO_CONVERT_3MF
void ArrangeJob::prepare_auto_convert_3mf_selected(bool consider_lock)
{
    PartPlateList& plate_list = m_plater->get_partplate_list();

    clear_input();

    Model& model              = m_plater->model();
    bool   selected_is_locked = false;
    // BBS: remove logic for unselected object
    // double stride = bed_stride_x(m_plater);

    PartPlate* cur_plate = plate_list.get_curr_plate();
    current_plate_index  = plate_list.get_curr_plate_index();

    std::vector<int> need_arrange_obj_loaded_ids = wxGetApp().auto_convert_3mf_mgr.get_need_arrange_object_ids(current_plate_index);

    // Go through the objects and check if inside the selection
    for (size_t oidx = 0; oidx < model.objects.size(); ++oidx) {
        ModelObject* mo = model.objects[oidx];
        if (!mo || mo->from_loaded_id < 0)
            continue;

        if (std::find(need_arrange_obj_loaded_ids.begin(), need_arrange_obj_loaded_ids.end(), mo->from_loaded_id) ==
            need_arrange_obj_loaded_ids.end())
            continue;

        std::vector<bool> inst_sel(mo->instances.size(), true);

        for (size_t i = 0; i < mo->instances.size(); ++i) {
            ModelInstance*   mi = mo->instances[i];
            ArrangePolygon&& ap = prepare_arrange_polygon(mo->instances[i]);

            bool locked = false;
            if (consider_lock) {
                // BBS: partplate_list preprocess
                // remove the locked plate's instances, neither in selected, nor in un-selected
                locked = plate_list.preprocess_arrange_polygon(oidx, i, ap, inst_sel[i]);
            }

            if (!locked) {
                ArrangePolygons& cont = mo->instances[i]->printable ? (inst_sel[i] ? m_selected : m_unselected) : m_unprintable;

                ap.itemid = cont.size();

                // if do selection layout, the selected arrange_poly's setter callback need to be reset to deal with  arrange failing situation
                if (m_plater->get_prepare_state() == Job::JobPrepareState::PREPARE_STATE_EXTRA) {
                    ap.instance_id = cont.size(); // use instance_id to for [arrange selected] logic, because the itemid can be changed by
                                                  // the arrange and used for other logic

                    if (mo->instances[i]->printable && inst_sel[i]) {
                        m_instance_id_to_instance[ap.instance_id] = mo->instances[i];
                    }
                }

                cont.emplace_back(std::move(ap));
            } else {
                // skip this object due to be locked in plate
                ap.itemid = m_locked.size();
                m_locked.emplace_back(std::move(ap));
                if (inst_sel[i])
                    selected_is_locked = true;
                BOOST_LOG_TRIVIAL(debug) << __FUNCTION__
                                         << boost::format(": skip locked instance, obj_id %1%, instance_id %2%, name %3%") % oidx % i %
                                                mo->name;
            }
        }
    }

    // If the selection was empty arrange everything
    // if (m_selected.empty()) m_selected.swap(m_unselected);
    if (m_selected.empty()) {
        if (!selected_is_locked)
            m_selected.swap(m_unselected);
        else {
            m_plater->get_notification_manager()->push_notification(
                NotificationType::BBLPlateInfo, NotificationManager::NotificationLevel::WarningNotificationLevel,
                into_u8(_L("All the selected objects are on the locked plate,\nWe can not do auto-arrange on these objects.")));
        }
    }

    if (m_plater->get_prepare_state() == Job::JobPrepareState::PREPARE_STATE_EXTRA) {
        // adjust some offsets between the current plate and the first plate(start point located at (0,0))
        PartPlateList& plate_list       = m_plater->get_partplate_list();
        PartPlate*     plate0           = plate_list.get_plate(0);
        Vec3d          cur_plate_origin = plate_list.get_current_plate_origin();
        Vec3d          plate_offset     = cur_plate_origin - plate0->get_origin();
        for (auto& polygon : m_unselected) {
            polygon.translation.x() -= scaled<double>(plate_offset.x());
            polygon.translation.y() -= scaled<double>(plate_offset.y());
        }
    }

    prepare_wipe_tower_ex(cur_plate->get_index());

    if (m_plater->get_prepare_state() == Job::JobPrepareState::PREPARE_STATE_EXTRA) {
        // this param is set to false when arrange selected(if have objects contained in the plate)
        params.do_final_align = m_unselected.empty();
    }
}
#endif

std::optional<arrangement::ArrangePolygon>
get_wipe_tower_arrangepoly(const Plater &plater)
{
    int id = plater.canvas3D()->fff_print()->get_plate_index();
    if (auto wti = get_wipe_tower(plater, id))
        return get_wipetower_arrange_poly(&wti);

    return {};
}

//BBS: add sudoku-style stride
double bed_stride_x(const Plater* plater) {
    double bedwidth = plater->build_volume().bounding_box().size().x();
    return (1. + LOGICAL_BED_GAP) * bedwidth;
}

double bed_stride_y(const Plater* plater) {
    double beddepth = plater->build_volume().bounding_box().size().y();
    return (1. + LOGICAL_BED_GAP) * beddepth;
}

// call before get selected and unselected
arrangement::ArrangeParams init_arrange_params(Plater *p)
{
    arrangement::ArrangeParams         params;
    GLCanvas3D::ArrangeSettings       &settings     = p->canvas3D()->get_arrange_settings();
    auto                              &print        = wxGetApp().plater()->get_partplate_list().get_current_fff_print();
    const PrintConfig                 &print_config = print.config();

    std::tuple<float, float> object_skirt = std::make_tuple(0.f, 0.f);
    if(!print.empty()) {
        if(print.is_all_objects_are_short()) {
            float object_skirt_width = print_config.skirt_loops >= 1 ? (print.skirt_flow().width() + (print_config.skirt_loops-1) * print.skirt_flow().spacing()) : 0.0f;
            float object_skirt_offset = print_config.skirt_distance + object_skirt_width;
            object_skirt = std::make_tuple(object_skirt_offset, object_skirt_width);
        }
        else
        {
            object_skirt = print.object_skirt_offset();
        }
    }

    float object_skirt_offset = std::get<0>(object_skirt);

    params.clearance_height_to_rod             = print_config.extruder_clearance_height_to_rod.value;
    params.clearance_height_to_lid             = print_config.extruder_clearance_height_to_lid.value;
    params.cleareance_radius                   = print_config.extruder_clearance_radius.value + object_skirt_offset * 2;;
    params.object_skirt_offset                 = object_skirt_offset;
    params.printable_height                    = print_config.printable_height.value;
    params.allow_rotations                     = settings.enable_rotation;
    params.nozzle_height                       = print.config().nozzle_height.value;
    params.align_center                        = print_config.best_object_pos.value;
    params.allow_multi_materials_on_same_plate = settings.allow_multi_materials_on_same_plate;
    params.avoid_extrusion_cali_region         = settings.avoid_extrusion_cali_region;
    params.is_seq_print                        = settings.is_seq_print;
    params.min_obj_distance                    = scaled(settings.distance);
    params.align_to_y_axis                     = settings.align_to_y_axis;

    PartPlateList &plate_list = p->get_partplate_list();
    PartPlate *    plate      = plate_list.get_curr_plate();
    bool plate_same_as_global = true;
    int state = p->get_prepare_state();
    if (state == Job::JobPrepareState::PREPARE_STATE_MENU) {
        params.is_seq_print       = plate->get_real_print_seq(&plate_same_as_global) == PrintSequence::ByObject;
        // if plate's print sequence is not the same as global, the settings.distance is no longer valid, we set it to auto
        if (!plate_same_as_global)
            params.min_obj_distance = 0;
    }
    else if(state == Job::JobPrepareState::PREPARE_STATE_DEFAULT) {  // global( all plates ) arrangement
        if( 1 == plate_list.get_plate_count()) {
            // if only one plate exist, use the plate sequence print setting
            params.is_seq_print       = plate->get_real_print_seq(&plate_same_as_global) == PrintSequence::ByObject;
            if (params.is_seq_print)
                params.min_obj_distance = 0;
        }
    }

    if (params.is_seq_print) {
        params.bed_shrink_x = BED_SHRINK_SEQ_PRINT;
        params.bed_shrink_y = BED_SHRINK_SEQ_PRINT;
    }
    return params;
}

}} // namespace Slic3r::GUI
