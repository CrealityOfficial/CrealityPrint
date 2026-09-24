#include "KlipperStepper.hpp"

#include <cmath>

namespace Slic3r {
namespace KlipperSim {

KlipperStepper::KlipperStepper(std::string name,
                               KlipperMCU& mcu,
                               uint32_t oid,
                               double rotation_dist,
                               double steps_per_rotation,
                               bool invert_dir,
                               uint32_t max_error_ticks)
    : m_name(std::move(name))
    , m_rotation_dist(rotation_dist)
    , m_steps_per_rotation(steps_per_rotation)
    , m_step_dist(rotation_dist / steps_per_rotation)
    , m_mcu(mcu)
    , m_oid(oid)
    , m_invert_dir(invert_dir)
    , m_orig_invert_dir(invert_dir)
    , m_stepqueue(max_error_ticks)
{
    m_mcu.steppersync().register_stepqueue(&m_stepqueue);
}

void KlipperStepper::set_rotation_distance(double rotation_dist)
{
    int mcu_pos = get_mcu_position();
    m_rotation_dist = rotation_dist;
    m_step_dist = rotation_dist / m_steps_per_rotation;
    set_mcu_position(mcu_pos);
}

void KlipperStepper::set_dir_inverted(bool invert_dir)
{
    m_invert_dir = invert_dir;
}

void KlipperStepper::set_stepper_kinematics(KlipperItersolve* sk)
{
    int mcu_pos = 0;
    if (m_stepper_kinematics != nullptr)
        mcu_pos = get_mcu_position();
    m_stepper_kinematics = sk;
    if (m_stepper_kinematics != nullptr)
        m_stepper_kinematics->set_step_dist(m_step_dist);
    set_mcu_position(mcu_pos);
}

void KlipperStepper::set_position(double x, double y, double z)
{
    int mcu_pos = get_mcu_position();
    if (m_stepper_kinematics != nullptr)
        m_stepper_kinematics->set_position(x, y, z);
    set_mcu_position(mcu_pos);
}

bool KlipperStepper::is_active_axis(char axis) const
{
    return m_stepper_kinematics ? m_stepper_kinematics->is_active_axis(axis) : false;
}

double KlipperStepper::get_commanded_position() const
{
    return m_stepper_kinematics ? m_stepper_kinematics->commanded_position() : 0.0;
}

int KlipperStepper::get_mcu_position() const
{
    double mcu_pos_dist = get_commanded_position() + m_mcu_position_offset;
    double mcu_pos = mcu_pos_dist / m_step_dist;
    if (mcu_pos >= 0.0)
        return (int)(mcu_pos + 0.5);
    return (int)(mcu_pos - 0.5);
}

void KlipperStepper::set_mcu_position(int mcu_pos)
{
    double mcu_pos_dist = mcu_pos * m_step_dist;
    m_mcu_position_offset = mcu_pos_dist - get_commanded_position();
}

int KlipperStepper::get_past_mcu_position(double print_time) const
{
    uint64_t clock = m_mcu.print_time_to_clock(print_time);
    return (int)m_stepqueue.find_past_position(clock);
}

double KlipperStepper::mcu_to_commanded_position(int mcu_pos) const
{
    return mcu_pos * m_step_dist - m_mcu_position_offset;
}

void KlipperStepper::note_homing_end()
{
    m_stepqueue.reset_to_clock(0);
}

}} // namespace Slic3r::KlipperSim
