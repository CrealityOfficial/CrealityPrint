#include "EnumSettings.h"

namespace cura52
{
    std::string flavorToString(const EGCodeFlavor& flavor)
    {
        switch (flavor)
        {
        case EGCodeFlavor::BFB:
            return "BFB";
        case EGCodeFlavor::MACH3:
            return "Mach3";
        case EGCodeFlavor::MAKERBOT:
            return "Makerbot";
        case EGCodeFlavor::ULTIGCODE:
            return "UltiGCode";
        case EGCodeFlavor::MARLIN_VOLUMATRIC:
            return "Marlin(Volumetric)";
        case EGCodeFlavor::GRIFFIN:
            return "Griffin";
        case EGCodeFlavor::REPETIER:
            return "Repetier";
        case EGCodeFlavor::REPRAP:
            return "RepRap";
        case EGCodeFlavor::MACH3_Creality:
            return "MACH3(Creality)";
        case EGCodeFlavor::Creality_OS:
            return "Creality OS";
        case EGCodeFlavor::MARLIN:
        default:
            return "Marlin";
        }
    }
}