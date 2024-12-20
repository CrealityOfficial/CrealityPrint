#include "simplegcodebuilder.h"
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "qtuser3d/geometry/basicshapecreatehelper.h"

#include <Qt3DRender/QBuffer>
#include <QVector3D>

#include "trimesh2/XForm.h"
namespace cxgcode
{
	trimesh::fxform beltXForm(const trimesh::vec3& offset, float angle, int beltType)
	{
		float theta = angle * M_PIf / 180.0f;

		trimesh::fxform xf0 = trimesh::fxform::trans(offset);

		trimesh::fxform xf1 = trimesh::fxform::identity();
		xf1(2, 2) = 1.0f / sinf(theta);
		xf1(1, 2) = -1.0f / tanf(theta);

		trimesh::fxform xf2 = trimesh::fxform::identity();
		xf2(2, 2) = 0.0f;
		xf2(1, 1) = 0.0f;
		xf2(2, 1) = -1.0f;
		xf2(1, 2) = 1.0f;

		if (1 == beltType)
		{
			trimesh::fxform xf3 = trimesh::fxform::trans(0.0f, 0.0f, 0.0f);

			trimesh::fxform xf = xf3 * xf2 * xf1 * xf0;
			//trimesh::fxform xf = xf3 * xf0;
			return xf;
		}
		else
		{
			trimesh::fxform xf3 = trimesh::fxform::trans(0.0f, 0.0f, 200.0f);
			trimesh::fxform xf = xf3 * xf2 * xf1 * xf0;
			return xf;
		}
	}

	SimpleGCodeBuilder::SimpleGCodeBuilder()
		:GCodeBuilder()
	{
		m_visualTypeFlags.name = QString("visualTypeFlags");
	}

	SimpleGCodeBuilder::~SimpleGCodeBuilder()
	{

	}


	float SimpleGCodeBuilder::traitDuration(int layer, int step)
	{
		int index = stepIndex(layer, step);
		if (index >= 0 && index < baseInfo.totalSteps)
		{
			int i = m_struct.m_moves[index].start;
			trimesh::vec3 p0 = m_struct.m_positions.at(i);
			trimesh::vec3 p1 = m_struct.m_positions.at(i + 1);
			float length = trimesh::length(p1 - p0);
			float currentSpeed = m_struct.m_moves[index].speed;
			if (length > 0.0f && currentSpeed > 0.0f)
			{
				//time use as millisecond
				float time = length * 60 * 1000 / currentSpeed;  

				return std::max(time, 10.0f);
			}
		}

		return 10.0f; //ms
	}

	float SimpleGCodeBuilder::traitSpeed(int layer, int step)
	{
		int index = stepIndex(layer, step);
		if (index >= 0 && index < baseInfo.totalSteps)
		{
			if ((baseInfo.speedMax - baseInfo.speedMin + 0.01f) > 0.0f)
			{
				//reference:void GcodeSpeedListModel::resetData()
				return m_struct.m_moves[index].speed / 60.0f;
			}
		}
		
		return 0.0f;
	}

	trimesh::vec3 SimpleGCodeBuilder::traitPosition(int layer, int step)
	{
		int index = stepIndex(layer, step);
		if (index >= 0 && index < baseInfo.totalSteps)
		{
			int i = m_struct.m_moves[index].start + 1;
			trimesh::vec3 p = m_struct.m_positions.at(i);
            p = parseInfo.xf4 * p;
            return p;
		}
		return trimesh::vec3();
	}

	float SimpleGCodeBuilder::layerHeight(int layer)
	{
		if (!m_struct.m_layerHeights.empty() && m_struct.m_layerHeights.size() >= layer && layer > 0)
		{
			return m_struct.m_layerHeights[layer - 1];
		}
		return -1;
	}

	int SimpleGCodeBuilder::layerIndex(float layerValue)
	{
		for (int i = 0; i < m_struct.m_layerHeights.size(); i++)
		{
			float layHeight_1 = m_struct.m_layerHeights[i];
			float layHeight_2 = layHeight_1;
			if (i + 1 < m_struct.m_layerHeights.size())
			{
				layHeight_2 = m_struct.m_layerHeights[i + 1];
			}
			if (qFuzzyCompare(layerValue, layHeight_1) || (layerValue > layHeight_1 && layerValue < layHeight_2) )
			{
				return i+1;
			}
		}

		return 1;
	}


	float SimpleGCodeBuilder::traitLayerHeight(int layer, int step)
	{
		int index = stepIndex(layer, step);
		if (index >= 0 && index < baseInfo.totalSteps)
		{
			int idx = m_struct.m_layerInfoIndex[index];
			const gcode::GcodeLayerInfo& l = m_struct.m_gcodeLayerInfos[idx];
			
			float min = baseInfo.minLayerHeight;
			float max = baseInfo.maxLayerHeight;

			if ((max - min + 0.01f) > 0.0f)
			{
				return l.layerHight;
			}
		}
		return layerHeight(layer);
	}

	float SimpleGCodeBuilder::traitAcc(int layer, int step)
	{
		int index = stepIndex(layer, step);
		if (index >= 0 && index < baseInfo.totalSteps)
		{
			if ((baseInfo.maxAcc - baseInfo.minAcc + 0.01f) > 0.0f)
			{
				return m_struct.m_moves[index].acc;
			}
		}

		return 0.0f;
	}

	float SimpleGCodeBuilder::traitLineWidth(int layer, int step)
	{
		int index = stepIndex(layer, step);
		if (index >= 0 && index < baseInfo.totalSteps)
		{
			int idx = m_struct.m_layerInfoIndex[index];
			const gcode::GcodeLayerInfo& l = m_struct.m_gcodeLayerInfos[idx];
			return l.width;
		}
		return 0.0f;
	}

	float SimpleGCodeBuilder::traitFlow(int layer, int step)
	{
		int index = stepIndex(layer, step);
		if (index >= 0 && index < baseInfo.totalSteps)
		{
			int idx = m_struct.m_layerInfoIndex[index];
			const gcode::GcodeLayerInfo& l = m_struct.m_gcodeLayerInfos[idx];
			return l.flow;
		}

		return 0.0f;
	}

	float SimpleGCodeBuilder::traitLayerTime(int layer, int step)
	{
		int index = stepIndex(layer, step);
		if (index >= 0 && index < baseInfo.totalSteps)
		{
			std::map<int, float>::iterator it = m_struct.m_layerTimes.find(layer - INDEX_START_AT_ONE);
			if (it != m_struct.m_layerTimes.end())
			{
				float time = it->second;
				return time;
			}
		}

		return 0.0f;
	}

