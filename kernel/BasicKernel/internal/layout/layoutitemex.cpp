#include "layoutitemex.h"
#include "interface/spaceinterface.h"
#include "msbase/space/cloud.h"
#include "qtuser3d/trimesh2/conv.h"

namespace creative_kernel
{
	LayoutItemEx::LayoutItemEx(int groupId, bool outlineConcave, QObject* parent)
		: QObject(parent)
		, modelGroupId(groupId)
		, m_outlineConcave(outlineConcave)
	{

	}

	LayoutItemEx::~LayoutItemEx()
	{
#ifdef DEBUG
		qInfo() << Q_FUNC_INFO;
#endif // DEBUG

	}

	std::vector<trimesh::vec3> LayoutItemEx::outLine()
	{
		ModelGroup* aGroup = getModelGroupByObjectId(modelGroupId);
		Q_ASSERT(aGroup);

		std::vector<trimesh::vec3> paths = aGroup->rsPath();

		if (m_outlineConcave)
		{
			return aGroup->concave_path();
		}

		return paths;

	}
}