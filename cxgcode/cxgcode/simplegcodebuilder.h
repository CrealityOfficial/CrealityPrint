#ifndef CXGCODE_KERNEL_SIMPLEGCODEBUILDER_1677118729997_H
#define CXGCODE_KERNEL_SIMPLEGCODEBUILDER_1677118729997_H
#include "cxgcode/gcodebuilder.h"
#include "crslice/gcode/slicemodelbuilder.h"
#include "crslice/gcode/define.h"

#include <Qt3DRender/QGeometryRenderer>

namespace cxgcode
{	
	struct GCodeParseInfo;
	class CXGCODE_API SimpleGCodeBuilder : public GCodeBuilder
	{
	public:
		SimpleGCodeBuilder();
		virtual ~SimpleGCodeBuilder();

		float layerHeight(int layer);
		int layerIndex(float layerValue);

		float traitDuration(int layer, int step);
		trimesh::vec3 traitPosition(int layer, int step) override;
		float traitSpeed(int layer, int step) override;
		float traitLayerHeight(int layer, int step);
		float traitAcc(int layer, int step);
		float traitLineWidth(int layer, int step);
		float traitFlow(int layer, int step);
		float traitLayerTime(int layer, int step);
		float traitFanSpeed(int layer, int step);
		float traitTemperature(int layer, int step);

		Qt3DRender::QGeometry* buildGeometry() override;
		void rebuildGeometryVisualTypeData();
		Qt3DRender::QGeometry* buildRetractionGeometry();
		Qt3DRender::QGeometry* buildZSeamsGeometry();
		Qt3DRender::QGeometry* buildUnretractGeometry();

		Qt3DRender::QGeometryRenderer* buildRetractionGeometryRenderer();
		Qt3DRender::QGeometryRenderer* buildZSeamsGeometryRenderer();

		void updateFlagAttribute(Qt3DRender::QAttribute* attribute, gcode::GCodeVisualType type) override;

		gcode::GCodeStruct& getGCodeStruct();
		trimesh::box3 pathBox();
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
		
		//���������߶��νӴ��Ĺսǲ���
		float calculateCornelCompensate(const std::vector<trimesh::vec3>& positions, const std::vector<gcode::GCodeMove>& moves, int index);

		Qt3DRender::QGeometryRenderer* buildGeometryRenderer(const std::vector<trimesh::vec3>& positions, const std::vector<int>& index, trimesh::vec2* pStepsFlag);

	protected:
		gcode::GCodeStruct m_struct;
		qtuser_3d::AttributeShade m_positions;
		qtuser_3d::AttributeShade m_normals;
		qtuser_3d::AttributeShade m_steps;
		qtuser_3d::AttributeShade m_lineWidthAndLayerHeights; //combine line width & layer height per step

#if SIMPLE_GCODE_IMPL == 1
		qtuser_3d::AttributeShade m_indices;
#elif SIMPLE_GCODE_IMPL == 3
		qtuser_3d::AttributeShade m_endPositions;
#endif
		qtuser_3d::AttributeShade m_visualTypeFlags;

		Qt3DRender::QAttribute* m_visualTypeFlagsAttribute;

		trimesh::box3 m_path_box;
	};
}

#endif // CXGCODE_KERNEL_SIMPLEGCODEBUILDER_1677118729997_H