	float SimpleGCodeBuilder::traitFanSpeed(int layer, int step)
	{
		int index = stepIndex(layer, step);
		if (index >= 0 && index < baseInfo.totalSteps)
		{
			int idx = m_struct.m_fanIndex[index];
			gcode::GcodeFan& fans = m_struct.m_fans[idx];
			return fans.fanSpeed;
		}
		return 0.0f;
	}

	float SimpleGCodeBuilder::traitTemperature(int layer, int step)
	{
		int index = stepIndex(layer, step);
		if (index >= 0 && index < baseInfo.totalSteps)
		{
			int idx = m_struct.m_temperatureIndex[index];
			gcode::GcodeTemperature& t = m_struct.m_temperatures[idx];
			return t.temperature;
		}

		return 0.0f;
	}

	Qt3DRender::QGeometry* SimpleGCodeBuilder::buildGeometry()
	{
#if SIMPLE_GCODE_IMPL == 1
		return qtuser_3d::GeometryCreateHelper::create(nullptr, &m_positions, &m_normals, &m_steps, &m_indices);
#elif SIMPLE_GCODE_IMPL == 3

		Qt3DRender::QGeometry *geo = qtuser_3d::GeometryCreateHelper::create(nullptr, &m_positions, &m_endPositions, &m_normals, &m_steps, &m_lineWidthAndLayerHeights);
		{
			Qt3DRender::QBuffer* buffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
			buffer->setData(m_visualTypeFlags.bytes);
			m_visualTypeFlagsAttribute = new Qt3DRender::QAttribute(buffer, m_visualTypeFlags.name, Qt3DRender::QAttribute::Float, m_visualTypeFlags.stride, m_visualTypeFlags.count);
			geo->addAttribute(m_visualTypeFlagsAttribute);
		}
		return geo;

#else
		return qtuser_3d::GeometryCreateHelper::create(nullptr, &m_positions, &m_normals, &m_steps);
#endif
	}

	void SimpleGCodeBuilder::rebuildGeometryVisualTypeData()
	{
		if (m_visualTypeFlagsAttribute && m_visualTypeFlagsAttribute->buffer())
		{
			Qt3DRender::QBuffer* buffer = m_visualTypeFlagsAttribute->buffer();
			buffer->setData(m_visualTypeFlags.bytes);
			m_visualTypeFlagsAttribute->setCount(m_visualTypeFlags.count);
		}
	}

	Qt3DRender::QGeometry* SimpleGCodeBuilder::buildRetractionGeometry()
	{
		const std::vector<trimesh::vec3>& positions = m_struct.m_positions;
		const std::vector<int>& retractions = m_struct.m_retractions;
		int count = retractions.size();

		trimesh::vec2* stepsFlags = (trimesh::vec2*)m_steps.bytes.data();

		qtuser_3d::AttributeShade* positionAttr = new qtuser_3d::AttributeShade();
		positionAttr->name = Qt3DRender::QAttribute::defaultPositionAttributeName();
		positionAttr->stride = 3;
		positionAttr->count = count;
		positionAttr->bytes.resize(sizeof(float) * 3 * count);

		qtuser_3d::AttributeShade* stepsAttr = new qtuser_3d::AttributeShade();
		stepsAttr->name = QString("stepsFlag");
		stepsAttr->stride = 2;
		stepsAttr->count = count;
		stepsAttr->bytes.resize(sizeof(float) * 2 * count);

		{
			trimesh::vec3* tPosition = (trimesh::vec3*)positionAttr->bytes.data();
			trimesh::vec2* tSteps = (trimesh::vec2*)stepsAttr->bytes.data();

			for (size_t i = 0; i < count; i++)
			{
				int idx = retractions[i];
				if (idx >= positions.size() || idx >= m_steps.count)
				{
					continue;
				}
				const trimesh::vec3& v = positions.at(idx);
				tPosition[i] = v;
				tSteps[i] = stepsFlags[idx];
			}
		}

		Qt3DRender::QGeometry* geometry = qtuser_3d::GeometryCreateHelper::create(nullptr, positionAttr, stepsAttr);
		delete positionAttr;
		delete stepsAttr;

		return geometry;
	}

	Qt3DRender::QGeometry* SimpleGCodeBuilder::buildZSeamsGeometry()
	{
		const std::vector<trimesh::vec3>& positions = m_struct.m_positions;
		const std::vector<int>& zSeamsIndex = m_struct.m_zSeams;
		int count = zSeamsIndex.size();

		trimesh::vec2* stepsFlags = (trimesh::vec2*)m_steps.bytes.data();

		qtuser_3d::AttributeShade* positionAttr = new qtuser_3d::AttributeShade();
		positionAttr->name = Qt3DRender::QAttribute::defaultPositionAttributeName();
		positionAttr->stride = 3;
		positionAttr->count = count;
		positionAttr->bytes.resize(sizeof(float) * 3 * count);

		qtuser_3d::AttributeShade* stepsAttr = new qtuser_3d::AttributeShade();
		stepsAttr->name = QString("stepsFlag");
		stepsAttr->stride = 2;
		stepsAttr->count = count;
		stepsAttr->bytes.resize(sizeof(float) * 2 * count);

		{
			trimesh::vec3* tPosition = (trimesh::vec3*)positionAttr->bytes.data();
			trimesh::vec2* tSteps = (trimesh::vec2*)stepsAttr->bytes.data();

			for (size_t i = 0; i < count; i++)
			{
				int idx = zSeamsIndex[i];
				if (idx >= positions.size() || idx >= m_steps.count)
				{
					continue;
				}
				const trimesh::vec3& v = positions.at(idx);
				tPosition[i] = v;
				tSteps[i] = stepsFlags[idx];
			}
		}

		Qt3DRender::QGeometry* geometry = qtuser_3d::GeometryCreateHelper::create(nullptr, positionAttr, stepsAttr);
		delete positionAttr;
		delete stepsAttr;

		return geometry;
	}

