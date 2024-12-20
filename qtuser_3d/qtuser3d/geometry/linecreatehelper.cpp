#include "linecreatehelper.h"
#include "qtuser3d/geometry/bufferhelper.h"

namespace qtuser_3d
{
	LineCreateHelper::LineCreateHelper(QObject* parent)
		:GeometryCreateHelper(parent)
	{
	}
	
	LineCreateHelper::~LineCreateHelper()
	{
	}

	Qt3DRender::QGeometry* LineCreateHelper::create(int num, float* position, float* color, Qt3DCore::QNode* parent)
	{
		if (num == 0 || !position) return nullptr;

		Qt3DRender::QAttribute* positionAttribute = BufferHelper::CreateVertexAttribute((const char*)position, AttribueSlot::Position, num);
		Qt3DRender::QAttribute* colorAttribute = nullptr;
		if(color) colorAttribute = BufferHelper::CreateVertexAttribute((const char*)color, AttribueSlot::Color, num);
		return GeometryCreateHelper::create(parent, positionAttribute, colorAttribute);
	}
}