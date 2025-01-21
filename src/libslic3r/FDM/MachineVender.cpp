#include "MachineVender.hpp"
#include "libslic3r/PrintConfig.hpp"

#include <algorithm>
#include <boost/algorithm/string.hpp>

namespace creality
{
    MachineVender::MachineVender()
        : m_isBBLPrinter(false)
        , m_is_firmwaresoft_mm_printer(false)
    {

    }

    MachineVender::~MachineVender()
    {

    }

    void MachineVender::apply_config(Slic3r::DynamicPrintConfig* config)
    {
        if(config == nullptr)
            return;

        m_is_firmwaresoft_mm_printer = is_firmwaresoft_mm_printer_from_string(config->opt_string("printer_model"));
    }

    bool MachineVender::is_firmwaresoft_mm_printer()
    {
        return m_is_firmwaresoft_mm_printer;
    }

    void MachineVender::set_is_bbl_printer(bool isBBLPrinter)
    {
        m_isBBLPrinter = isBBLPrinter;
    }

    bool MachineVender::is_bbl_printer() const
    {
        return m_isBBLPrinter;
    }

    bool is_firmwaresoft_mm_printer_from_string(const std::string& printer_model)
    {
        std::string str = printer_model;
        std::transform(str.begin(), str.end(), str.begin(),
                    [](unsigned char c) {return std::tolower(c);}
                    );

        if(boost::contains(str, "f008") ||
            boost::contains(str, "k2 plus") ||
            boost::contains(str, "k1") ||
            boost::contains(str, "ender") ||
            boost::starts_with(str, "cr"))
            return true;

        return false;
    }
}