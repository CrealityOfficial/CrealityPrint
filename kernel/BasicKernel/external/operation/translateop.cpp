#include "translateop.h"
#include <QQmlProperty>
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/camera/cameracontroller.h"
#include <QMetaObject>
#include "interface/visualsceneinterface.h"
#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/modelinterface.h"
#include "interface/printerinterface.h"

#include "interface/eventinterface.h"
#include "interface/reuseableinterface.h"
#include "kernel/kernelui.h"

#include "internal/multi_printer/printer.h"

using namespace creative_kernel;
TranslateOp::TranslateOp(QObject* parent)
	:MoveOperateMode(parent)
{
	int types =
		HELPERTYPE_AXIS_X | \
		HELPERTYPE_AXIS_Y | \
		HELPERTYPE_AXIS_Z | \
		HELPERTYPE_PLANE_XY | \
		HELPERTYPE_PLANE_YZ | \
		HELPERTYPE_PLANE_ZX;

    m_type = qtuser_3d::SceneOperateMode::ReusableMode;
	m_helperEntity = new qtuser_3d::TranslateHelperEntity(nullptr, types);
	m_helperEntity->setCameraController(cameraController());

}

TranslateOp::~TranslateOp()
{
}

void TranslateOp::setMessage(bool isRemove)
{
	if (isRemove)
	{
		requestVisUpdate(true);
	}
}

bool TranslateOp::getMessage()
{
	return false;
}

void TranslateOp::onAttach()
{
	m_helperEntity->attach();

	tracePickable(m_helperEntity->xArrowPickable());
	tracePickable(m_helperEntity->yArrowPickable());
	tracePickable(m_helperEntity->zArrowPickable());
	tracePickable(m_helperEntity->xyPlanePickable());
	tracePickable(m_helperEntity->yzPlanePickable());
	tracePickable(m_helperEntity->zxPlanePickable());
	tracePickable(m_helperEntity->xyzCubePickable());

	addSelectTracer(this);
	onSelectionsChanged();

	addLeftMouseEventHandler(this);
	addWheelEventHandler(this);
	addKeyEventHandler(this);

	requestVisUpdate(true);

}

void TranslateOp::onDettach()
{
	m_helperEntity->detach();

	unTracePickable(m_helperEntity->xArrowPickable());
	unTracePickable(m_helperEntity->yArrowPickable());
	unTracePickable(m_helperEntity->zArrowPickable());
	unTracePickable(m_helperEntity->xyPlanePickable());
	unTracePickable(m_helperEntity->yzPlanePickable());
	unTracePickable(m_helperEntity->zxPlanePickable());
	unTracePickable(m_helperEntity->xyzCubePickable());

	visHide(m_helperEntity);

	removeSelectorTracer(this);	

	removeLeftMouseEventHandler(this);
	removeWheelEventHandler(this);
	removeKeyEventHandler(this);

	requestVisUpdate(true);
}

void TranslateOp::onLeftMouseButtonPress(QMouseEvent* event)
{
	MoveOperateMode::onLeftMouseButtonPress(event);
	setNeedCheckScope(0);

	auto models = selectionms();
	if (models.isEmpty())
		return;

	qtuser_3d::Pickable* pickable = checkPickable(event->pos(), nullptr);
	if (pickable == m_helperEntity->xArrowPickable()) 
		setMode(TMode::x);
	else if (pickable == m_helperEntity->yArrowPickable()) 
		setMode(TMode::y);
	else if (pickable == m_helperEntity->zArrowPickable()) 
		setMode(TMode::z);
	else if (pickable == m_helperEntity->xyPlanePickable()) 
		setMode(TMode::xy);
	else if (pickable == m_helperEntity->yzPlanePickable()) 
		setMode(TMode::yz);
	else if (pickable == m_helperEntity->zxPlanePickable()) 
		setMode(TMode::zx);
	resetSpacePoint(event->pos());
}

