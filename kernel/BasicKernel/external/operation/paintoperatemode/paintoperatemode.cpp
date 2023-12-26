#include "paintoperatemode.h"
#include <QCoreApplication>
#include <QHoverEvent>
#include <QMouseEvent>
#include <QElapsedTimer>

#include "kernel/modelnselector.h"
#include "kernel/kernel.h"
#include "kernel/modelnselector.h"

#include "interface/selectorinterface.h"
#include "interface/camerainterface.h"
#include "interface/eventinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/renderinterface.h"
#include "interface/commandinterface.h"
#include "interface/modelinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/visualsceneinterface.h"

#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/utils/qtuser3denum.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "entity/outlineentity.h"

#include "data/modeln.h"
#include "data/modelspaceundo.h"

#include "trimesh2/TriMesh.h"
#include "cxkernel/data/trimeshutils.h"

#include "./colorexecutor/colorexecutor.h"
#include "colorundo/colorundocommand.h"
#include "colorloadjob.h"
#include "spread/meshspread.h"
#include "spreadmodel/spreadmodel.h"
#include "msbase/space/angle.h"


PaintOperateMode::PaintOperateMode(QObject* parent)
    : qtuser_3d::SceneOperateMode(parent)
  {
    qRegisterMetaType<std::vector<int>>("std::vector<int>");

	m_spreadModel = new SpreadModel();

    m_meshSpreadWrapper.reset(new spread::MeshSpreadWrapper());
    m_colorExecutor = new ColorExecutor(m_meshSpreadWrapper.get(), this);

    connect(m_colorExecutor, &ColorExecutor::sig_needUpdate, this, &PaintOperateMode::onGotNewData);
	connect(creative_kernel::getKernel(), &creative_kernel::Kernel::currentPhaseChanged, this, [=] ()
	{
		if (m_originModel != NULL)
		{
			creative_kernel::setVisOperationMode(NULL);
		}
	});

	setHighlightOverhangsDeg(m_highlightOverhangsDeg);
  }

PaintOperateMode::~PaintOperateMode()
{
	resetRouteNum(0);
	m_spreadModel->deleteLater();
}

bool PaintOperateMode::isRunning()
{
	return m_isRunning;
}

void PaintOperateMode::restore(creative_kernel::ModelN* model, const std::vector<std::string>& data)
{
	m_lastSpreadData = data;
	m_meshSpreadWrapper->set_triangle_from_data(data);
	m_meshSpreadWrapper->updateTriangle();
	updateSpreadModel();
}

std::vector<std::string> PaintOperateMode::getPaintData() const
{
	m_meshSpreadWrapper->updateData();
	return m_meshSpreadWrapper->get_data_as_string();
}

bool PaintOperateMode::isHoverModel(const QPoint& pos) 
{
	spread::SceneData data;
	return getSceneData(pos, data);
}

void PaintOperateMode::setWaitForLoadModel(bool enable)
{
	m_waitForLoadModel = enable;
}

/* 界面参数 */
QPoint PaintOperateMode::pos()
{
  return m_pos;
}

float PaintOperateMode::scale()
{
  return m_scale;
}

bool PaintOperateMode::pressed()
{
  return m_pressed;
}

float PaintOperateMode::sectionRate()
{
	return m_sectionRate;
}

void PaintOperateMode::setSectionRate(float rate)
{
	if (m_sectionRate != rate)
	{
		m_sectionRate = rate;
		updateSection();

		emit sectionRateChanged();
	}
}

float PaintOperateMode::colorRadius()
{
	return m_colorRadius;
}

void PaintOperateMode::setColorRadius(float radius)
{
	if (m_colorRadius != radius)
	{
		m_colorRadius = radius;

		emit colorRadiusChanged();
	}
}

int PaintOperateMode::colorMethod()
{
	return m_colorMethod; 
}

void PaintOperateMode::setColorMethod(int method)
{
	if (m_colorMethod == method)
		return;

	m_colorMethod = method;
	// m_originFillData = m_colorModel->getColors2Facets();
}

int PaintOperateMode::colorIndex()
{
	return m_colorIndex - 1;
}

void PaintOperateMode::setColorIndex(int index)
{
	m_colorIndex = index + 1;
	m_colorExecutor->setColorIndex(index + 1);
}

