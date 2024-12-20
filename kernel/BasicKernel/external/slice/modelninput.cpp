#include "modelninput.h"
#include "msbase/utils/box2dgrid.h"
#include "msbase/primitive/primitive.h"
#include "msbase/mesh/merge.h"

#include "interface/appsettinginterface.h"
#include "internal/multi_printer/printer.h"
#include "qtuser3d/trimesh2/conv.h"

namespace creative_kernel
{
	ModelNInput::ModelNInput(ModelN* model, Printer* printer, QObject* parent)
		:ModelInput(parent)
		, m_model(model)
	{
		settings->merge(m_model->setting());
		m_mesh = m_model->meshptr();
		m_xform = m_model->getMatrix();
	}

	ModelNInput::~ModelNInput()
	{
	}

	std::vector<double> ModelNInput::layerHeightProfile()
	{
		return m_model->layerHeightProfile();
	}

}