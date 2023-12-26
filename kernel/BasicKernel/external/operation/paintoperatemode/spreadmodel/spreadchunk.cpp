#include "spreadchunk.h"
#include "qtuser3d/refactor/xeffect.h"
#include "qtuser3d/trimesh2/renderprimitive.h"
#include "qtuser3d/refactor/pxentity.h"
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
//#include "data/trimeshutils.h"

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
		int flagNum = flags.size();
		vertexFlags.resize(flagNum * 3);
		for (int i = 0, j = 0; i < flagNum; ++i, j += 3)
		{
			int flag = flags[i];
			flag = flag == 0 ? flag : flag - 1;
			vertexFlags[j] = flag;
			vertexFlags[j + 1] = flag;
			vertexFlags[j + 2] = flag;
		}
		m_entity->setGeometry(qtuser_3d::createTrianglesWithFlags(position, vertexFlags), Qt3DRender::QGeometryRenderer::Triangles, true);
	
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

void SpreadChunk::setPose(const QMatrix4x4& pose)
{
	m_entity->setPose(pose);
}

QMatrix4x4 SpreadChunk::pose() const
{
	return m_entity->pose();
}
