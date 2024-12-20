#ifndef PARESEGCODE_1595984973500_H
#define PARESEGCODE_1595984973500_H

#include "crslice/interface.h"
#include "crslice/gcode/define.h"
#include "crslice/gcode/header.h"

#include <unordered_map>

namespace gcode
{
    void Stringsplit(std::string str, const char split, std::vector<std::string>& res);

    //for cloud
    void CRSLICE_API paraseGcode(const std::string& gCodeFile, std::vector<std::vector<SliceLine3D>>& m_sliceLayers, trimesh::box3& box, std::unordered_map<std::string, std::string>& kvs, ccglobal::Tracer* tracer = nullptr);

    //for load gcode
    void CRSLICE_API paraseGcodeAndPreview(const std::string& gCodeFile, GcodeTracer* pathData, ccglobal::Tracer* tracer = nullptr);
}
#endif // PARESEGCODE_1595984973500_H