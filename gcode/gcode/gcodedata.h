#ifndef GCODE_CXSW_GCODEDATA_1603698412412_H
#define GCODE_CXSW_GCODEDATA_1603698412412_H
#include "gcode/interface.h"
#include <vector>
#include "ccglobal/tracer.h"
#include "gcode/slicemodelbuilder.h"
#define PI 3.1415926535

namespace cxsw
{
	GCODE_API bool gcodeGenerate(const std::string& fileName, gcode::GCodeStruct& _gcodeStruct,ccglobal::Tracer* tracer = nullptr);
	GCODE_API bool _SaveGCode(const std::string& fileName, const std::string& previewImageString,
		const std::vector<std::string>& layerString, const std::string& prefixString, const std::string& tailCode);

	GCODE_API void parseGCodeInfo(gcode::SliceResult* result, gcode::GCodeParseInfo& parseInfo);
}

#endif // GCODE_CXSW_GCODEDATA_1603698412412_H