QVariantList PaintOperateMode::colorsList()
{
	QVariantList datasList;
	for (trimesh::vec color : m_colorsList)
		datasList << QVariant(QColor(color[0] * 255.0, color[1] * 255.0, color[2] * 255.0));

	return datasList;
}

void PaintOperateMode::setColorsList(const QVariantList& datasList)
{
	m_colorsList.clear();
	for (QVariant data : datasList)
	{
		QColor color = data.value<QColor>();
		m_colorsList.push_back(trimesh::vec(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0));
	}
  	onColorsListChanged();
}

void PaintOperateMode::setColorsList(const std::vector<trimesh::vec>& datasList)
{
	m_colorsList = datasList;
  	onColorsListChanged();
}

std::vector<trimesh::vec> PaintOperateMode::getColorsList()
{
  	return m_colorsList;
}

float PaintOperateMode::highlightOverhangsDeg()
{
	return m_highlightOverhangsDeg;
}

void PaintOperateMode::setHighlightOverhangsDeg(float rate)
{
	m_highlightOverhangsDeg = rate;
	m_spreadModel->setHighlightOverhangsDeg(m_highlightOverhangsDeg);
	m_meshSpreadWrapper->set_paint_on_overhangs_only(m_overHangsEnable ? m_highlightOverhangsDeg : 0);
	emit highlightOverhangsDegChanged();
}

bool PaintOperateMode::overHangsEnable()
{
	return m_overHangsEnable;
}

void PaintOperateMode::setOverHangsEnable(bool enable)
{
	m_overHangsEnable = enable;
	m_meshSpreadWrapper->set_paint_on_overhangs_only(m_overHangsEnable ? m_highlightOverhangsDeg : 0);
	emit overHangsEnableChanged();
}

void PaintOperateMode::resize()
{
  auto camera = creative_kernel::visCamera()->camera();
  m_scale = camera->fieldOfView();
  emit scaleChanged();
}

/* 剖面功能 */
void PaintOperateMode::resetDirection()
{
	m_sectionNormal = -creative_kernel::cameraController()->getViewDir().normalized();
	updateSectionBoundary();
	updateSection();
}

void PaintOperateMode::updateSectionBoundary()
{
	if (m_originModel == NULL)
		return;

	qtuser_3d::Box3D box = m_originModel->globalSpaceBox();
	QVector3D modelCenter = box.center();
	QVector3D diagonalLine = box.max - box.min;
	QVector3D boxCorners[8];
	boxCorners[0] = box.min + QVector3D(0.0, 0.0, diagonalLine.z());
	boxCorners[1] = box.min + QVector3D(0.0, diagonalLine.y(), diagonalLine.z());
	boxCorners[2] = box.min + QVector3D(diagonalLine.x(), 0.0, diagonalLine.z());
	boxCorners[3] = box.min + QVector3D(diagonalLine.x(), diagonalLine.y(), diagonalLine.z());
	boxCorners[4] = box.min + QVector3D(0.0, 0.0, 0.0);
	boxCorners[5] = box.min + QVector3D(0.0, diagonalLine.y(), 0.0);
	boxCorners[6] = box.min + QVector3D(diagonalLine.x(), 0.0, 0.0);
	boxCorners[7] = box.min + QVector3D(diagonalLine.x(), diagonalLine.y(), 0.0);

    float minProjection = std::numeric_limits<float>::infinity();
    float maxProjection = -std::numeric_limits<float>::infinity();
    int minIndex = -1;
    int maxIndex = -1;

    for (int i = 0; i < 8; ++i) {
        float projection = QVector3D::dotProduct(boxCorners[i], m_sectionNormal);

        if (projection < minProjection) {
            minProjection = projection;
            minIndex = i;
        }
        if (projection > maxProjection) {
            maxProjection = projection;
            maxIndex = i;
        }
    }
	m_frontPos = boxCorners[minIndex];
	m_backPos = boxCorners[maxIndex];
	m_sectionPos = m_frontPos;
}