void TranslateOp::onLeftMouseButtonRelease(QMouseEvent* event)
{
	MoveOperateMode::onLeftMouseButtonRelease(event);
}

void TranslateOp::onLeftMouseButtonMove(QMouseEvent* event)
{
	MoveOperateMode::onLeftMouseButtonMove(event);
	setNeedCheckScope(0);
}

void TranslateOp::onLeftMouseButtonClick(QMouseEvent* event)
{
}

void TranslateOp::onWheelEvent(QWheelEvent* event)
{
}

void TranslateOp::onKeyPress(QKeyEvent* event)
{
	// QList<creative_kernel::ModelN*> selections = selectionms();
	// if (selections.size() == 0)
	// 	return;

	// QVector3D delta;
	// bool needUpdate = false;

	// if (event->key() == Qt::Key_D)
	// {
	// 	delta = QVector3D(1.0f, 0.0f, 0.0f);
	// 	needUpdate = true;
	// }
	// else if (event->key() == Qt::Key_A)
	// {
	// 	delta = QVector3D(-1.0f, 0.0f, 0.0f);
	// 	needUpdate = true;
	// }
	// else if (event->key() == Qt::Key_W)
	// {
	// 	delta = QVector3D(0.0f, 1.0f, 0.0f);
	// 	needUpdate = true;
	// }
	// else if (event->key() == Qt::Key_S)
	// {
	// 	delta = QVector3D(0.0f, -1.0f, 0.0f);
	// 	needUpdate = true;
	// }

	// if (needUpdate)
	// {
	// 	for (size_t i = 0; i < selections.size(); i++)
	// 	{
	// 		QVector3D localPosition = selections.at(i)->localPosition();
	// 		moveModel(selections.at(i), localPosition, localPosition + delta);
	// 	}
	// 	requestVisUpdate(true);
	// 	emit positionChanged();
	// }
}

void TranslateOp::onKeyRelease(QKeyEvent* event)
{
}

void TranslateOp::onSelectionsChanged()
{
	QList<creative_kernel::ModelN*> selections = selectionms();
	setSelectedModel(selections);
	emit mouseLeftClicked();
}

void TranslateOp::selectChanged(qtuser_3d::Pickable* pickable)
{
	auto models = creative_kernel::selectionms();
	for (auto m : models)
	{
		if (pickable == m)
			updateHelperEntity();
	}
	if (models.size() > 0)
	{
		emit positionChanged();
	}
}

void TranslateOp::setSelectedModel(creative_kernel::ModelN* model)
{
#if 0
	TranslateMode* mode = qobject_cast<TranslateMode*>(parent());
	if (!m_selectedModel)
	{
		mode->setSource("qrc:/kernel/qml/NoSelect.qml");
	}
	else {
		mode->setSource("qrc:/kernel/qml/MovePanel.qml");
	}

	qtuser_qml::ToolCommandCenter* left = getUILCenter();
	left->changeCommand(mode);
#endif
	buildFromSelections();
}

void TranslateOp::setSelectedModel(QList<creative_kernel::ModelN*> models)
{
	//auto models = creative_kernel::selectionms();
	//for (auto m : models)
	{
		//m_saveLocalPositions.push_back(m->localPosition());

	}
	buildFromSelections();
}

