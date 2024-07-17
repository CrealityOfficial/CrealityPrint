#include "slicepreviewnode.h"
#include "interface/appsettinginterface.h"

namespace creative_kernel
{
	SlicePreviewNode::SlicePreviewNode(Qt3DCore::QNode* parent)
		: GCodeViewEntity(parent)
	{
		// addPassFilter(0, "view");
	}

	SlicePreviewNode::~SlicePreviewNode()
	{
	}

	void SlicePreviewNode::initialize()
	{
		QVariantList values = CONFIG_PLUGIN_VARLIST(gcodeeffect_typecolors, slice_group);
		m_typeColors->setValue(values);

		m_speedColorValues = CONFIG_PLUGIN_VARLIST(gcodeeffect_speedcolors, slice_group);
		m_speedColors->setValue(m_speedColorValues);
		setParameter("speedcolorSize", (float)(m_speedColorValues.size() - 1));

		values = CONFIG_PLUGIN_VARLIST(gcodeeffect_nozzlecolors, slice_group);
		m_nozzleColors->setValue(values);
	}

	void SlicePreviewNode::setNozzleColorList(const QVariantList& list)
	{
		m_nozzleColors->setValue(list);
	}
}