void PaintOperateMode::updateSection()
{
	if (m_sectionNormal.isNull()) 
		resetDirection();

	float distance = m_backPos.distanceToPlane(m_frontPos, m_sectionNormal);
	m_sectionPos = m_frontPos + distance * (1.0 - m_sectionRate) * m_sectionNormal;
	m_spreadModel->setSection(m_sectionPos, m_backPos, m_sectionNormal);	

	qtuser_3d::Box3D localBox = m_originModel->localBox();
	qtuser_3d::Box3D globalBox = m_originModel->globalSpaceBox();
	float x = localBox.max.x() - localBox.min.x();
	float y = localBox.max.y() - localBox.min.y();
	float z = localBox.max.z() - localBox.min.z();
	float radius = 0.5 * sqrt(x * x + y * y + z * z);
	QVector3D center = globalBox.center();
	float dist = QVector3D::dotProduct(m_sectionNormal, center);
	//m_offset = (dist - (-radius) - (-0.25) * 2* radius);
	float clpOffset = (dist - (-radius) - m_sectionRate * 2 * radius);

	const QMatrix4x4 trafo = m_originModel->pose();
	const QMatrix4x4 trafo_normal(trafo.normalMatrix());
	const QMatrix4x4 trafo_inv = trafo.inverted();

	QVector3D point_on_plane = m_sectionNormal * clpOffset;
	QVector3D point_on_plane_transformed = trafo_inv * point_on_plane;
	QVector3D normal_transformed = trafo_normal * m_sectionNormal;
	m_sectionNormal = normal_transformed;
	m_offset = float(QVector3D::dotProduct(point_on_plane_transformed, (normal_transformed)));

}

/* 渲染 */
void PaintOperateMode::clearAllColor()
{
	m_meshSpreadWrapper->reset();
	updateSpreadModel();
}

void PaintOperateMode::setEnabled(bool enabled)
{
	if (m_enabled == enabled)
		return;

	m_enabled = enabled;
	updateScene();
}

void PaintOperateMode::updateScene()
{
	if (m_enabled)
	{
		creative_kernel::useCustomScene();
		creative_kernel::setPrinterVisible(false);
		installEventHandlers();
	}
	else
	{
		creative_kernel::useDefaultScene();
		creative_kernel::setPrinterVisible(true);
		uninstallEventHandlers();
	}
	creative_kernel::requestVisUpdate(true);
}

void PaintOperateMode::resetRouteNum(int number)
{
	while (m_outlineEntitys.size() < number)
	{
		auto lineEntity = new qtuser_3d::OutlineEntity();
		m_outlineEntitys << lineEntity;
	}

	while (m_outlineEntitys.size() > number)
		m_outlineEntitys.takeLast()->deleteLater();
}

void PaintOperateMode::updateRoute(const std::vector<trimesh::vec3>& route)
{
	resetRouteNum(1);
	qtuser_3d::OutlineEntity* entity = m_outlineEntitys.first();
	entity->setRoute(route);
	entity->setPose(m_spreadModel->pose());
	// entity->setPose(m_colorModel->pose());
	creative_kernel::visShowCustom(entity);
}

void PaintOperateMode::updateRoute(const std::vector<std::vector<trimesh::vec3>>& routes)
{
	if (routes.size() == 0)
		return;

	resetRouteNum(1);
	auto entity = m_outlineEntitys.first();
	entity->setRoute(routes);
	entity->setPose(m_spreadModel->pose());
	creative_kernel::visShowCustom(entity);
}

void PaintOperateMode::hideRoute()
{
	m_meshSpreadWrapper->seed_fill_unselect_all_triangles();
	for (auto entity : m_outlineEntitys)
		creative_kernel::visHideCustom(entity);
}


void PaintOperateMode::chooseTriangle()
{
	spread::SceneData data;
	if (!getSceneData(pos(), data))
	{
		hideRoute();
		return;
	}
	std::vector<std::vector<trimesh::vec3>> contour;
	trimesh::vec normal(-m_sectionNormal.x(), -m_sectionNormal.y(), -m_sectionNormal.z());
	m_meshSpreadWrapper->bucket_fill_select_triangles_preview(data.center, data.faceId, m_colorIndex, contour, normal, m_offset, false);
	updateRoute(contour);
}

void PaintOperateMode::chooseFillArea()
{
	spread::SceneData data;
	if (!getSceneData(pos(), data))
	{
		hideRoute();
		return;
	}
	std::vector<std::vector<trimesh::vec3>> contour;
	trimesh::vec normal(-m_sectionNormal.x(), -m_sectionNormal.y(), -m_sectionNormal.z());
	m_meshSpreadWrapper->bucket_fill_select_triangles_preview(data.center, data.faceId, m_colorIndex, contour, normal, m_offset);
	updateRoute(contour);
}

