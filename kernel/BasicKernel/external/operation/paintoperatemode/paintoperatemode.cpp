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
#include "interface/printerinterface.h"

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
#include "external/kernel/kernelui.h"

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
	connect(this, &PaintOperateMode::colorMethodChanged, this, [=] ()
	{
		m_spreadModel->setNeedMixEnabled(m_colorMethod == 3);
	});
	connect(this, &PaintOperateMode::colorMethodChanged, this, &PaintOperateMode::onTriggerGapFill);
	connect(this, &PaintOperateMode::gapAreaChanged, this, &PaintOperateMode::onTriggerGapFill);

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
	// qDebug() << "******* restore *******";
	// for (int i = 0, count = m_lastSpreadData.size(); i < count; ++i) 
	// {
	// 	if (m_lastSpreadData[i].size() > 0) 
	// 	{
	// 		qDebug() << i << m_lastSpreadData[i].data();
	// 	}
	// }
	m_meshSpreadWrapper->set_triangle_from_data(data);
	m_meshSpreadWrapper->updateTriangle();
	updateSpreadModel();

	onTriggerGapFill();
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
	return 1.0 - m_sectionRate;
}

void PaintOperateMode::setSectionRate(float rate)
{
	rate = 1.0 - rate;
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

		if (m_originModel)
			radiusMapToScreen(m_originModel->localPosition());
	}
}

float PaintOperateMode::screenRadius()
{
	return m_screenRadius;
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
	emit colorMethodChanged();
}

int PaintOperateMode::colorIndex()
{
	return m_colorIndex - 1;
}

void PaintOperateMode::setColorIndex(int index)
{
	index = index + 1;
	if (m_colorIndex != index) 
	{
		m_colorIndex = index;
		m_colorExecutor->setColorIndex(index);
		emit colorIndexChanged();
	}
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
	std::vector<trimesh::vec3> colors;
	for (QVariant data : datasList)
	{
		QColor color = data.value<QColor>();
		colors.push_back(trimesh::vec(color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0));
	}
	handleColorsListChanged(colors);
  	onColorsListChanged();
}

void PaintOperateMode::setColorsList(const std::vector<trimesh::vec>& datasList)
{
	handleColorsListChanged(datasList);
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

	float deg = actualUsedHighlightHangsAngle();
	m_meshSpreadWrapper->set_paint_on_overhangs_only(deg);
	
	emit highlightOverhangsDegChanged();
}

bool PaintOperateMode::overHangsEnable()
{
	return m_overHangsEnable;
}

void PaintOperateMode::setOverHangsEnable(bool enable)
{
	m_overHangsEnable = enable;
	float deg = actualUsedHighlightHangsAngle();
	m_meshSpreadWrapper->set_paint_on_overhangs_only(deg);
	emit overHangsEnableChanged();
}

float PaintOperateMode::smartFillAngle()
{
	return m_smartFillAngle;
}

void PaintOperateMode::setSmartFillAngle(float angle)
{
	m_smartFillAngle = angle;
	emit smartFillAngleChanged();
}

bool PaintOperateMode::smartFillEnable()
{
	return m_smartFillEnable;
}

void PaintOperateMode::setSmartFillEnable(bool enable)
{
	m_smartFillEnable = enable;
	emit smartFillEnableChanged();
}

float PaintOperateMode::heightRange()
{
	return m_heightRange;
}

void PaintOperateMode::setHeightRange(float heightRange)
{
	m_heightRange = heightRange;
	emit heightRangeChanged();
}

float PaintOperateMode::gapArea()
{
	return m_gapArea;
}

void PaintOperateMode::setGapArea(float areaSize)
{
	m_gapArea = areaSize;
	emit gapAreaChanged();
}

