#include "spreadchunk.h"
#include "qtuser3d/refactor/xeffect.h"
#include "qtuser3d/trimesh2/renderprimitive.h"
#include "qtuser3d/refactor/pxentity.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
//#include "data/trimeshutils.h"

#include "qtuser3d/geometry/bufferhelper.h"
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "qtuser3d/trimesh2/create.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"
#include "interface/camerainterface.h"

#define CHECK_FACE 0

static Qt3DRender::QGeometry* createTrianglesWithFlags(const std::vector<trimesh::vec3>& tris, const std::vector<float>& flags1, const std::vector<float>& flags2)
{
	//if (tris.size() <= 0 || flags1.size() <= 0 || flags2.size() <= 0)
	//	return nullptr;

	int posNum = tris.size();
	int flag1Num = flags1.size();
	int flag2Num = flags2.size();

	Qt3DRender::QAttribute* positionAttribute = qtuser_3d::BufferHelper::CreateVertexAttribute(
		(const char*)tris.data(), posNum, 3, Qt3DRender::QAttribute::defaultPositionAttributeName());

	Qt3DRender::QAttribute* flagAttribute1 = qtuser_3d::BufferHelper::CreateVertexAttribute(
		(const char*)flags1.data(), flag1Num, 1, "vertexFlag");

	Qt3DRender::QAttribute* flagAttribute2 = qtuser_3d::BufferHelper::CreateVertexAttribute(
		(const char*)flags2.data(), flag2Num, 1, "originFlag");

	return qtuser_3d::GeometryCreateHelper::create(nullptr, positionAttribute, flagAttribute1, flagAttribute2);
}

SpreadChunk::SpreadChunk(int id, QObject* parent)
	:Pickable(parent)
	, m_id(id)
{
	m_entity = new qtuser_3d::PickXEntity();
	m_entity->bindPickable(this);

	setEnableSelect(false);
	creative_kernel::tracePickable(this);
}

SpreadChunk::~SpreadChunk()
{
	creative_kernel::unTracePickable(this);
	creative_kernel::visHideCustom(m_entity);
	delete m_entity;
}

int SpreadChunk::id()
{
	return m_id;
}

qtuser_3d::XEntity* SpreadChunk::entity()
{
	return m_entity;
}

void SpreadChunk::setEffect(qtuser_3d::XEffect* effect)
{
	m_entity->setEffect(effect); 
}

int SpreadChunk::parentId(int primitiveID)
{
	if (primitiveID < 0 || primitiveID >= (int)m_indexMap.size())
		return -1;

	return m_indexMap.at(primitiveID);
}

bool SpreadChunk::getFace(int primitiveID, trimesh::vec3& p1, trimesh::vec3& p2, trimesh::vec3& p3)
{
	if (primitiveID < 0 || primitiveID >= (int)m_indexMap.size())
		return false;

	p1 = m_positions.at(3 * primitiveID);
	p2 = m_positions.at(3 * primitiveID + 1); 
	p3 = m_positions.at(3 * primitiveID + 2);

	return true;
}

void SpreadChunk::updateData(const std::vector<trimesh::vec3>& position, const std::vector<int>& flags)
{
	m_positions = position;

	if (m_positions.size() == 0)
	{
		creative_kernel::visHideCustom(m_entity);
	}
	else
	{
		std::vector<float> vertexFlags;
		std::vector<float> originFlags;
		int flagNum = flags.size();
		vertexFlags.resize(flagNum * 3);
		originFlags.resize(flagNum * 3);
		int faceId = 1;
		for (int i = 0, j = 0; i < flagNum; ++i, j += 3)
		{
			int flag = flags[i];
			flag = flag == 0 ? m_defaultFlag : flag;
			flag -= 1;
#if CHECK_FACE
			vertexFlags[j] = faceId;
			vertexFlags[j + 1] = faceId;
			vertexFlags[j + 2] = faceId;
			originFlags[j] = faceId;
			originFlags[j + 1] = faceId;
			originFlags[j + 2] = faceId;
			faceId++;
#else 
			vertexFlags[j] = flag;
			vertexFlags[j + 1] = flag;
			vertexFlags[j + 2] = flag;
			originFlags[j] = flag;
			originFlags[j + 1] = flag;
			originFlags[j + 2] = flag;
#endif
		}
		//m_entity->setGeometry(qtuser_3d::createTrianglesWithFlags(position, vertexFlags), Qt3DRender::QGeometryRenderer::Triangles, true);
		m_entity->setGeometry(createTrianglesWithFlags(position, vertexFlags, originFlags), Qt3DRender::QGeometryRenderer::Triangles, true);
	
		creative_kernel::visShowCustom(m_entity);
	}
}

void SpreadChunk::updateData(const std::vector<trimesh::vec3>& position, const std::vector<int>& flags, const std::vector<int>& originFlags)
{
	m_positions = position;

	if (m_positions.size() == 0)
	{
		creative_kernel::visHideCustom(m_entity);
	}
	else
	{
		std::vector<float> vertexFlags1;
		std::vector<float> vertexFlags2;
		int flagNum = flags.size();
		vertexFlags1.resize(flagNum * 3);
		vertexFlags2.resize(flagNum * 3);
		for (int i = 0, j = 0; i < flagNum; ++i, j += 3)
		{
			int flag = flags[i];
			flag = flag == 0 ? m_defaultFlag : flag;
			flag -= 1;
			vertexFlags1[j] = flag;
			vertexFlags1[j + 1] = flag;
			vertexFlags1[j + 2] = flag;

			int orgFlag = originFlags[i];
			orgFlag = orgFlag == 0 ? m_defaultFlag : orgFlag;
			orgFlag -= 1;
			vertexFlags2[j] = orgFlag;
			vertexFlags2[j + 1] = orgFlag;
			vertexFlags2[j + 2] = orgFlag;

		}
		m_entity->setGeometry(createTrianglesWithFlags(position, vertexFlags1, vertexFlags2), Qt3DRender::QGeometryRenderer::Triangles, true);

		creative_kernel::visShowCustom(m_entity);
	}
}

void SpreadChunk::setIndexMap(std::vector<int> indexMap)
{
	m_indexMap= indexMap;
}

int SpreadChunk::primitiveNum()
{
	return (int)m_indexMap.size();
}

void SpreadChunk::updateMatrix()
{
}

void SpreadChunk::setPose(const QMatrix4x4& pose)
{
	m_entity->setPose(pose);
}

QMatrix4x4 SpreadChunk::pose() const
{
	return m_entity->pose();
}

void SpreadChunk::setDefaultFlag(int defaultFlag)
{
	m_defaultFlag = defaultFlag;
}