	Qt3DRender::QGeometry* SimpleGCodeBuilder::buildUnretractGeometry()
	{
		const std::vector<trimesh::vec3>& positions = m_struct.m_positions;
		const std::vector<gcode::GCodeMove>& moves = m_struct.m_moves;
		
		std::vector<gcode::GCodeMove> unretract;
		for (size_t i = 0; i < moves.size(); i++)
		{
			const gcode::GCodeMove& m = moves.at(i);
			if (m.type == SliceLineType::Unretract)
			{
				unretract.emplace_back(m);
			}
		}

		int count = unretract.size();

		trimesh::vec2* stepsFlags = (trimesh::vec2*)m_steps.bytes.data();

		qtuser_3d::AttributeShade* positionAttr = new qtuser_3d::AttributeShade();
		positionAttr->name = Qt3DRender::QAttribute::defaultPositionAttributeName();
		positionAttr->stride = 3;
		positionAttr->count = count;
		positionAttr->bytes.resize(sizeof(float) * 3 * count);

		qtuser_3d::AttributeShade* stepsAttr = new qtuser_3d::AttributeShade();
		stepsAttr->name = QString("stepsFlag");
		stepsAttr->stride = 2;
		stepsAttr->count = count;
		stepsAttr->bytes.resize(sizeof(float) * 2 * count);

		{
			trimesh::vec3* tPosition = (trimesh::vec3*)positionAttr->bytes.data();
			trimesh::vec2* tSteps = (trimesh::vec2*)stepsAttr->bytes.data();

			for (size_t i = 0; i < count; i++)
			{
				const gcode::GCodeMove& m = unretract.at(i);
				int idx = m.start;
				if (idx >= positions.size() || idx >= m_steps.count)
				{
					continue;
				}
				const trimesh::vec3& v = positions.at(idx);
				tPosition[i] = v;
				tSteps[i] = stepsFlags[idx];
			}
		}

		Qt3DRender::QGeometry* geometry = qtuser_3d::GeometryCreateHelper::create(nullptr, positionAttr, stepsAttr);
		delete positionAttr;
		delete stepsAttr;

		return geometry;
	}

	Qt3DRender::QGeometryRenderer* SimpleGCodeBuilder::buildRetractionGeometryRenderer()
	{
		return buildGeometryRenderer(m_struct.m_positions, m_struct.m_retractions, (trimesh::vec2*)m_steps.bytes.data());;
	}

	Qt3DRender::QGeometryRenderer* SimpleGCodeBuilder::buildZSeamsGeometryRenderer()
	{
		return buildGeometryRenderer(m_struct.m_positions, m_struct.m_zSeams, (trimesh::vec2*)m_steps.bytes.data());
	}

	void SimpleGCodeBuilder::updateFlagAttribute(Qt3DRender::QAttribute* attribute, gcode::GCodeVisualType type)
	{
		/*if (!attribute)
			return;*/

		qtuser_3d::AttributeShade& steps = m_visualTypeFlags;
		
#if SIMPLE_GCODE_IMPL == 0
		cpuTriSoupUpdate(steps, type);
#elif SIMPLE_GCODE_IMPL == 1
		cpuIndicesUpdate(steps, type);
#elif SIMPLE_GCODE_IMPL == 2
#elif SIMPLE_GCODE_IMPL == 3
		gpuIndicesUpdate(steps, type);
#endif
		/*attribute->setBuffer(qtuser_3d::GeometryCreateHelper::createBuffer(&steps));
		attribute->setName(steps.name);
		attribute->setCount(steps.count);
		attribute->setVertexSize(steps.stride);
		attribute->setVertexBaseType(Qt3DRender::QAttribute::Float);*/
	}

	gcode::GCodeStruct& SimpleGCodeBuilder::getGCodeStruct()
	{
		return m_struct;
	}

	trimesh::box3 SimpleGCodeBuilder::pathBox()
	{
		if (!m_path_box.valid)
		{
			int pCount = (int)m_struct.m_positions.size();
			int stepCount = (int)m_struct.m_moves.size();
			if (pCount < 2 || stepCount < 1)
				return trimesh::box3();

			for (int i = 0; i < stepCount; ++i)
			{
				const gcode::GCodeMove& move = m_struct.m_moves.at(i);

				if (SliceLineType::OuterWall == move.type || SliceLineType::erSkirt == move.type || SliceLineType::erBrim == move.type ||
					SliceLineType::erSupportMaterial == move.type ||
					SliceLineType::erSupportMaterialInterface == move.type || SliceLineType::erSupportTransition == move.type || SliceLineType::erWipeTower == move.type)
				{
					if (move.start >= 0 && move.start < m_struct.m_positions.size())
					{
						m_path_box += m_struct.m_positions[move.start];
					}

					if ((move.start + 1) >= 0 && (move.start + 1) < m_struct.m_positions.size())
					{
						m_path_box += m_struct.m_positions[move.start + 1];
					}
				}

			}
		}

		return m_path_box;
	}

	void SimpleGCodeBuilder::implBuild(SliceResultPointer result)
	{
        if (!result)
            return;

		float step = 0.9f;
		if (m_tracer)
			m_tracer->resetProgressScope(0.0f, step);

		m_struct.buildFromResult(result, parseInfo, baseInfo, m_stepGCodesMaps, m_tracer);
        //cr30
        processCr30offset(parseInfo);
		if (m_tracer)
		{
			m_tracer->resetProgressScope(step, 1.0f);
			m_tracer->variadicFormatMessage(1);
		}
#if SIMPLE_GCODE_IMPL == 0
		cpuTriSoupBuild();
#elif SIMPLE_GCODE_IMPL == 1
		cpuIndicesBuild();
#elif SIMPLE_GCODE_IMPL == 2
#elif SIMPLE_GCODE_IMPL == 3
		gpuIndicesBuild();
#endif
	}

	void SimpleGCodeBuilder::implBuild()
	{
		float step = 0.9f;
		if (m_tracer)
			m_tracer->resetProgressScope(0.0f, step);

		m_struct.buildFromResult(parseInfo, baseInfo, m_stepGCodesMaps, m_tracer);
		//cr30
		processCr30offset(parseInfo);
		if (m_tracer)
			m_tracer->resetProgressScope(step, 1.0f);
#if SIMPLE_GCODE_IMPL == 0
		cpuTriSoupBuild();
#elif SIMPLE_GCODE_IMPL == 1
		cpuIndicesBuild();
#elif SIMPLE_GCODE_IMPL == 2
#elif SIMPLE_GCODE_IMPL == 3
		gpuIndicesBuild();
#endif
	}

    void SimpleGCodeBuilder::processCr30offset(gcode::GCodeParseInfo& info)
    {
        const int& beltType = parseInfo.beltType;
        if (beltType)
        {
            trimesh::vec3 offset(0, 0, 0);
            trimesh::vec3 offsetY(-info.beltOffset, 0, -info.beltOffsetY);
            if (1 == beltType)//creality print belt
            {
                trimesh::fxform xf = beltXForm(offset, 45.0f, 1);
                info.xf4 = trimesh::inv(xf);
            }
            else//creality slicer belt
            {
                trimesh::fxform xf = beltXForm(offset, 45.0f, beltType);
                info.xf4 = trimesh::inv(xf);
            }
            info.xf4 = info.xf4 * trimesh::fxform::trans(offsetY);
        }
    }