float PaintOperateMode::radiusMapToScreen(QVector3D center)
{
	if (!m_hoverOnModel) 
	{
		if (m_screenRadius != 0)
		{
			m_screenRadius = 0;
			emit screenRadiusChanged();
		}
		return 0;
	}

	auto camera = creative_kernel::cameraController()->screenCamera()->camera();
	QVector3D viewPos = creative_kernel::cameraController()->getviewCenter();
	QVector3D viewDir = creative_kernel::cameraController()->getViewDir();
	//QVector3D position = ray.start;
	//QVector3D dir = ray.dir.normalized();
	QVector3D nearCenter = viewPos + viewDir * camera->nearPlane();
	QVector3D leftDir = QVector3D::crossProduct(viewDir, camera->upVector()).normalized();
	QVector3D leftPos = center + leftDir * m_colorRadius;

	qtuser_3d::ScreenCamera* sc = creative_kernel::visCamera();
	const QMatrix4x4 vp = sc->projectionMatrix() * sc->viewMatrix();

	QVector4D clipO = vp * QVector4D(center, 1.0);
	QVector3D ndcO = QVector3D(clipO) / clipO.w();
	QVector2D uvO = QVector2D(ndcO);

	QVector4D clipP = vp * QVector4D(leftPos, 1.0);
	QVector3D ndcP = QVector3D(clipP) / clipP.w();
	QVector2D uvP = QVector2D(ndcP);

	QVector2D screenCross = uvO * QVector2D(sc->size().width(), sc->size().height());
	QVector2D radiusLine = (uvP - uvO) * QVector2D(sc->size().width(), sc->size().height());

	m_screenRadius = radiusLine.length() / 2;
	emit screenRadiusChanged();
	return m_screenRadius;
}

float PaintOperateMode::actualUsedHighlightHangsAngle()
{
	float deg = 0;
	if (m_overHangsEnable)
	{
		deg = (m_highlightOverhangsDeg == 90 ? 89.9 : m_highlightOverhangsDeg);
	}
	return deg;
}

void PaintOperateMode::resize()
{
  auto camera = creative_kernel::visCamera()->camera();
  m_scale = camera->fieldOfView();
  emit scaleChanged();

  if (m_originModel)
  	choose(pos());
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
	m_backPos = boxCorners[maxIndex]; //偏移5避免误差
	m_sectionPos = m_frontPos;
}

void PaintOperateMode::updateSection()
{
	if (m_sectionNormal.isNull()) 
		resetDirection();

	/* 
	 * note: The following "sectionPosOffset" and "backPosOffset"
	 * are to avoid errors in boundary value calculation. 
	 */
	float distance = m_backPos.distanceToPlane(m_frontPos, m_sectionNormal);
	m_sectionPos = m_frontPos + distance * (1.0 - m_sectionRate) * m_sectionNormal;

	float sectionPosOffset = 0;
	if (m_sectionRate == 1.0)
		sectionPosOffset = -5;

	float backPosOffset = 5;
	if (m_sectionRate == 0.0)
		backPosOffset = 0;

	m_spreadModel->setSection(m_sectionPos + sectionPosOffset * m_sectionNormal, m_backPos + backPosOffset * m_sectionNormal, m_sectionNormal);

	QVector3D _m_sectionNormal = -m_sectionNormal;
	float d1 = QVector3D::dotProduct(_m_sectionNormal, m_frontPos);
	float d2 = QVector3D::dotProduct(_m_sectionNormal, m_backPos);
	m_offset = d2 + (d1 - d2) * m_sectionRate;
	m_normal = _m_sectionNormal;
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
		creative_kernel::hidePrinter();
		updateSectionBoundary();
		updateSection();
		installEventHandlers();
		
		static bool firstStart = true;
		if (firstStart)
		{
			///* trigger render */
			firstStart = false;

			std::vector<trimesh::vec3> line;
			line.push_back(trimesh::vec3(0, 0, 0));
			line.push_back(trimesh::vec3(1, 1, 1));

			std::vector<std::vector<trimesh::vec3>> routes;
			routes.push_back(line);

			updateRoute(routes);
			hideRoute();
		}
	}
	else
	{
		creative_kernel::useDefaultScene();
		creative_kernel::showPrinter();
		uninstallEventHandlers();
	}
	
	creative_kernel::requestVisUpdate(true);
}

