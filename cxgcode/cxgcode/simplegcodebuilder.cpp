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

	}

	SimpleGCodeBuilder::~SimpleGCodeBuilder()
	{

	}

	float SimpleGCodeBuilder::traitSpeed(int layer, int step)
	{
		int index = stepIndex(layer, step);
		if (index >= 0 && index < baseInfo.totalSteps)
			return m_struct.m_moves[index].speed;
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

	Qt3DRender::QGeometry* SimpleGCodeBuilder::buildGeometry()
	{
#if SIMPLE_GCODE_IMPL == 1
		return qtuser_3d::GeometryCreateHelper::create(nullptr, &m_positions, &m_normals, &m_steps, &m_indices);
#elif SIMPLE_GCODE_IMPL == 3
		return qtuser_3d::GeometryCreateHelper::create(nullptr, &m_positions, &m_endPositions, &m_normals, &m_steps, &m_lineWidths);
#else
		return qtuser_3d::GeometryCreateHelper::create(nullptr, &m_positions, &m_normals, &m_steps);
#endif
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
		if (!attribute)
			return;

		qtuser_3d::AttributeShade steps;
		steps.name = QString("visualTypeFlags");

#if SIMPLE_GCODE_IMPL == 0
		cpuTriSoupUpdate(steps, type);
#elif SIMPLE_GCODE_IMPL == 1
		cpuIndicesUpdate(steps, type);
#elif SIMPLE_GCODE_IMPL == 2
#elif SIMPLE_GCODE_IMPL == 3
		gpuIndicesUpdate(steps, type);
#endif

		attribute->setBuffer(qtuser_3d::GeometryCreateHelper::createBuffer(&steps));
		attribute->setName(steps.name);
		attribute->setCount(steps.count);
		attribute->setVertexSize(steps.stride);
		attribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
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

		int stride = 1;
		int count = stride * stepCount;
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

		m_lineWidths.name = QString("lineWidth");
		m_lineWidths.stride = 1;
		m_lineWidths.count = count;
		m_lineWidths.bytes.resize(sizeof(float) * 1 * count);

		trimesh::vec3* tposition = (trimesh::vec3*)m_positions.bytes.data();
		trimesh::vec3* tEndPosition = (trimesh::vec3*)m_endPositions.bytes.data();
		trimesh::vec3* tnormals = (trimesh::vec3*)m_normals.bytes.data();
		trimesh::vec2* tsteps = (trimesh::vec2*)m_steps.bytes.data();
		float* tlineWidths = (float *)m_lineWidths.bytes.data();

		for (int i = 0; i < stepCount; ++i)
		{
			const gcode::GCodeMove& move = structMoves.at(i);
			trimesh::vec3* tempPosition = tposition + stride * i;
			trimesh::vec3* tempEndPosition = tEndPosition + stride * i;
			trimesh::vec3* tempNormals = tnormals + stride * i;
			trimesh::vec2* tempSteps = tsteps + stride * i;
			float* tempLineWidths = tlineWidths + stride * i;

			trimesh::vec3 start = structPositions.at(move.start);
			trimesh::vec3 end = structPositions.at(move.start + 1);

			*(tempPosition) = start;
			*(tempEndPosition) = end;
			*(tempNormals) = positionsNormals.at(i);
			*(tempSteps) = layerSteps.at(i);
			
			{
				int idx = m_struct.m_layerInfoIndex[i];
				const gcode::GcodeLayerInfo& l = m_struct.m_gcodeLayerInfos[idx];
				*tempLineWidths = l.width;
			}


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
				flag = (float)move.speed;
				//着色器里面把flag < 0.0的线段忽略
				if (move.type == SliceLineType::Travel)
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
				if (move.type == SliceLineType::Travel)
					flag = -1.0f;
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
				if (move.type == SliceLineType::Travel)
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

				
				if (move.type == SliceLineType::Travel)
					flag = -1.0f;
			}
			break;

			case gcode::GCodeVisualType::gvt_flow:
			{
				int idx = m_struct.m_layerInfoIndex[step];
				const gcode::GcodeLayerInfo& l = m_struct.m_gcodeLayerInfos[idx];
				float flow = l.flow;
				//qDebug() << "flow = " << flow;
				if (move.type == SliceLineType::Travel || flow <= 0.0)
					flag = -1.0f;
				else
				{
					flag = (flow - baseInfo.minFlowOfStep) / (baseInfo.maxFlowOfStep - baseInfo.minFlowOfStep);
				}
			}
				break;

			case gcode::GCodeVisualType::gvt_layerTime:
			{
				if (move.type == SliceLineType::Travel)
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
				flag = fminf(fmaxf(fans.fanSpeed / 255.0, 0.0), 1.0);

				//qDebug() << "fan speed = " << flag;
				if (move.type == SliceLineType::Travel)
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

				if (move.type == SliceLineType::Travel)
					flag = -1.0f;
			}
			break;

			default:
				break;
		}

		return flag;
	}

	void SimpleGCodeBuilder::getPathData(const trimesh::vec3 point, float e, int type)
	{
		m_struct.getPathData(point, e, type);
	}
	void SimpleGCodeBuilder::getPathDataG2G3(const trimesh::vec3 point, float i, float j, float e, int type, bool isG2)
	{
		m_struct.getPathDataG2G3(point,i,j,e,type,isG2);
	}
	void SimpleGCodeBuilder::setParam(gcode::GCodeParseInfo& pathParam)
	{
		m_struct.setParam(pathParam);
	}
	void SimpleGCodeBuilder::setLayer(int layer) {
		m_struct.setLayer(layer);
	}
	void SimpleGCodeBuilder::setSpeed(float s) {
		m_struct.setSpeed(s);
	}
	void SimpleGCodeBuilder::setTEMP(float temp) {
		m_struct.setTEMP(temp);
	}
	void SimpleGCodeBuilder::setExtruder(int nr) {
		m_struct.setExtruder(nr);
	}
	void SimpleGCodeBuilder::setFan(float fan) {
		m_struct.setFan(fan);
	}
	void SimpleGCodeBuilder::setZ(float z,float h){
		m_struct.setZ(z,h);
	}
	void SimpleGCodeBuilder::setE(float e) {
		m_struct.setE(e);
	}

	void SimpleGCodeBuilder::setTime(float time)
	{
		m_struct.setTime(time);
	}

	void SimpleGCodeBuilder::getNotPath()
	{
		m_struct.getNotPath();
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
}