	int SimpleGCodeBuilder::stepIndex(int layer, int step)
	{
		int _layer = layer - INDEX_START_AT_ONE;
		int _step = step - INDEX_START_AT_ONE;

		int startIndex = 0;
		for (int i = 0; i < _layer; ++i)
			startIndex += baseInfo.steps.at(i);

		return startIndex + _step;
	}

	void SimpleGCodeBuilder::cpuTriSoupBuild()
	{
		float sradius = parseInfo.layerHeight / 2.0f;
		float lradius = parseInfo.lineWidth / 2.0f;

		const std::vector<trimesh::vec3>& structPositions = m_struct.m_positions;
		const std::vector<gcode::GCodeMove>& structMoves = m_struct.m_moves;

		int pCount = (int)structPositions.size();
		int stepCount = (int)structMoves.size();
		if (pCount < 2 || stepCount < 1)
			return;

		//position offset normals
		std::vector<trimesh::vec3> positionsNormals;
		processOffsetNormals(positionsNormals);

		std::vector<trimesh::ivec2> layerSteps;
		processSteps(layerSteps);

		int stride = 24;
		int count = stride * stepCount;
		m_positions.name = Qt3DRender::QAttribute::defaultPositionAttributeName();
		m_positions.stride = 3;
		m_positions.count = count;
		m_positions.bytes.resize(sizeof(float) * 3 * count);

		m_normals.name = Qt3DRender::QAttribute::defaultNormalAttributeName();
		m_normals.stride = 3;
		m_normals.count = count;
		m_normals.bytes.resize(sizeof(float) * 3 * count);

		m_steps.name = QString("stepsFlag");
		m_steps.stride = 2;
		m_steps.count = count;
		m_steps.bytes.resize(sizeof(float) * 2 * count);

		trimesh::uivec3 spridIndex[8];
		spridIndex[0] = trimesh::uivec3(0, 4, 1);
		spridIndex[1] = trimesh::uivec3(4, 5, 1);
		spridIndex[2] = trimesh::uivec3(1, 5, 6);
		spridIndex[3] = trimesh::uivec3(1, 6, 2);
		spridIndex[4] = trimesh::uivec3(0, 3, 7);
		spridIndex[5] = trimesh::uivec3(0, 7, 4);
		spridIndex[6] = trimesh::uivec3(6, 7, 3);
		spridIndex[7] = trimesh::uivec3(6, 3, 2);

		trimesh::vec3* tposition = (trimesh::vec3*)m_positions.bytes.data();
		trimesh::vec3* tnormals = (trimesh::vec3*)m_normals.bytes.data();
		trimesh::vec2* tsteps = (trimesh::vec2*)m_steps.bytes.data();
		for (int i = 0; i < stepCount; ++i)
		{
			const gcode::GCodeMove& move = structMoves.at(i);
			trimesh::vec3* tempPosition = tposition + stride * i;
			trimesh::vec3* tempNormals = tnormals + stride * i;
			trimesh::vec2* tempSteps = tsteps + stride * i;

			trimesh::vec3 poses[8];
			trimesh::vec3 start = structPositions.at(move.start);
			trimesh::vec3 startNormal = positionsNormals.at(move.start);
			trimesh::vec3 end = structPositions.at(move.start + 1);
			trimesh::vec3 endNormal = positionsNormals.at(move.start + 1);

			poses[0] = start + sradius * trimesh::vec3(0.0f, 0.0f, 1.0f);
			poses[1] = start + lradius * startNormal;
			poses[2] = start - sradius * trimesh::vec3(0.0f, 0.0f, 1.0f);
			poses[3] = start - lradius * startNormal;
			poses[4] = end + sradius * trimesh::vec3(0.0f, 0.0f, 1.0f);
			poses[5] = end + lradius * endNormal;
			poses[6] = end - sradius * trimesh::vec3(0.0f, 0.0f, 1.0f);
			poses[7] = end - lradius * endNormal;

			trimesh::ivec2 ls = layerSteps.at(i);
			for (int j = 0; j < 8; ++j)
			{
				const trimesh::uivec3& triangle = spridIndex[j];

				const trimesh::vec3& p0 = poses[triangle.x];
				const trimesh::vec3& p1 = poses[triangle.y];
				const trimesh::vec3& p2 = poses[triangle.z];

				trimesh::vec3 n01 = p1 - p0;
				trimesh::vec3 n02 = p2 - p0;
				trimesh::vec3 n = n01 TRICROSS n02;
				trimesh::normalize(n);

				*(tempPosition) = p0;
				*(tempPosition + 1) = p1;
				*(tempPosition + 2) = p2;

				*(tempNormals) = n;
				*(tempNormals + 1) = n;
				*(tempNormals + 2) = n;

				*(tempSteps) = ls;
				*(tempSteps + 1) = ls;
				*(tempSteps + 2) = ls;

				tempPosition += 3;
				tempNormals += 3;
				tempSteps += 3;
			}
		}
	}

	void SimpleGCodeBuilder::cpuTriSoupUpdate(qtuser_3d::AttributeShade& shade, gcode::GCodeVisualType type)
	{
		const std::vector<trimesh::vec3>& structPositions = m_struct.m_positions;
		const std::vector<gcode::GCodeMove>& structMoves = m_struct.m_moves;

		int pCount = (int)structPositions.size();
		int stepCount = (int)structMoves.size();
		if (pCount < 2 || stepCount < 1)
			return;

		int stride = 24;
		int count = stride * stepCount;

		shade.stride = 1;
		shade.count = count;
		shade.bytes.resize(sizeof(float) * 1 * count);

		float* data = (float*)shade.bytes.data();
		for (int i = 0; i < stepCount; ++i)
		{
			const gcode::GCodeMove& move = structMoves.at(i);
			float* tempSteps = data + stride * i;

			float flag = produceFlag(move, type, i);
			for (int j = 0; j < 24; ++j)
			{
				*(tempSteps + j) = flag;
			}
		}
	}

