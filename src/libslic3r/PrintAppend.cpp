#include "Print.hpp"

#include "libslic3r/FDM/MachineVender.hpp"
#include <boost/log/trivial.hpp>

namespace Slic3r {
    void Print::set_is_BBL_printer(bool isBBLPrinter)
    {
        m_machine_vender->set_is_bbl_printer(isBBLPrinter);
    }

    const bool Print::is_BBL_printer() const
    {
        return m_machine_vender->is_bbl_printer();
    }
}