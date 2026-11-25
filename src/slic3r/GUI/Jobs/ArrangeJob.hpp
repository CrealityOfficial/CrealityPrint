#ifndef ARRANGEJOB_HPP
#define ARRANGEJOB_HPP


#include <functional>
#include <optional>
#include <set>

#include "Job.hpp"
#include "libslic3r/Arrange.hpp"

namespace Slic3r {

class ModelInstance;

namespace GUI {

class Plater;

class ArrangeJob : public Job
{
    using ArrangePolygon = arrangement::ArrangePolygon;
    using ArrangePolygons = arrangement::ArrangePolygons;

    //BBS: add locked logic
    ArrangePolygons m_selected, m_unselected, m_unprintable, m_locked;
    std::vector<ModelInstance*> m_unarranged;
    std::map<int, ArrangePolygons> m_selected_groups;   // groups of selected items for sequential printing
    std::vector<int> m_uncompatible_plates;  // plate indices with different printing sequence than global

    // under the "only_on_partplate" situation, if ArrangePolygon can not place on the current plate, move them to the top left position of the first plate
    std::vector<int> m_move_top_left_ids;
    std::map<int, ModelInstance*> m_instance_id_to_instance;

    // the unprintable model need to move to top left of first plate
    std::vector<ModelInstance*> m_unprintable_instances;  

    arrangement::ArrangeParams params;
    int current_plate_index = 0;
    Polygon bed_poly;
    Plater *m_plater;

    // BBS: add flag for whether on current part plate
    bool only_on_partplate{false};

    // clear m_selected and m_unselected, reserve space for next usage
    void clear_input();

    // Prepare the selected and unselected items separately. If nothing is
    // selected, behaves as if everything would be selected.
    // if consider_lock is false, will ignore the plate lock state, (the "Arrange Selected" situation will set the "consider_lock" to false)
    void prepare_selected(bool consider_lock = true);

#if AUTO_CONVERT_3MF
    void prepare_auto_convert_3mf_selected(bool consider_lock = true);
    void prepare_wipe_tower_ex(int plate_index);
#endif

    void prepare_all();

    //BBS:prepare the items from current selected partplate
    void prepare_partplate();
    void prepare_wipe_tower();
    void queue_estimated_wipe_tower(int plate_index, int unlocked_index, const std::set<int>& extruder_ids);
    std::vector<std::pair<size_t, std::function<ArrangePolygon()>>> m_pending_wipe_towers;

    ArrangePolygon prepare_arrange_polygon(void* instance);

protected:

    void check_unprintable();

public:

    void prepare();

    void process(Ctl &ctl) override;

    ArrangeJob();

    int status_range() const
    {
        // ensure finalize() is called after all operations in process() is finished.
        return int(m_selected.size() + m_unprintable.size() + 1);
    }

    void finalize(bool canceled, std::exception_ptr &e) override;
};

std::optional<arrangement::ArrangePolygon> get_wipe_tower_arrangepoly(const Plater &);

// The gap between logical beds in the x axis expressed in ratio of
// the current bed width.
static const constexpr double LOGICAL_BED_GAP = 1. / 5.;

//BBS: add sudoku-style strides for x and y
// Stride between logical beds
double bed_stride_x(const Plater* plater);
double bed_stride_y(const Plater* plater);

arrangement::ArrangeParams init_arrange_params(Plater *p);

}} // namespace Slic3r::GUI

#endif // ARRANGEJOB_HPP
