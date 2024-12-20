#include "pickbottomop.h"
#include <QtCore/QDebug>  

#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/eventinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/appsettinginterface.h"
#include "interface/spaceinterface.h"
#include "trimesh2/quaternion.h"
#include "qtuser3d/trimesh2/conv.h"

#include "interface/modelgroupinterface.h"
#include "interface/printerinterface.h"
#include "cxkernel/data/meshdata.h"

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
	creative_kernel::addModelNSelectorTracer(this);

	reGenerateFaces();
	creative_kernel::delayCapture(100);
}

void PickBottomOp::onDettach()
{
	removeFaces();

	creative_kernel::removeLeftMouseEventHandler(this);
	creative_kernel::removeHoverEventHandler(this);
	creative_kernel::removeModelNSelectorTracer(this);
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

	// emit mouseLeftClicked();
	// creative_kernel::delayCapture(50);
	creative_kernel::requestVisPickUpdate(false);
}

void PickBottomOp::onSelectionsBoxChanged()
{
	for (std::vector<PickFace*>& pickFaces : m_pickFaces)
	{
		for (PickFace* pickFace : pickFaces)
		{
			if (pickFace->model())
			{
				pickFace->setPose(pickFace->model()->globalMatrix());
			} else if (pickFace->modelGroup())
			{
				pickFace->setPose(pickFace->modelGroup()->globalMatrix());
			}
		}
	}

}

void PickBottomOp::reGenerateFaces()
{
	QList<creative_kernel::ModelN*> selections = creative_kernel::selectionms();
	if (selections.size() < 1)
		return;

	QVector4D hoverColor = CONFIG_PLUGIN_VEC4(pickbottom_hover_color, pickbottom_group);
	QVector4D noHoverColor = CONFIG_PLUGIN_VEC4(pickbottom_nohover_color, pickbottom_group);
	QList<qtuser_3d::Pickable*> pickables;
	QList<creative_kernel::ModelGroup*> groups = creative_kernel::selectedGroups();
	for (creative_kernel::ModelGroup* group : groups)
	{
		cxkernel::MeshDataPtr meshDataPtr = group->meshDataWithHullFaces(false);

		std::vector<PickFace*> pickFaces;
		for (const cxkernel::KernelHullFace& face : meshDataPtr->faces)
		{
			PickFace* faceEntity = new PickFace(group, face, nullptr);
			faceEntity->setHoverColor(hoverColor);
			faceEntity->setColor(noHoverColor);
			faceEntity->setParent(group->qEntity());
			// creative_kernel::spaceShow(faceEntity);
			faceEntity->setModelMatrix(group->globalMatrix());
			pickables << faceEntity->pickable();
			pickFaces.push_back(faceEntity);
		}
		m_pickFaces.push_back(pickFaces);
	}

	QList<creative_kernel::ModelN*> parts = creative_kernel::selectedParts();
	for (creative_kernel::ModelN* model : parts)
	{
		model->data()->calculateFaces();

		const std::vector<cxkernel::KernelHullFace>& faces = model->data()->faces;
		std::vector<PickFace*> pickFaces;
		for (const cxkernel::KernelHullFace& face : faces)
		{
			PickFace* faceEntity = new PickFace(model, face, nullptr);
			faceEntity->setHoverColor(hoverColor);
			faceEntity->setColor(noHoverColor);
			faceEntity->setParent(model->qEntity());
			faceEntity->setModelMatrix(model->globalMatrix());
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

	trimesh::dvec3 normal = trimesh::dvec3(face->gNormal());
	
	creative_kernel::ModelN* model = face->model();
	if (model)
	{
		QList<creative_kernel::ModelN*> models;
		models << model;
		creative_kernel::setModelNsFaceBottom(models, normal);
	}

	creative_kernel::ModelGroup* modelGroup = face->modelGroup();
	if (modelGroup)
	{
		QList<creative_kernel::ModelGroup*> groups;
		groups << modelGroup;

		creative_kernel::setModelGroupsFaceBottom(groups, normal);
	}

	for (std::vector<PickFace*> faces : m_pickFaces)
	{
		for (PickFace* face : faces)
		{
			if (face->model())
			{
				face->setModelMatrix(face->model()->globalMatrix());
			}
			else if (face->modelGroup())
			{
				face->setModelMatrix(face->modelGroup()->globalMatrix());
			}
		}
	}

	creative_kernel::requestVisUpdate(true);
	//creative_kernel::requestVisPickUpdate(true);
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