void PaintOperateMode::handleColorsListChanged(const std::vector<trimesh::vec>& newColors)
{
	int oldSize = m_colorsList.size(), newSize = newColors.size();
	if (newSize == 0 || !m_originModel || !m_isWrapperReady)
		return;

	if (newSize < m_colorIndex) //m_colorIndex start from 1
	{
		setColorIndex(0);
	}

	int deleteCount = oldSize - newSize;
	while (deleteCount > 0)
	{	
		std::vector<int> chunks;
		m_meshSpreadWrapper->change_state_all_triangles(oldSize + deleteCount - 1, 1, chunks);
		deleteCount--;
		onGotNewData(chunks);
	}

	m_colorsList = newColors;
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

	qtuser_3d::ScreenCamera* screenCamera = creative_kernel::visCamera();
	Qt3DRender::QCamera* camera = screenCamera->camera();
	bool isOrth = camera->projectionType() == Qt3DRender::QCameraLens::ProjectionType::OrthographicProjection;
	for (auto entity : m_outlineEntitys)
	{
		if (isOrth)
			entity->setOffset(0.0);
		else 
			entity->setOffset(0.1);
	}
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
	routeFlag = true;
	if (routes.size() == 0)
	{
		if (m_meshSpreadWrapper->judge_select_triangles())
			return;
		hideRoute();
		return;
	}

	resetRouteNum(1);
	auto entity = m_outlineEntitys.first();
	entity->setRoute(routes);
	entity->setPose(m_spreadModel->pose());
	creative_kernel::visShowCustom(entity);
}

void PaintOperateMode::hideRoute()
{
	routeFlag = false;
	m_meshSpreadWrapper->seed_fill_unselect_all_triangles();
	for (auto entity : m_outlineEntitys)
		creative_kernel::visHideCustom(entity);

	getKernelUI()->hideToolTip();
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
	trimesh::vec normal(m_sectionNormal.x(), m_sectionNormal.y(), m_sectionNormal.z());
	m_meshSpreadWrapper->bucket_fill_select_triangles_preview(data.center, data.faceId, m_colorIndex, contour, normal, m_offset, 30, false, false);
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
	m_meshSpreadWrapper->bucket_fill_select_triangles_preview(data.center, data.faceId, m_colorIndex, contour, normal, m_offset, m_smartFillEnable ? m_smartFillAngle : 90, true, true, m_overHangsEnable);
	updateRoute(contour);
}

void PaintOperateMode::chooseCircle()
{
	spread::SceneData data;
	if (!getSceneData(pos(), data))
	{
		if (m_screenRadius != 0)
		{
			m_screenRadius = 0;
			emit screenRadiusChanged();
		}
		m_hoverOnModel = false;
		hideRoute();
		return;
	}
	m_hoverOnModel = true;
	radiusMapToScreen(QVector3D(data.center.x, data.center.y, data.center.z));
}

void PaintOperateMode::chooseHeightRange()
{
	spread::SceneData data;
	if (!getSceneData(pos(), data))
	{
		hideRoute();
		return;
	}

	std::vector<std::vector<trimesh::vec3>> contour;
	m_meshSpreadWrapper->get_height_contour(data.center, m_heightRange, contour);
	updateRoute(contour);
}

void PaintOperateMode::chooseGapFillArea()
{
	spread::SceneData data;
	if (!m_hasGap || !getSceneData(pos(), data))
	{
		m_spreadModel->setNeedHighlightEnabled(false);
		hideRoute();
		return;
	}

	m_spreadModel->setNeedHighlightEnabled(true);
	getKernelUI()->showToolTip(pos() + QPoint(15, -9), tr("click to gap fill"));
		
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
	{		
		return false;
	}

	/* 配置SceneData参数 */ 
	data.center = cross;
	QVector3D cameraPos = creative_kernel::cameraController()->getViewPosition();
	data.cameraPos = trimesh::vec(cameraPos.x(), cameraPos.y(), cameraPos.z());
	data.radius = colorRadius();
	data.faceId = faceID;// 获取原始的faceID
	data.planeNormal = trimesh::vec(m_normal.x(), m_normal.y(), m_normal.z());
	data.planeOffset = m_offset;
	data.support_angle_threshold_deg = actualUsedHighlightHangsAngle();

	/* 检验faceID是否有效 */
	TriMeshPtr mesh = m_originModel->globalMesh();
	if (faceID >= mesh->faces.size())
	{
		return false;
	}

	/* 检验是否背面 */
	auto face = mesh->faces[faceID];
	trimesh::vec3 edge1 = mesh->vertices[face[1]] - mesh->vertices[face[0]];
	trimesh::vec3 edge2 = mesh->vertices[face[2]] - mesh->vertices[face[1]];
	trimesh::vec3 normal = edge1.cross(edge2);
	if (normal.dot(rayDirection) > 0)
	{
		return false;
	}
	
	return true;
}

