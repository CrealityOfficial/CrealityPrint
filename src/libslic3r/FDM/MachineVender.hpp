#ifndef MACHINE_VENDER_HPP
#define MACHINE_VENDER_HPP

#include <string>
namespace Slic3r
{
    class DynamicPrintConfig;
}

namespace creality
{ 
    class MachineVender
    {
    public:
        MachineVender();
        ~MachineVender();

        //apply before slicing
        void apply_config(Slic3r::DynamicPrintConfig* config);
        bool is_firmwaresoft_mm_printer();

        void set_is_bbl_printer(bool isBBLPrinter);
        bool is_bbl_printer() const;
    protected:
        //SoftFever
        bool m_isBBLPrinter;

        bool m_is_firmwaresoft_mm_printer;  
    };

    bool is_firmwaresoft_mm_printer_from_string(const std::string& printer_model);
}

#endif // MACHINE_VENDER_HPP