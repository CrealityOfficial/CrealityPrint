#ifndef slic3r_GCode_KlipperSim_KlipperStepper_hpp_
#define slic3r_GCode_KlipperSim_KlipperStepper_hpp_

#include <cstdint>
#include <string>

#include "KlipperItersolve.hpp"
#include "KlipperMCU.hpp"
#include "KlipperStepCompress.hpp"
#include "KlipperTrapQ.hpp"

namespace Slic3r {
namespace KlipperSim {

// Partial C++ port of klippy/stepper.py MCU_stepper.
// This mirrors the state and helper methods used to connect:
//   kinematics (itersolve) <-> stepcompress <-> mcu
// It intentionally keeps the Python file's responsibilities grouped together,
// instead of spreading them across ad-hoc helpers in the simulator.
class KlipperStepper
{
public:
    KlipperStepper(std::string name,
                   KlipperMCU& mcu,
                   uint32_t oid,
                   double rotation_dist,
                   double steps_per_rotation,
                   bool invert_dir,
                   uint32_t max_error_ticks);

    const std::string& name() const { return m_name; }
    uint32_t oid() const { return m_oid; }
    double step_dist() const { return m_step_dist; }
    double rotation_dist() const { return m_rotation_dist; }
    bool invert_dir() const { return m_invert_dir; }

    void set_rotation_distance(double rotation_dist);
    void set_dir_inverted(bool invert_dir);

    void set_stepper_kinematics(KlipperItersolve* sk);
    KlipperItersolve* stepper_kinematics() const { return m_stepper_kinematics; }

    void set_trapq(const KlipperTrapQ* tq) { m_trapq = tq; }
    const KlipperTrapQ* trapq() const { return m_trapq; }
    void set_position(double x, double y, double z);
    bool is_active_axis(char axis) const;

    double get_commanded_position() const;
    int get_mcu_position() const;
    void set_mcu_position(int mcu_pos);
    int get_past_mcu_position(double print_time) const;
    double mcu_to_commanded_position(int mcu_pos) const;

    void note_homing_end();

    KlipperStepCompress& stepqueue() { return m_stepqueue; }
    const KlipperStepCompress& stepqueue() const { return m_stepqueue; }
    KlipperMCU& mcu() { return m_mcu; }
    const KlipperMCU& mcu() const { return m_mcu; }

private:
    std::string          m_name;
    double               m_rotation_dist;
    double               m_steps_per_rotation;
    double               m_step_dist;
    KlipperMCU&          m_mcu;
    uint32_t             m_oid;
    bool                 m_invert_dir;
    bool                 m_orig_invert_dir;
    double               m_mcu_position_offset = 0.0;
    KlipperStepCompress  m_stepqueue;
    KlipperItersolve*    m_stepper_kinematics = nullptr;
    const KlipperTrapQ*  m_trapq = nullptr;
};

}} // namespace Slic3r::KlipperSim

#endif