void TranslateOp::buildFromSelections()
{
	auto models = selectionms();
	if (models.size())
	{
		updateHelperEntity();
		visShow(m_helperEntity);
	}
	else
	{
		visHide(m_helperEntity);
	}

	requestVisPickUpdate(true);
	requestVisUpdate(true);
	if (models.size())
	{
		emit positionChanged();
	}
		
}
void TranslateOp::center()
{
	qtuser_3d::Box3D modelsbox;
	auto models = creative_kernel::selectionms();
	for (auto m : models)
	{
		qtuser_3d::Box3D modelbox = m->globalSpaceBox();
		modelsbox += modelbox;
	}
	QVector3D movePos;
	float newZ = 0;
	if (modelsbox.valid)
	{
		Printer* printer = getSelectedPrinter();
		qtuser_3d::Box3D box = printer->globalBox();
		QVector3D size = box.size();
		QVector3D center = box.center();
		newZ = center.z() - size.z() / 2.0f;

		movePos = box.center() - modelsbox.center();
		movePos.setZ(0);
	}

	for (auto m : models)
	{
		QVector3D oldLocalPosition = m->localPosition();
		QVector3D newPositon = oldLocalPosition + movePos;
		//newPositon.setZ(newZ);

		moveModel(m, oldLocalPosition, newPositon, true);

		updateHelperEntity();
		requestVisUpdate(true);
	}

	emit positionChanged();
	creative_kernel::checkModelCollision();
}

void TranslateOp::movePositionToinit(QList<creative_kernel::ModelN*>& selections)
{
    for (size_t i = 0; i < selections.size(); i++)
    {
        QVector3D oldLocalPosition = selections.at(i)->localPosition();
        QVector3D initPosition = selections.at(i)->GetInitPosition();
        initPosition.setZ(0.0f);
        QVector3D position = selections.at(i)->mapGlobal2Local(initPosition);
        position.setX(initPosition.x());
        position.setY(initPosition.y());

        moveModel(selections.at(i), oldLocalPosition, position, true);
    }
}

void TranslateOp::reset()
{
	QList<ModelN*> selections = selectionms();
	if(selections.size())
	{
        movePositionToinit(selections);
	}
	else
	{
		ModelSpace* space = getModelSpace();
		QList<ModelN*> models = space->modelns();

        movePositionToinit(models);
	}
	updateHelperEntity();
	requestVisUpdate(true);
	emit positionChanged();

	creative_kernel::checkModelCollision();
}

QVector3D TranslateOp::position()
{
	auto models = selectionms();
	if (models.size() > 0)
	{
		qtuser_3d::Box3D box = models[models.size() - 1]->globalSpaceBox();
		QVector3D size = box.size();
		QVector3D center = box.center();
		center.setZ(center.z() - size.z() / 2.0f);
		//end
		return center;
	}
    return QVector3D(0.0f, 0.0f, 0.0f);
}

void TranslateOp::setPosition(QVector3D position)
{
	auto models = creative_kernel::selectionms();
	if (models.empty())
		return;

	auto model = models.last();
	QVector3D src = model->localPosition();
	QVector3D dst = model->mapGlobal2Local(position);

	if (1 == models.size())
	{
		moveModel(model, src, dst, true);
		updateHelperEntity();
		requestVisUpdate(true);
		emit positionChanged();
	}
	else
	{
		QVector3D moveOffset = dst - src;
		for (auto m : models)
		{
			QVector3D pos = m->localPosition();
            moveModel(m, pos, pos + moveOffset, true);
		}

		updateHelperEntity();
		requestVisUpdate(true);
		emit positionChanged();
	}

	creative_kernel::checkModelCollision();
}

void TranslateOp::setBottom()
{
	auto models = creative_kernel::selectionms();
	for (auto m : models)
    {
        qtuser_3d::Box3D box = m->globalSpaceBox();

		float fOffset = -box.min.z();

        QVector3D zoffset = QVector3D(0.0f, 0.0f, fOffset);
        QVector3D localPosition = m->localPosition();

        moveModel(m, localPosition, localPosition + zoffset, true);
    }
    if(models.size())
    {
        updateHelperEntity();
        requestVisUpdate(true);
        emit positionChanged();
    }

	creative_kernel::checkModelCollision();
}

void TranslateOp::updateHelperEntity()
{
	auto models = selectionms();
	if (models.size())
	{
		qtuser_3d::Box3D box = creative_kernel::modelsBox(models);
		m_helperEntity->updateBox(box);
	}
}

bool TranslateOp::shouldMultipleSelect()
{
	return true;
}