void PaintOperateMode::color(const QPoint &pos, int colorIndex, bool isFirstColor)
{
	m_colorExecutor->setColorIndex(colorIndex);

	if (m_colorMethod == spread::CursorType::TRIANGLE)
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
	else if (m_colorMethod == spread::CursorType::FILL)
	{
		m_colorExecutor->fillColor();
	}
	else if (m_colorMethod == spread::CursorType::HEIGHT_RANGE)
	{
		spread::SceneData data;
		if (!getSceneData(pos, data))
			return;
		m_colorExecutor->heightRangeColor(data, m_heightRange);
	}
	else if (m_colorMethod == spread::CursorType::GAP_FILL)
	{
		m_colorExecutor->fillGapColor();
		onTriggerGapFill();
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
	if (m_colorMethod == spread::CursorType::TRIANGLE)
		chooseTriangle();
	else if(m_colorMethod == spread::CursorType::FILL)
		chooseFillArea();
	else if (m_colorMethod == spread::CursorType::CIRCLE)
		chooseCircle();
	else if (m_colorMethod == spread::CursorType::HEIGHT_RANGE)
		chooseHeightRange();
	else if (m_colorMethod == spread::CursorType::GAP_FILL)
		chooseGapFillArea();
	else 
		hideRoute();

	// if (pressed())
	// 	color(pos, m_erase ? m_spreadModel->defaultFlag() : m_colorIndex, false);
	if (pressed())
		color(pos, m_erase ? 0 : m_colorIndex, false);
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

void PaintOperateMode::onTriggerGapFill()
{
	if (m_colorMethod == spread::CursorType::GAP_FILL)
	{
		std::vector<int> dirtyChunks;
		m_hasGap = m_meshSpreadWrapper->get_triangles_per_patch(m_gapArea, dirtyChunks);
		for (int i = 0; i < dirtyChunks.size(); ++i)
		{
			std::vector<trimesh::vec3> positions;
			std::vector<int> flags;
			std::vector<int> flags_before;
			std::vector<int> splitIndices;

			int id = dirtyChunks[i];
			m_meshSpreadWrapper->chunk_gap_fill(dirtyChunks[i], positions, flags_before, flags, splitIndices);

			m_spreadModel->updateChunkData(id, positions, flags, flags_before);
			m_spreadModel->setChunkIndexMap(id, splitIndices);
		}
	}
}

void PaintOperateMode::refreshRender()
{
	int count = m_meshSpreadWrapper->chunkCount();	
	for (int chunkId = 0; chunkId < count; ++chunkId)
	{
		std::vector<trimesh::vec3> positions; 
		std::vector<int> flags;
		std::vector<int> indexMap;
		m_meshSpreadWrapper->chunk(chunkId, positions, flags, indexMap);
		m_spreadModel->updateChunkData(chunkId, positions, flags);
		m_spreadModel->setChunkIndexMap(chunkId, indexMap);
	}
}

void PaintOperateMode::setDefaultFlag(int flag)
{
	m_spreadModel->setDefaultFlag(flag);
}

bool PaintOperateMode::paintDataEqual(std::vector<std::string> data1, std::vector<std::string> data2)
{
	bool isChanged = false;
	if (data1.size() != data2.size())
	{
		isChanged = true;
	}
	else
	{
		for (int i = 0, count = data1.size(); i < count; ++i)
		{
			if (data1[i] != data2[i])
			{
				isChanged = true;
				break;
			}
		}
	}

	return !isChanged;
}

/**** override ***/
void PaintOperateMode::installEventHandlers()
{
	creative_kernel::addHoverEventHandler(this);
	creative_kernel::prependLeftMouseEventHandler(this);
	creative_kernel::prependKeyEventHandler(this);
	creative_kernel::prependRightMouseEventHandler(this);
	creative_kernel::prependMidMouseEventHandler(this);
	creative_kernel::prependWheelEventHandler(this);
	connect(creative_kernel::cameraController(), &qtuser_3d::CameraController::signalViewChanged, this, &PaintOperateMode::resize);
}

void PaintOperateMode::uninstallEventHandlers()
{
	creative_kernel::removeHoverEventHandler(this);
	creative_kernel::removeLeftMouseEventHandler(this);
	creative_kernel::removeKeyEventHandler(this);
	creative_kernel::removeRightMouseEventHandler(this);
	creative_kernel::removeMidMouseEventHandler(this);
	creative_kernel::removeWheelEventHandler(this);
	disconnect(creative_kernel::cameraController(), &qtuser_3d::CameraController::signalViewChanged, this, &PaintOperateMode::resize);
}

void PaintOperateMode::onAttach() 
{
	creative_kernel::setContinousRender();
	creative_kernel::addSelectTracer(this);
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
		// creative_kernel::requestVisPickUpdate(true);
		creative_kernel::requestVisUpdate(true);
		creative_kernel::setCommandRender();
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
	creative_kernel::showPrinter();
	creative_kernel::updateWipeTowers();
	creative_kernel::resetModelSection();	

	creative_kernel::setCommandRender();
	m_meshSpreadWrapper->clearBuffer();
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
	m_leftMoveStatus = false;
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
	m_leftPressStatus = false;
	m_pressed = true;
	m_erase = event->modifiers() == Qt::ShiftModifier;

	QPoint pos = event->pos();
	// color(pos, m_erase ? m_spreadModel->defaultFlag() : m_colorIndex, true);
	color(pos, m_erase ? 0 : m_colorIndex, true);

}
	
void PaintOperateMode::onLeftMouseButtonRelease(QMouseEvent* event) 
{
	m_leftReleaseStatus = false;
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

	// qDebug() << "******* push *******";
	// for (int i = 0, count = m_lastSpreadData.size(); i < count; ++i) 
	// {
	// 	if (m_lastSpreadData[i].size() > 0) 
	// 	{
	// 		qDebug() << i << m_lastSpreadData[i].data();
	// 	}
	// }

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
	else if (event->modifiers() == Qt::ControlModifier)
	{
		bool plus = event->angleDelta().y() > 0;
		std::function<float(float value, bool plus, float stepSize, float min, float max)> getResult = 
		[] (float value, bool plus, float stepSize, float min, float max) {
			if (plus)
			{
				value += stepSize;
				value = value > max ? max : value;
			}
			else 
			{
				value -= stepSize;
				value = value < min ? min : value;
			}
			return value;
		};

		if (m_colorMethod == spread::CursorType::CIRCLE) 
		{
			float radius = colorRadius();
			radius = getResult(radius, plus, 0.1, 0.4, 8.0);
			setColorRadius(radius);
			m_wheelStatus = false;
		}
		else if (m_colorMethod == spread::CursorType::HEIGHT_RANGE)
		{
			float range = heightRange();
			range = getResult(range, plus, 0.1, 0.1, 8.0);
			setHeightRange(range);
			m_wheelStatus = false;
		}
		else if (m_colorMethod == spread::CursorType::FILL && smartFillEnable())
		{
			float angle = smartFillAngle();
			angle = getResult(angle, plus, 1.0, 0.0, 90.0);
			setSmartFillAngle(angle);
			m_wheelStatus = false;
		}
		else if (m_colorMethod == spread::CursorType::GAP_FILL)
		{
			float area = gapArea();
			area = getResult(area, plus, 0.1, 0.0, 5.0);
			setGapArea(area);
			m_wheelStatus = false;
		}
	}
}

void PaintOperateMode::onSelectionsChanged()
{
	if (m_originModel == NULL)
		return;

	creative_kernel::setVisOperationMode(NULL);
}