QVector3D PaintOperateMode::getTriangleNormal(const QVector3D& v1, const QVector3D& v2, const QVector3D& v3) const
{
	QVector3D edge1 = v2 - v1;
	QVector3D edge2 = v3 - v1;
	return QVector3D::crossProduct(edge1, edge2).normalized();
}

static QVector3D calculateIntersection(const QVector3D& planePoint, const QVector3D& planeNormal,
	const QVector3D& rayStart, const QVector3D& rayDirection) {
	// 计算平面方程的常数项 D
	float D = -QVector3D::dotProduct(planeNormal, planePoint);

	// 计算射线方程和平面方程的交点参数 t
	float t = -(QVector3D::dotProduct(planeNormal, rayStart) + D) / QVector3D::dotProduct(planeNormal, rayDirection);

	// 计算交点
	QVector3D intersection = rayStart + t * rayDirection;

	return intersection;
}

QVector3D PaintOperateMode::mapToSectionArea(const QVector3D& position, const QVector3D& direction)
{
	if (QVector3D::dotProduct(m_sectionNormal, position - m_frontPos) < 0)
	{
		float D = -QVector3D::dotProduct(m_sectionNormal, m_sectionPos);
		float t = -(QVector3D::dotProduct(m_sectionNormal, position) + D) / QVector3D::dotProduct(m_sectionNormal, direction);
		QVector3D result = position + t * direction;
		return result;
	}
	else 
	{
		return position;
	}
}

bool PaintOperateMode::getSceneData(const QPoint& pos, spread::SceneData& data)
{
	qtuser_3d::ScreenCamera* screenCamera = creative_kernel::visCamera();
	qtuser_3d::Ray ray = screenCamera->screenRay(pos);
	QVector3D from = mapToSectionArea(ray.start, ray.dir);
	trimesh::vec rayOrigin(from.x(), from.y(), from.z());
	trimesh::vec rayDirection(ray.dir.x(), ray.dir.y(), ray.dir.z());
	trimesh::vec cross;
    int faceID = m_meshSpreadWrapper->getFacet(rayOrigin, rayDirection, cross);
	if (faceID == -1)
		return false;

	/* 配置SceneData参数 */ 
	data.center = cross;
	QVector3D cameraPos = creative_kernel::cameraController()->getViewPosition();
	data.cameraPos = trimesh::vec(cameraPos.x(), cameraPos.y(), cameraPos.z());
	data.radius = colorRadius();
	data.faceId = faceID;// 获取原始的faceID
	data.planeNormal = trimesh::vec(-m_sectionNormal.x(), -m_sectionNormal.y(), -m_sectionNormal.z());	
	data.planeOffset = m_offset;

	data.support_angle_threshold_deg = m_overHangsEnable ? m_highlightOverhangsDeg : 0;

	return true;
}

void PaintOperateMode::color(const QPoint &pos, int colorIndex, bool isFirstColor)
{
	m_colorExecutor->setColorIndex(colorIndex);

	if (m_colorMethod == spread::CursorType::POINTER)
	{
		m_colorExecutor->triangleColor();
	}
	else if (m_colorMethod == spread::CursorType::CIRCLE)
	{
		spread::SceneData data;
		if (!getSceneData(pos, data))
			return;
		m_colorExecutor->circleColor(data, isFirstColor);
	}
	else if (m_colorMethod == spread::CursorType::GAP_FILL)
	{
		m_colorExecutor->fillColor();
	}
}

void PaintOperateMode::updateSpreadModel(int chunkId)
{
	if (chunkId == -1)
	{
		int count = m_meshSpreadWrapper->chunkCount();
		for (int i = 0; i < count; ++i)
		{
			std::vector<trimesh::vec3> positions;
			std::vector<int> flags;
			std::vector<int> indexMap;
			m_meshSpreadWrapper->chunk(i, positions, flags, indexMap);
			m_spreadModel->updateChunkData(i, positions, flags);
			m_spreadModel->setChunkIndexMap(i, indexMap);
		}
	}
	else 
	{
		std::vector<trimesh::vec3> positions;
		std::vector<int> flags;
		std::vector<int> indexMap;
		m_meshSpreadWrapper->chunk(chunkId, positions, flags, indexMap);
		m_spreadModel->updateChunkData(chunkId, positions, flags);
		m_spreadModel->setChunkIndexMap(chunkId, indexMap);
	}

	creative_kernel::updateFaceBases();
	creative_kernel::requestVisPickUpdate(false);
}