	void SimpleGCodeBuilder::cpuIndicesBuild()
	{
#if SIMPLE_GCODE_IMPL == 1
		float sradius = parseInfo.layerHeight / 2.0f;
		float lradius = parseInfo.lineWidth / 2.0f;

		const std::vector<trimesh::vec3>& structPositions = m_struct.m_positions;
		const std::vector<GCodeMove>& structMoves = m_struct.m_moves;

		int pCount = (int)structPositions.size();
		int stepCount = (int)structMoves.size();
		if (pCount < 2 || stepCount < 1)
			return;

		//position offset normals
		std::vector<trimesh::vec3> positionsNormals;
		processOffsetNormals(positionsNormals, true);

		std::vector<trimesh::ivec2> layerSteps;
		processSteps(layerSteps);

		int stride = 8;
		int count = stride * stepCount;
		m_positions.name = Qt3DRender::QAttribute::defaultPositionAttributeName();
		m_positions.stride = 3;
		m_positions.count = count;
		m_positions.bytes.resize(sizeof(float) * 3 * count);

		m_normals.name = Qt3DRender::QAttribute::defaultNormalAttributeName();
		m_normals.stride = 3;
		m_normals.count = count;
		m_normals.bytes.resize(sizeof(float) * 3 * count);

		m_steps.name = QString("stepsFlag");
		m_steps.stride = 2;
		m_steps.count = count;
		m_steps.bytes.resize(sizeof(float) * 2 * count);

		m_indices.stride = 1;
		m_indices.count = 3 * count;
		m_indices.bytes.resize(sizeof(unsigned) * 3 * count);
		m_indices.type = 1;

		trimesh::uivec3 spridIndex[8];
		spridIndex[0] = trimesh::uivec3(0, 4, 1);
		spridIndex[1] = trimesh::uivec3(4, 5, 1);
		spridIndex[2] = trimesh::uivec3(1, 5, 6);
		spridIndex[3] = trimesh::uivec3(1, 6, 2);
		spridIndex[4] = trimesh::uivec3(0, 3, 7);
		spridIndex[5] = trimesh::uivec3(0, 7, 4);
		spridIndex[6] = trimesh::uivec3(6, 7, 3);
		spridIndex[7] = trimesh::uivec3(6, 3, 2);

		trimesh::vec3* tposition = (trimesh::vec3*)m_positions.bytes.data();
		trimesh::vec3* tnormals = (trimesh::vec3*)m_normals.bytes.data();
		trimesh::vec2* tsteps = (trimesh::vec2*)m_steps.bytes.data();
		trimesh::uivec3* indices = (trimesh::uivec3*)m_indices.bytes.data();
		for (int i = 0; i < stepCount; ++i)
		{
			const GCodeMove& move = structMoves.at(i);
			trimesh::vec3* tempPosition = tposition + stride * i;
			trimesh::vec3* tempNormals = tnormals + stride * i;
			trimesh::vec2* tempSteps = tsteps + stride * i;
			trimesh::uivec3* tempIndices = indices + stride * i;

			trimesh::vec3 start = structPositions.at(move.start);
			trimesh::vec3 end = structPositions.at(move.start + 1);

			trimesh::vec3 poses[8];
			trimesh::vec3 norms[8];

			norms[0] = trimesh::vec3(0.0f, 0.0f, 1.0f);
			norms[1] = positionsNormals.at(i);
			norms[2] = trimesh::vec3(0.0f, 0.0f, - 1.0f);
			norms[3] = - positionsNormals.at(i);
			norms[4] = trimesh::vec3(0.0f, 0.0f, 1.0f);
			norms[5] = positionsNormals.at(i);
			norms[6] = trimesh::vec3(0.0f, 0.0f, -1.0f);
			norms[7] = - positionsNormals.at(i);

			poses[0] = start + sradius * norms[0];
			poses[1] = start + lradius * norms[1];
			poses[2] = start + sradius * norms[2];
			poses[3] = start + lradius * norms[3];
			poses[4] = end + sradius * norms[4];
			poses[5] = end + lradius * norms[5];
			poses[6] = end + sradius * norms[6];
			poses[7] = end + lradius * norms[7];

			trimesh::uivec3 startIndex = trimesh::uivec3(stride * i, stride * i, stride * i);

			trimesh::ivec2 ls = layerSteps[i];
			for (int j = 0; j < 8; ++j)
			{
				*(tempPosition + j) = poses[j];
				*(tempNormals + j) = norms[j];
				*(tempSteps + j) = ls;
				*(tempIndices + j) = spridIndex[j] + startIndex;
			}
		}
#endif
	}

	void SimpleGCodeBuilder::cpuIndicesUpdate(qtuser_3d::AttributeShade& shade, gcode::GCodeVisualType type)
	{
#if SIMPLE_GCODE_IMPL == 1
		const std::vector<trimesh::vec3>& structPositions = m_struct.m_positions;
		const std::vector<GCodeMove>& structMoves = m_struct.m_moves;

		int pCount = (int)structPositions.size();
		int stepCount = (int)structMoves.size();
		if (pCount < 2 || stepCount < 1)
			return;

		int stride = 8;
		int count = stride * stepCount;

		shade.stride = 1;
		shade.count = count;
		shade.bytes.resize(sizeof(float) * 1 * count);

		float* data = (float*)shade.bytes.data();
		for (int i = 0; i < stepCount; ++i)
		{
			const GCodeMove& move = structMoves.at(i);
			float* tempSteps = data + stride * i;

			float flag = produceFlag(move, type);
			for (int j = 0; j < 8; ++j)
			{
				*(tempSteps + j) = flag;
			}
		}
#endif
	}

	void SimpleGCodeBuilder::gpuTriSoupBuild()
	{

	}

