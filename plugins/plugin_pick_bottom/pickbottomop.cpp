#include "pickbottomop.h"
#include <QtCore/QDebug>  

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/modelinterface.h"
#include "interface/eventinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/spaceinterface.h"

#include "trimesh2/quaternion.h"
#include "qtuser3d/trimesh2/conv.h"

#include "interface/printerinterface.h"

PickBottomOp::PickBottomOp(QObject* parent)
	:MoveOperateMode(parent)
{
	m_type = qtuser_3d::SceneOperateMode::FixedMode;
}

PickBottomOp::~PickBottomOp()
{
}

void PickBottomOp::onAttach()
{
	creative_kernel::addLeftMouseEventHandler(this);
	creative_kernel::addHoverEventHandler(this);
	creative_kernel::addSelectTracer(this);

	creative_kernel::delayCapture(100);
}

void PickBottomOp::onDettach()
{
	removeFaces();

	creative_kernel::removeLeftMouseEventHandler(this);
	creative_kernel::removeHoverEventHandler(this);
	creative_kernel::removeSelectorTracer(this);
}

void PickBottomOp::onLeftMouseButtonClick(QMouseEvent* event)
{
	int primitiveID = -1;
	qtuser_3d::Pickable* pickable = creative_kernel::checkPickable(event->pos(), &primitiveID);
	if (pickable && pickable->parent())
	{
		PickFace* face = qobject_cast<PickFace*>(pickable->parent());
		executeFace(face);
	}
}

void PickBottomOp::onHoverMove(QHoverEvent* event)
{
	QList<creative_kernel::ModelN*> selections = creative_kernel::selectionms();
	if (selections.size() < 1)
		return;

	creative_kernel::requestVisUpdate(false);
}

void PickBottomOp::onSelectionsChanged()
{
	removeFaces();
	reGenerateFaces();

	emit mouseLeftClicked();
	creative_kernel::delayCapture(50);
}

void PickBottomOp::reGenerateFaces()
{
	QList<creative_kernel::ModelN*> selections = creative_kernel::selectionms();
	if (selections.size() < 1)
		return;

	QList<qtuser_3d::Pickable*> pickables;
	for (creative_kernel::ModelN* model : selections)
	{
		QVector4D hoverColor = CONFIG_PLUGIN_VEC4(pickbottom_hover_color, pickbottom_group);
		QVector4D noHoverColor = CONFIG_PLUGIN_VEC4(pickbottom_nohover_color, pickbottom_group);
		
		model->data()->calculateFaces();

		const std::vector<cxkernel::KernelHullFace>& faces = model->data()->faces;
		std::vector<PickFace*> pickFaces;
		for (const cxkernel::KernelHullFace& face : faces)
		{
			PickFace* faceEntity = new PickFace(model, face, nullptr);
			faceEntity->setHoverColor(hoverColor);
			faceEntity->setColor(noHoverColor);
			faceEntity->setParent(model->getModelEntity());
			// creative_kernel::tracePickable(faceEntity->pickable());
			pickables << faceEntity->pickable();
			pickFaces.push_back(faceEntity);
		}
		m_pickFaces.push_back(pickFaces);
	}
	creative_kernel::addPickables(pickables);
}

void PickBottomOp::removeFaces()
{
	 QList<qtuser_3d::Pickable*> pickables;
	 for (auto pickFaces : m_pickFaces)
	 {
	 	for (PickFace* apickFace : pickFaces)
	 		pickables << apickFace->pickable();
	 }
	 creative_kernel::removePickables(pickables);

	 for (auto pickFaces : m_pickFaces)
	 {
	 	qDeleteAll(pickFaces);
	 	pickFaces.clear();
	 }
	 m_pickFaces.clear();

}

void PickBottomOp::executeFace(PickFace* face)
{
	if (!face)
		return;

	QVector3D normal = qtuser_3d::vec2qvector(face->gNormal());
	creative_kernel::rotateModelNormal2Bottom(face->model(), normal);
}

void PickBottomOp::setMaxFaceBottom()
{
	/*for (auto pickFaces : m_pickFaces)
	{
		if (pickFaces.size() > 0)
		{
			PickFace* face = pickFaces.at(0);
			executeFace(face);
		}
	}*/

	creative_kernel::setModelsMaxFaceBottomExceptLock();

	creative_kernel::delayCapture(50);
}

void PickBottomOp::selectChanged(qtuser_3d::Pickable* pickable)
{
}

bool PickBottomOp::getMessage()
{
	return false;
}

void PickBottomOp::setMessage(bool isRemove)
{
	if (isRemove)
	{
		creative_kernel::requestVisUpdate(true);
	}
}