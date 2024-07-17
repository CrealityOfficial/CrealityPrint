#include "gcodebuilder.h"
#include <regex>
#include "crslice/gcode/gcodedata.h"

namespace cxgcode
{
	GCodeBuilder::GCodeBuilder()
		:m_tracer(nullptr)
	{

	}

	GCodeBuilder::~GCodeBuilder()
	{

	}

	void GCodeBuilder::build(SliceResultPointer result, ccglobal::Tracer* tracer)
	{
		if (!result)
			return;

		m_tracer = tracer;
		gcode::parseGCodeInfo(result.get(), parseInfo);
		implBuild(result);
		m_tracer = nullptr;
	}

	void GCodeBuilder::build(ccglobal::Tracer* tracer)
	{
		implBuild();
		m_tracer = nullptr;
	}

	void GCodeBuilder::build_with_image(SliceResultPointer result, ccglobal::Tracer* tracer)
	{
		gcode::parseGCodeInfo(result.get(), parseInfo);
		implBuild();
		m_tracer = nullptr;
	}

    std::string trim(const std::string& str) {
        std::string::const_iterator it = str.begin();
        while (it != str.end() && std::isspace(*it))
            ++it;

        std::string::const_reverse_iterator rit = str.rbegin();
        while (rit.base() != it && std::isspace(*rit))
            ++rit;

        return std::string(it, rit.base());
    }
    

	void GCodeBuilder::implBuild(SliceResultPointer result)
	{

	}

	void GCodeBuilder::implBuild() {
	}

	float GCodeBuilder::traitSpeed(int layer, int step)
	{
		return 0.0f;
	}

	trimesh::vec3 GCodeBuilder::traitPosition(int layer, int step)
	{
		return trimesh::vec3();
	}

	Qt3DRender::QGeometry* GCodeBuilder::buildGeometry()
	{
		return nullptr;
	}

	void GCodeBuilder::updateFlagAttribute(Qt3DRender::QAttribute* attribute, gcode::GCodeVisualType type)
	{
	}
}