	void SimpleGCodeBuilder::gpuIndicesBuild()
	{
#if SIMPLE_GCODE_IMPL == 3
		
		const std::vector<trimesh::vec3>& structPositions = m_struct.m_positions;
		const std::vector<gcode::GCodeMove>& structMoves = m_struct.m_moves;

		int pCount = (int)structPositions.size();
		int stepCount = (int)structMoves.size();
		if (pCount < 2 || stepCount < 1)
			return;

		//position offset normals
		std::vector<trimesh::vec3> positionsNormals;
		processOffsetNormals(positionsNormals, true);

		std::vector<trimesh::ivec2> layerSteps;
		processSteps(layerSteps);

		int count = stepCount;
		m_positions.name = Qt3DRender::QAttribute::defaultPositionAttributeName();
		m_positions.stride = 3;
		m_positions.count = count;
		m_positions.bytes.resize(sizeof(float) * 3 * count);

		m_endPositions.name = QString("endVertexPosition");
		m_endPositions.stride = 3;
		m_endPositions.count = count;
		m_endPositions.bytes.resize(sizeof(float) * 3 * count);

		m_normals.name = Qt3DRender::QAttribute::defaultNormalAttributeName();
		m_normals.stride = 3;
		m_normals.count = count;
		m_normals.bytes.resize(sizeof(float) * 3 * count);

		m_steps.name = QString("stepsFlag");
		m_steps.stride = 2;
		m_steps.count = count;
		m_steps.bytes.resize(sizeof(float) * 2 * count);

		m_lineWidthAndLayerHeights.name = QString("lineWidth_layerHeight");
		m_lineWidthAndLayerHeights.stride = 4;
		m_lineWidthAndLayerHeights.count = count;
		m_lineWidthAndLayerHeights.bytes.resize(sizeof(float) * 4 * count);


		trimesh::vec3* tposition = (trimesh::vec3*)m_positions.bytes.data();
		trimesh::vec3* tEndPosition = (trimesh::vec3*)m_endPositions.bytes.data();
		trimesh::vec3* tnormals = (trimesh::vec3*)m_normals.bytes.data();
		trimesh::vec2* tsteps = (trimesh::vec2*)m_steps.bytes.data();
		trimesh::vec4* tlineWidthHeight = (trimesh::vec4*)m_lineWidthAndLayerHeights.bytes.data();

		const gcode::GCodeParseInfo& parseInfo = m_struct.getParam();

		for (int i = 0; i < stepCount; ++i)
		{
			const gcode::GCodeMove& move = structMoves.at(i);
			trimesh::vec3* tempPosition = tposition + i;
			trimesh::vec3* tempEndPosition = tEndPosition + i;
			trimesh::vec3* tempNormals = tnormals + i;
			trimesh::vec2* tempSteps = tsteps + i;
			trimesh::vec4* tempLineWidthHeight = tlineWidthHeight + i;

			trimesh::vec3 start = structPositions.at(move.start);
			trimesh::vec3 end = structPositions.at(move.start + 1);

			*(tempPosition) = start;
			*(tempEndPosition) = end;
			*(tempNormals) = positionsNormals.at(i);
			*(tempSteps) = layerSteps.at(i);
			
			int idx = m_struct.m_layerInfoIndex[i];
			const gcode::GcodeLayerInfo& l = m_struct.m_gcodeLayerInfos[idx];

			const SliceLineType& linetype = move.type;
			float tempLineWidth;
			if (linetype == SliceLineType::Travel || linetype == SliceLineType::MoveCombing || linetype == SliceLineType::React)
			{
				tempLineWidth = parseInfo.lineWidth * 0.25;
			} 
			else if (linetype == SliceLineType::Wipe)
			{
				tempLineWidth = parseInfo.lineWidth * 0.15;
			}
			else if (linetype == SliceLineType::erIroning)
			{
				tempLineWidth = l.width;
				*(tempPosition) = start + trimesh::vec3(0.0f, 0.0f, 1.0f) * (parseInfo.layerHeight * 0.5);
				*(tempEndPosition) = end + trimesh::vec3(0.0f, 0.0f, 1.0f) * (parseInfo.layerHeight * 0.5);
			}
			/*else if (linetype == SliceLineType::erInternalBridgeInfill) 
			{
				tempLineWidth = l.width;
			}*/
			else
			{
				tempLineWidth = l.width;
			}
			
			float compensate0 = calculateCornelCompensate(structPositions, structMoves, move.start);
			float compensate1 = calculateCornelCompensate(structPositions, structMoves, move.start + 1);
			*tempLineWidthHeight = trimesh::vec4(tempLineWidth, l.layerHight, compensate0, compensate1);

			if (m_tracer && (i % 1000 == 0))
			{
				m_tracer->progress((float)i / (float)stepCount);
				if (m_tracer->interrupt())
				{
					m_tracer->failed("SimpleGCodeBuilder::gpuIndicesBuild interrupt.");
					break;
				}
			}
		}
#endif
	}

	void SimpleGCodeBuilder::gpuIndicesUpdate(qtuser_3d::AttributeShade& shade, gcode::GCodeVisualType type)
	{
		const std::vector<trimesh::vec3>& structPositions = m_struct.m_positions;
		const std::vector<gcode::GCodeMove>& structMoves = m_struct.m_moves;

		int pCount = (int)structPositions.size();
		int stepCount = (int)structMoves.size();
		if (pCount < 2 || stepCount < 1)
			return;

		int stride = 1;
		int count = stride * stepCount;

		shade.stride = 1;
		shade.count = count;
		shade.bytes.resize(sizeof(float) * 1 * count);

		float* data = (float*)shade.bytes.data();
		for (int i = 0; i < stepCount; ++i)
		{
			const gcode::GCodeMove& move = structMoves.at(i);
			float* tempData = data + stride * i;

			*(tempData) = produceFlag(move, type, i);
		}
	}

	void SimpleGCodeBuilder::processOffsetNormals(std::vector<trimesh::vec3>& normals, bool step)
	{
		const std::vector<trimesh::vec3>& structPositions = m_struct.m_positions;
		const std::vector<gcode::GCodeMove>& structMoves = m_struct.m_moves;

		int pCount = (int)structPositions.size();
		int stepCount = (int)structMoves.size();
		if (pCount < 1 || stepCount < 1)
			return;

		if (step)
		{
			normals.resize(stepCount);
			for (int i = 0; i < stepCount; ++i)
			{
				const gcode::GCodeMove& move = structMoves.at(i);
				trimesh::vec3 delta = structPositions.at(move.start + 1) - structPositions.at(move.start);
				delta.z = 0.0f;
				trimesh::normalize(delta);
				trimesh::vec3 norm = trimesh::vec3(0.0f, 0.0f, 1.0f) TRICROSS delta;
				trimesh::normalize(norm);

				normals.at(i) = norm;
			}
		}
		else
		{
			normals.resize(pCount);
			for (int i = 0; i < pCount; ++i)
			{
				float len = 1.0f;
				trimesh::vec3 norm;
				if (i == 0)
				{
					trimesh::vec3 delta = structPositions.at(i + 1) - structPositions.at(i);
					delta.z = 0.0f;
					trimesh::normalize(delta);
					norm = trimesh::vec3(0.0f, 0.0f, 1.0f) TRICROSS delta;
				}
				else if (i == pCount - 1)
				{
					trimesh::vec3 delta = structPositions.at(i) - structPositions.at(i - 1);
					delta.z = 0.0f;
					trimesh::normalize(delta);
					norm = trimesh::vec3(0.0f, 0.0f, 1.0f) TRICROSS delta;
				}
				else
				{
					trimesh::vec3 delta1 = structPositions.at(i + 1) - structPositions.at(i);
					trimesh::vec3 delta2 = structPositions.at(i) - structPositions.at(i - 1);
					delta1.z = 0.0f;
					delta2.z = 0.0f;
					trimesh::normalize(delta1);
					trimesh::normalize(delta2);
					norm = trimesh::vec3(0.0f, 0.0f, 1.0f) TRICROSS(delta1 + delta2);
					trimesh::normalize(norm);
					float d = norm DOT delta2;
					if (d < 0.9999f)
					{
						len = 1.0f / sqrtf(1.0f - d * d);
					}
				}

				trimesh::normalize(norm);
				normals.at(i) = norm * len;
			}
		}
	}