void PaintOperateMode::choose(const QPoint& pos)
{
	if (m_colorMethod == spread::CursorType::POINTER)
		chooseTriangle();
	else if(m_colorMethod == spread::CursorType::GAP_FILL)
		chooseFillArea();
	else 
		hideRoute();

	if (pressed())
		color(pos, m_erase ? 1 : m_colorIndex, false);
}

void PaintOperateMode::onColorsListChanged()
{
	m_spreadModel->setPalette(m_colorsList);
}

void PaintOperateMode::onGotNewData(std::vector<int> dirtyChunks)
{
	for (auto chunkId : dirtyChunks)
	{
		std::vector<trimesh::vec3> positions; 
		std::vector<int> flags;
		std::vector<int> indexMap;
		m_meshSpreadWrapper->chunk(chunkId, positions, flags, indexMap);
		m_spreadModel->updateChunkData(chunkId, positions, flags);
		m_spreadModel->setChunkIndexMap(chunkId, indexMap);
	}
}

/**** override ***/
void PaintOperateMode::installEventHandlers()
{
	creative_kernel::addHoverEventHandler(this);
	creative_kernel::addLeftMouseEventHandler(this);
	creative_kernel::prependRightMouseEventHandler(this);
	creative_kernel::prependMidMouseEventHandler(this);
	creative_kernel::prependWheelEventHandler(this);
}

void PaintOperateMode::uninstallEventHandlers()
{
	creative_kernel::removeHoverEventHandler(this);
	creative_kernel::removeLeftMouseEventHandler(this);
	creative_kernel::removeRightMouseEventHandler(this);
	creative_kernel::removeMidMouseEventHandler(this);
	creative_kernel::removeWheelEventHandler(this);
}

void PaintOperateMode::onAttach() 
{
	creative_kernel::addSelectTracer(this);
	connect(creative_kernel::cameraController(), &qtuser_3d::CameraController::signalViewChanged, this, &PaintOperateMode::resize);	

	creative_kernel::setContinousRender();

	resize();
	m_isWrapperReady = false;
	m_originModel = NULL;
	auto selections = creative_kernel::selectionms();
	if (selections.isEmpty())
		return;

	m_originModel = selections.at(0);

	m_spreadModel->init();
	m_spreadModel->setPalette(m_colorsList);
	m_spreadModel->setRenderModel(m_originModel->getVisualMode());

	/* 初始化m_meshSpreadWrapper */
	m_colorLoadJob = new ColorLoadJob(this);
	m_colorLoadJob->setMeshSpreadWrapper(m_meshSpreadWrapper.get());
	m_colorLoadJob->setTriMesh(m_originModel->globalMesh());
	m_colorLoadJob->setColorsList(getColorsList());
	m_colorLoadJob->setData(m_lastSpreadData);
	cxkernel::executeJob(qtuser_core::JobPtr(m_colorLoadJob));
	connect(m_colorLoadJob, &ColorLoadJob::sig_finished, this, [=] ()
	{		
		int count = m_meshSpreadWrapper->chunkCount();
		for (int i = 0; i < count; ++i)
		{
			std::vector<trimesh::vec3> positions;
			std::vector<int> flags;
			std::vector<int> indexMap;
			m_meshSpreadWrapper->chunk(i, positions, flags, indexMap);
			m_spreadModel->addChunk(positions, flags);
			m_spreadModel->setChunkIndexMap(i, indexMap);
		}
		m_isWrapperReady = true;
		
		updateScene();
		updateSectionBoundary();
		updateSection();
		creative_kernel::updateFaceBases();
		creative_kernel::requestVisUpdate(true);
	});
	if (m_waitForLoadModel)
		m_colorLoadJob->wait();
	
	m_isRunning = true;
	emit enter();
}

