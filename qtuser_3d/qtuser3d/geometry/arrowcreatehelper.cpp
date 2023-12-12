#include "arrowcreatehelper.h"
#include "qtuser3d/geometry/bufferhelper.h"
#include "qtuser3d/geometry/arrowgeometrydata.h"

namespace qtuser_3d
{
	ArrowCreateHelper::ArrowCreateHelper(QObject* parent)
		:GeometryCreateHelper(parent)
	{
	}
	
	ArrowCreateHelper::~ArrowCreateHelper()
	{
	}

	Qt3DRender::QGeometry* ArrowCreateHelper::create(Qt3DCore::QNode* parent)
	{
		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)static_arrow_position, AttribueSlot::Position, static_arrow_vertex_count);
		Qt3DRender::QAttribute* indexAttribute = BufferHelper::CreateIndexAttribute((const char*)static_arrow_indices, static_arrow_index_count);

		return GeometryCreateHelper::create(parent, positionAttribute, indexAttribute);
	}
}