	void SimpleGCodeBuilder::processSteps(std::vector<trimesh::ivec2>& layerSteps)
	{
		layerSteps.clear();

		const std::vector<trimesh::vec3>& structPositions = m_struct.m_positions;
		const std::vector<gcode::GCodeMove>& structMoves = m_struct.m_moves;
		int stepCount = (int)structMoves.size();
		int pCount = (int)structPositions.size();
		if (stepCount < 1 || pCount < 2)
			return;

		layerSteps.resize(stepCount);
		int layer = 0;
		int step = 0;
		for (int i = 0; i < stepCount; ++i)
		{
			layerSteps.at(i) = trimesh::vec2(layer + INDEX_START_AT_ONE
											, step + INDEX_START_AT_ONE);
			if (i < pCount - 1)
			{
				++step;
				if (step >= baseInfo.steps.at(layer))
				{
					++layer;
					step = 0;
				}
			}
		}
	}

	float SimpleGCodeBuilder::produceFlag(const gcode::GCodeMove& move, gcode::GCodeVisualType type, int step)
	{
		float flag = 0.0f;
		
		switch (type)
		{
			case gcode::GCodeVisualType::gvt_speed:
			{
				//flag = (float)move.speed;
				flag = (move.speed - baseInfo.speedMin) / (baseInfo.speedMax - baseInfo.speedMin + 0.01f);
				//着色器里面把flag < 0.0的线段忽略
				if (move.type == SliceLineType::Travel || move.type == SliceLineType::MoveCombing || move.type == SliceLineType::React)
					flag = -1.0f;
			}
			break;

			case gcode::GCodeVisualType::gvt_acc:
			{
				flag = (move.acc - baseInfo.minAcc) / (baseInfo.maxAcc - baseInfo.minAcc + 0.01f);
				if (move.type == SliceLineType::Travel || move.type == SliceLineType::MoveCombing || move.type == SliceLineType::React)
					flag = -1.0f;
			}
			break;

			case gcode::GCodeVisualType::gvt_structure:
			{
				flag = (float)move.type;
			}
			break;

			case gcode::GCodeVisualType::gvt_extruder:
			{
				flag = (float)move.extruder;
				if (move.type == SliceLineType::Travel || move.type == SliceLineType::MoveCombing || move.type == SliceLineType::React)
				{
					flag = -1.0f;
				}
				else if (step < m_steps.count)
				{
					// find pause layers and set flag(16)
					int stride = 1;
					trimesh::vec2* tsteps = (trimesh::vec2*)m_steps.bytes.data();
					trimesh::vec2 layerAndStepAtOne = *(tsteps + stride * step);
					int layer = layerAndStepAtOne.x - INDEX_START_AT_ONE;

					const std::vector<int>& pauseLayers = m_struct.m_pause;
					std::vector<int>::const_iterator it = std::find(pauseLayers.begin(), pauseLayers.end(), layer);
					if (it != pauseLayers.end())
					{
						//喷嘴颜色使用前面50个，第51个作为暂停层的标志位
						flag = 50.0f;
					}
				}
			}
			break;

			case gcode::GCodeVisualType::gvt_layerHight:
			{
				//重新映射到[0.0, 1.0]
				int idx = m_struct.m_layerInfoIndex[step];
				const gcode::GcodeLayerInfo& l = m_struct.m_gcodeLayerInfos[idx];

				float min = baseInfo.minLayerHeight;
				float max = baseInfo.maxLayerHeight;

				if (min > 0.0 && (max - min) < min / 50.0)
				{
					flag = 0.0;
				}
				else {
					flag = (l.layerHight - min) / (max - min);
				}
				//qDebug() << "layer height = " << flag;
				if (move.type == SliceLineType::Travel || move.type == SliceLineType::MoveCombing || move.type == SliceLineType::React || move.type == SliceLineType::erWipeTower || move.type == SliceLineType::Wipe || move.type == SliceLineType::Unretract)
					flag = -1.0f;
			}
			break;

			case gcode::GCodeVisualType::gvt_lineWidth:
			{
				//重新映射到[0.0, 1.0]
				int idx = m_struct.m_layerInfoIndex[step];
				const gcode::GcodeLayerInfo& l = m_struct.m_gcodeLayerInfos[idx];

				float diff = (baseInfo.maxLineWidth - baseInfo.minLineWidth);
				if (diff <= 0.002)
				{
					flag = 0.0;
				}
				else {
					flag = (l.width - baseInfo.minLineWidth) / diff;
				}

				
				if (move.type == SliceLineType::Travel || move.type == SliceLineType::MoveCombing || move.type == SliceLineType::React)
					flag = -1.0f;
			}
			break;

			case gcode::GCodeVisualType::gvt_flow:
			{
				int idx = m_struct.m_layerInfoIndex[step];
				const gcode::GcodeLayerInfo& l = m_struct.m_gcodeLayerInfos[idx];
				float flow = l.flow;
				//qDebug() << "flow = " << flow;
				if (move.type == SliceLineType::Travel || flow <= 0.0 || move.type == SliceLineType::MoveCombing || move.type == SliceLineType::React)
					flag = -1.0f;
				else
				{
					flag = (flow - baseInfo.minFlowOfStep) / (baseInfo.maxFlowOfStep - baseInfo.minFlowOfStep);
				}
			}
				break;

			case gcode::GCodeVisualType::gvt_layerTime:
			{
				if (move.type == SliceLineType::Travel || move.type == SliceLineType::MoveCombing || move.type == SliceLineType::React)
					flag = -1.0f;
				else if (step < m_steps.count)
				{
					int stride = 1;
					trimesh::vec2* tsteps = (trimesh::vec2*)m_steps.bytes.data();
					trimesh::vec2 layerAndStepAtOne = *(tsteps + stride * step);
					int layer = layerAndStepAtOne.x - INDEX_START_AT_ONE;

					std::map<int, float>::iterator it = m_struct.m_layerTimes.find(layer);
					if (it != m_struct.m_layerTimes.end())
					{
						float time = it->second;
						//qDebug() << "layer Time = " << time;
						flag = (time - baseInfo.minTimeOfLayer) / (baseInfo.maxTimeOfLayer - baseInfo.minTimeOfLayer);	
						return flag;
					}
				}
				qDebug() << "some thing error";
				return flag;
			}

			case gcode::GCodeVisualType::gvt_fanSpeed:
			{
				//[0, 100%]
#ifdef DEBUG
				assert(0 <= step && step < m_struct.m_fanIndex.size());
#endif // DEBUG
				int idx = m_struct.m_fanIndex[step];
#ifdef DEBUG
				assert(0 <= idx && idx < m_struct.m_fans.size());
#endif // DEBUG
				gcode::GcodeFan& fans = m_struct.m_fans[idx];

				flag = fminf(fmaxf(fans.fanSpeed / 100.0, 0.0), 1.0);

				//qDebug() << "fan speed = " << flag;
				if (move.type == SliceLineType::Travel || move.type == SliceLineType::MoveCombing || move.type == SliceLineType::React)
					flag = -1.0f;
			}
			break;


			case gcode::GCodeVisualType::gvt_temperature:
			{
				//重新映射到[0.0, 1.0]
				int idx = m_struct.m_temperatureIndex[step];
				gcode::GcodeTemperature& t = m_struct.m_temperatures[idx];
				float temp = t.temperature ;
				
				float diff = (baseInfo.maxTemperature - baseInfo.minTemperature);
				if (diff <= 0.1)
				{
					flag = 0.0;
				}
				else {
					flag = (temp - baseInfo.minTemperature) / diff;
				}

				if (move.type == SliceLineType::Travel || move.type == SliceLineType::MoveCombing || move.type == SliceLineType::React)
					flag = -1.0f;
			}
			break;

			default:
				break;
		}

		return flag;
	}

