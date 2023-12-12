#ifndef CXGCODE_KERNEL_SIMPLEGCODEBUILDER_1677118729997_H
#define CXGCODE_KERNEL_SIMPLEGCODEBUILDER_1677118729997_H
#include "cxgcode/gcodebuilder.h"
#include "gcode/slicemodelbuilder.h"
#include "gcode/define.h"

#include <Qt3DRender/QGeometryRenderer>

namespace cxgcode
{	
	struct GCodeParseInfo;
	class CXGCODE_API SimpleGCodeBuilder : public GCodeBuilder
	{
	public:
		SimpleGCodeBuilder();
		virtual ~SimpleGCodeBuilder();

		float traitSpeed(int layer, int step) override;
		trimesh::vec3 traitPosition(int layer, int step) override;

		Qt3DRender::QGeometry* buildGeometry() override;
		Qt3DRender::QGeometry* buildRetractionGeometry();
		Qt3DRender::QGeometry* buildZSeamsGeometry();

		Qt3DRender::QGeometryRenderer* buildRetractionGeometryRenderer();
		Qt3DRender::QGeometryRenderer* buildZSeamsGeometryRenderer();

		void updateFlagAttribute(Qt3DRender::QAttribute* attribute, gcode::GCodeVisualType type) override;

		void getPathData(const trimesh::vec3 point, float e, int type);
		void getPathDataG2G3(const trimesh::vec3 point, float i, float j, float e, int type, bool isG2 = true);
		void setParam(gcode::GCodeParseInfo& pathParam);
		void setLayer(int layer);
		void setSpeed(float s);
		void setTEMP(float temp);
		void setExtruder(int nr);
		void setFan(float fan);
		void setZ(float z, float h);
		void setE(float e);
		void setTime(float time);
		void getNotPath();
	protected:
		void implBuild(SliceResultPointer result) override;
		void implBuild()override;
		int stepIndex(int layer, int step);

		void cpuTriSoupBuild();
		void cpuTriSoupUpdate(qtuser_3d::AttributeShade& shade, gcode::GCodeVisualType type);

		void cpuIndicesBuild();
		void cpuIndicesUpdate(qtuser_3d::AttributeShade& shade, gcode::GCodeVisualType type);

		void gpuTriSoupBuild();

		void gpuIndicesBuild();
		void gpuIndicesUpdate(qtuser_3d::AttributeShade& shade, gcode::GCodeVisualType type);

		void processOffsetNormals(std::vector<trimesh::vec3>& normals, bool step = false);
		void processSteps(std::vector<trimesh::ivec2>& layerSteps);
		float produceFlag(const gcode::GCodeMove& move, gcode::GCodeVisualType type, int step);

        //cr30 offset
        void processCr30offset(gcode::GCodeParseInfo& info);
		
		Qt3DRender::QGeometryRenderer* buildGeometryRenderer(const std::vector<trimesh::vec3>& positions, const std::vector<int>& index, trimesh::vec2* pStepsFlag);

	protected:
		gcode::GCodeStruct m_struct;
		qtuser_3d::AttributeShade m_positions;
		qtuser_3d::AttributeShade m_normals;
		qtuser_3d::AttributeShade m_steps;
		qtuser_3d::AttributeShade m_lineWidths;
#if SIMPLE_GCODE_IMPL == 1
		qtuser_3d::AttributeShade m_indices;
#elif SIMPLE_GCODE_IMPL == 3
		qtuser_3d::AttributeShade m_endPositions;
#endif
	};
}

#endif // CXGCODE_KERNEL_SIMPLEGCODEBUILDER_1677118729997_H