void PaintOperateMode::onDettach() 
{
	uninstallEventHandlers();

	creative_kernel::removeSelectorTracer(this);
	// creative_kernel::setSelectorEnable(true);
	creative_kernel::useDefaultScene();
	creative_kernel::setPrinterVisible(true);
	creative_kernel::resetModelSection();	

  	disconnect(creative_kernel::cameraController(), &qtuser_3d::CameraController::signalViewChanged, this, &PaintOperateMode::resize);

	creative_kernel::setCommandRender();
	m_spreadModel->clear();
	hideRoute();
	m_originModel = NULL;

	creative_kernel::requestVisUpdate(true);
	
	m_isRunning = false;
	emit quit();
}

void PaintOperateMode::onHoverMove(QHoverEvent* event) 
{
	if (m_pos == event->pos()) 
		return;

	m_pressed = false;
    m_pos = event->pos();
    emit posChanged();
    choose(m_pos);
}

void PaintOperateMode::onLeftMouseButtonMove(QMouseEvent* event) 
{
  	if (m_pos == event->pos()) 
		return;
	// m_pressed = event->buttons() == Qt::LeftButton || event->buttons() == Qt::RightButton;
	// m_erase = event->modifiers() == Qt::ShiftModifier;
    m_pos = event->pos();
    emit posChanged();
    choose(m_pos);
}

void PaintOperateMode::onLeftMouseButtonPress(QMouseEvent* event) 
{
	m_pressed = true;
	m_erase = event->modifiers() == Qt::ShiftModifier;

	QPoint pos = event->pos();
	color(pos, m_erase ? 1 : m_colorIndex, true);
}
	
void PaintOperateMode::onLeftMouseButtonRelease(QMouseEvent* event) 
{
	creative_kernel::updateFaceBases();
	creative_kernel::requestVisPickUpdate(true);
	m_pressed = false;
	m_erase = event->modifiers() == Qt::ShiftModifier;

	m_meshSpreadWrapper->updateData();
	auto currentSpreadData = m_meshSpreadWrapper->get_data_as_string();

	bool isModified = false;
	if (currentSpreadData.size() != m_lastSpreadData.size())
		return;
	for (int i = 0, count = m_lastSpreadData.size(); i < count; ++i)
	{
		const std::string& s1 = m_lastSpreadData[i];
		const std::string& s2 = currentSpreadData[i];
		if (s2 != s1)
		{
			isModified = true;
			break;
		}
	}
	if (!isModified)
		return;

	creative_kernel::ModelSpaceUndo* stack = creative_kernel::getKernel()->modelSpaceUndo();
	ColorUndoCommand* command = new ColorUndoCommand();
	command->setObjects(this, m_originModel);
	command->setData(m_lastSpreadData, currentSpreadData);

	m_lastSpreadData = currentSpreadData;

	stack->push(command);

}
	
void PaintOperateMode::onRightMouseButtonMove(QMouseEvent* event) 
{
  	if (m_pos == event->pos()) 
		return;

    m_pos = event->pos();
    emit posChanged();
}

void PaintOperateMode::onMidMouseButtonMove(QMouseEvent* event) 
{
  	if (m_pos == event->pos()) 
		return;

    m_pos = event->pos();
    emit posChanged();
}

void PaintOperateMode::onWheelEvent(QWheelEvent* event) 
{
	if (event->modifiers() == Qt::ShiftModifier)
	{
		bool plus = event->angleDelta().y() > 0;
		float rate = sectionRate();
		if (plus)
		{
			rate += 0.05;
			rate = rate > 1.0 ? 1.0 : rate;
		}
		else 
		{
			rate -= 0.05;
			rate = rate < 0.0 ? 0.0 : rate;
		}
		setSectionRate(rate);
		m_wheelStatus = false;
	}
	else if (event->modifiers() == Qt::ControlModifier && m_colorMethod == spread::CursorType::CIRCLE)
	{
		bool plus = event->angleDelta().y() > 0;
		float radius = colorRadius();
		if (plus)
		{
			radius += 0.5;
			radius = radius > 8.0 ? 8.0 : radius;
		}
		else 
		{
			radius -= 0.5;
			radius = radius < 0.1 ? 0.1 : radius;
		}
		setColorRadius(radius);
		m_wheelStatus = false;
	}
}

void PaintOperateMode::onSelectionsChanged()
{
	if (m_originModel == NULL)
		return;

	creative_kernel::setVisOperationMode(NULL);
}