	/*
	* 多实例渲染：
	* 本例是渲染多个球体，除了球体本身的几何数据之外，增加了每个球体世界坐标及其标识信息
	* 参考：https://github.com/qt/qt3d/tree/dev/examples/qt3d/instanced-arrays-qml
	*/
	Qt3DRender::QGeometryRenderer* SimpleGCodeBuilder::buildGeometryRenderer(const std::vector<trimesh::vec3>& positions, const std::vector<int>& index, trimesh::vec2* pStepsFlag)
	{
		int count = index.size();

		QByteArray positionBytes, stepsFlagBytes;
		positionBytes.resize(sizeof(float) * 3 * count);
		stepsFlagBytes.resize(sizeof(float) * 2 * count);

		{
			trimesh::vec3* tPosition = (trimesh::vec3*)positionBytes.data();
			trimesh::vec2* tSteps = (trimesh::vec2*)stepsFlagBytes.data();

			for (size_t i = 0; i < count; i++)
			{
				int idx = index[i];
				if (idx >= positions.size() || idx >= m_steps.count)
				{
					continue;
				}
				const trimesh::vec3& v = positions.at(idx);
				tPosition[i] = v;
				tSteps[i] = pStepsFlag[idx];
			}
		}

		Qt3DRender::QGeometryRenderer* renderer = new Qt3DRender::QGeometryRenderer();

		Qt3DRender::QBuffer* positionBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		positionBuffer->setData(positionBytes);
		Qt3DRender::QAttribute* positionAtt = new Qt3DRender::QAttribute(positionBuffer, "worldPos", Qt3DRender::QAttribute::Float, 3, count, 0, 3 * sizeof(float));
		positionAtt->setDivisor(1);
		positionAtt->setParent(renderer);

		Qt3DRender::QBuffer* stepsFlagsBuffer = new Qt3DRender::QBuffer(Qt3DRender::QBuffer::VertexBuffer);
		stepsFlagsBuffer->setData(stepsFlagBytes);
		Qt3DRender::QAttribute* stepsFlagsAtt = new Qt3DRender::QAttribute(stepsFlagsBuffer, "stepsFlag", Qt3DRender::QAttribute::Float, 2, count, 0, 2 * sizeof(float));
		stepsFlagsAtt->setDivisor(1);
		stepsFlagsAtt->setParent(renderer);


		Qt3DRender::QGeometry* geo = qtuser_3d::BasicShapeCreateHelper::createBall(QVector3D(0.0f, 0.0f, 0.0f), 0.3f, 30);
		geo->addAttribute(positionAtt);
		geo->addAttribute(stepsFlagsAtt);
		renderer->setGeometry(geo);
		renderer->setInstanceCount(count);

		return renderer;
	}

	float SimpleGCodeBuilder::calculateCornelCompensate(const std::vector<trimesh::vec3>& positions, const std::vector<gcode::GCodeMove>& moves, int index)
	{
		float compensate = 0.0f;
		if (0 < index && index < positions.size()-1)
		{
			const gcode::GCodeMove& curmove = moves.at(index);
			const gcode::GCodeMove& premove = moves.at(index - 1);

			auto f = [](SliceLineType linetype) {

				bool exceptLineWidth = false;
				if (linetype == SliceLineType::Travel ||
					linetype == SliceLineType::MoveCombing ||
					linetype == SliceLineType::React ||
					linetype == SliceLineType::Wipe ||
					linetype == SliceLineType::erIroning)
				{
					exceptLineWidth = true;
				}
				return exceptLineWidth;
			};

			bool preExcept = f(premove.type);
			bool curExcept = f(curmove.type);
			if (preExcept != curExcept)
			{
				return compensate;
			}

			//相邻的两个线段，计算出衔接处的拐角补偿因子
			trimesh::vec3 currentDir = trimesh::normalized(positions[index-1] - positions[index]);
			trimesh::vec3 nextDir = trimesh::normalized(positions[index + 1] - positions[index]);

			float d = trimesh::dot(currentDir, nextDir);
			if (d >= 0.999 || d <= -0.999)
			{
				return compensate;
			}
			float theta = acos(d) / 2.0;
			
			trimesh::vec3 crs = trimesh::cross(nextDir, currentDir);
			float sign = trimesh::sign(trimesh::dot(crs, trimesh::vec3(0.0f, 0.0f, 1.0f)));

			compensate = sign / tanf(theta);

			if (compensate > 2.50f || compensate < -2.50f)
			{
				compensate = 0.0f;
			}
		}

		return compensate;
	}
}