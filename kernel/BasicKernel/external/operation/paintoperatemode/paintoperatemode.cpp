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
#include "cxkernel/interface/jobsinterface.h"
#include "interface/visualsceneinterface.h"
#include "interface/printerinterface.h"
#include "interface/machineinterface.h"
#include "interface/reuseableinterface.h"
#include "interface/spaceinterface.h"

#include "qtuser3d/camera/cameracontroller.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/utils/qtuser3denum.h"
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
#include "entity/alonepointentity.h"
#include "entity/pureentity.h"
#include "msbase/primitive/primitive.h"
#include "cxkernel/data/trimeshutils.h"
#include "qtuser3d/geometry/geometrycreatehelper.h"
#include "qtuser3d/geometry/basicshapecreatehelper.h"
#include "qtuser3d/refactor/xeffect.h"
#include <QEntity>

PaintOperateMode::PaintOperateMode(QObject* parent)
    : qtuser_3d::SceneOperateMode(parent)
  {
    qRegisterMetaType<std::vector<int>>("std::vector<int>");

	m_sphereEntity = new qtuser_3d::PureEntity();
	m_sphereEntity->addPassFilter(0, "view");
	Qt3DRender::QGeometry* geometry = qtuser_3d::BasicShapeCreateHelper::createBall(QVector3D(0, 0, 0), 1, 5, m_sphereEntity);
	m_sphereEntity->setGeometry(geometry);

    m_colorExecutor = new ColorExecutor(this);

    connect(m_colorExecutor, &ColorExecutor::sig_needUpdate, this, &PaintOperateMode::onGotNewData);
	connect(creative_kernel::getKernel(), &creative_kernel::Kernel::currentPhaseChanged, this, [=] ()
	{
		if (isRunning())
		{
			creative_kernel::setVisOperationMode(NULL);
		}
	});
	connect(this, &PaintOperateMode::colorMethodChanged, this, [=] ()
	{
		for (auto model : m_spreadModels)
			model->setNeedMixEnabled(m_colorMethod == 3);
	});
	connect(this, &PaintOperateMode::colorMethodChanged, this, &PaintOperateMode::onTriggerGapFill);
	connect(this, &PaintOperateMode::gapAreaChanged, this, &PaintOperateMode::onTriggerGapFill);

	setHighlightOverhangsDeg(m_highlightOverhangsDeg);
  }

PaintOperateMode::~PaintOperateMode()
{
	resetRouteNum(0);
	m_sphereEntity->deleteLater();
	// m_spreadModel->deleteLater();
}

bool PaintOperateMode::isRunning()
{
	return m_isRunning;
}

void PaintOperateMode::restore(int objectID, const QList<std::vector<std::string>>& datas)
{
	m_lastSpreadDatas = datas;
	// qDebug() << "******* restore *******";
	// for (int i = 0, count = m_lastSpreadData.size(); i < count; ++i) 
	// {
	// 	if (m_lastSpreadData[i].size() > 0) 
	// 	{
	// 		qDebug() << i << m_lastSpreadData[i].data();
	// 	}
	// }

	//for (int i = 0, count = m_meshSpreadWrappers.count(); i < count; ++i)
	//{
	//	auto wrapper = m_meshSpreadWrappers[i];
	//	wrapper->set_triangle_from_data(datas[i]);
	//	wrapper->updateTriangle();
	//}

	for (int i = 0, spreadCount = m_spreadModels.count(); i < spreadCount; ++i)
	{
		auto wrapper = m_meshSpreadWrappers[i];
		wrapper->set_triangle_from_data(datas[i]);
		wrapper->updateTriangle();

		auto spreadModel = m_spreadModels[i];
		int count = wrapper->chunkCount();
		for (int i = 0; i < count; ++i)
		{
			std::vector<trimesh::vec3> positions;
			std::vector<int> flags;
			std::vector<int> indexMap;
			wrapper->chunk(i, positions, flags, indexMap);
			spreadModel->updateChunkData(i, positions, flags);
			spreadModel->setChunkIndexMap(i, indexMap);

			//QMatrix4x4 mat;
			//mat.translate(QVector3D(10, 10, 10));
			//spreadModel->setChunkIndexMap(i, indexMap);
			//spreadModel->setPose(mat);
		}
	}
	onTriggerGapFill();
}

std::vector<std::string> PaintOperateMode::getPaintData(int wrapperIndex) const
{
	m_meshSpreadWrappers[wrapperIndex]->updateData();
	return m_meshSpreadWrappers[wrapperIndex]->get_data_as_string();
}

bool PaintOperateMode::isHoverModel(const QPoint& pos) 
{
	spread::SceneData data;
	return getSceneData(pos, data) != -1;
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

		if (isRunning() && m_hoveredIndex != -1) 
		{
			radiusMapToScreen(m_originModels[m_hoveredIndex]->localPosition());
		}
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
		updateSphereColor();
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
	for (auto spreadModel : m_spreadModels)
		spreadModel->setHighlightOverhangsDeg(m_highlightOverhangsDeg);
	for (auto wrapper : m_meshSpreadWrappers)
	{
		wrapper->set_paint_on_overhangs_only(m_overHangsEnable ? m_highlightOverhangsDeg : 0);
	}
	emit highlightOverhangsDegChanged();
}

bool PaintOperateMode::overHangsEnable()
{
	return m_overHangsEnable;
}

void PaintOperateMode::setOverHangsEnable(bool enable)
{
	m_overHangsEnable = enable;
	for (auto wrapper : m_meshSpreadWrappers)
	{
		wrapper->set_paint_on_overhangs_only(m_overHangsEnable ? m_highlightOverhangsDeg : 0);
	}
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

float PaintOperateMode::sphereSize()
{
	return m_sphereSize;
}

void PaintOperateMode::setSphereSize(float size)
{
	m_sphereSize = size;
	
	QMatrix4x4 m = m_sphereEntity->pose();
	for (int i = 0; i < 3; ++i)
		m(i, i) = size;
	m_sphereEntity->setPose(m);

	emit sphereSizeChanged();
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

void PaintOperateMode::updateSphereColor()
{
	//auto colors = creative_kernel::currentColors();
	auto colors = m_colorsList;
	auto color = colors[m_colorIndex - 1];
	m_sphereEntity->setColor(QVector4D(color[0], color[1], color[2], 0.35));
}

void PaintOperateMode::resize()
{
  auto camera = creative_kernel::visCamera()->camera();
  m_scale = camera->fieldOfView();
  emit scaleChanged();

  if (isRunning())
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
	if (!isRunning())
		return;

	float minX, minY, minZ, maxX, maxY, maxZ;
	minX = minY = minZ = 100000;
	maxX = maxY = maxZ = -100000;
	for (auto m : m_originModels)
	{
		// 计算预览区域
		auto box = m->globalSpaceBox();
		minX = box.min.x() < minX ? box.min.x() : minX;
		minY = box.min.y() < minY ? box.min.y() : minY;
		minZ = box.min.z() < minZ ? box.min.z() : minZ;
		maxX = box.max.x() > maxX ? box.max.x() : maxX;
		maxY = box.max.y() > maxY ? box.max.y() : maxY;
		maxZ = box.max.z() > maxZ ? box.max.z() : maxZ;
	}
	qtuser_3d::Box3D box(QVector3D(minX, minY, minZ), QVector3D(maxX, maxY, maxZ));

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

	for (auto spreadModel : m_spreadModels)
		spreadModel->setSection(m_sectionPos + sectionPosOffset * m_sectionNormal, m_backPos + backPosOffset * m_sectionNormal, m_sectionNormal);

	QVector3D _m_sectionNormal = -m_sectionNormal;
	float d1 = QVector3D::dotProduct(_m_sectionNormal, m_frontPos);
	float d2 = QVector3D::dotProduct(_m_sectionNormal, m_backPos);
	m_offset = d2 + (d1 - d2) * m_sectionRate;
	m_normal = _m_sectionNormal;
}

/* 渲染 */
void PaintOperateMode::clearAllColor()
{
	for (auto model : m_originModels)
	{
		if (model->hasColors())
		{
			model->dirty();
		}
	}

	for (auto wrapper : m_meshSpreadWrappers)
		wrapper->reset();
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

		showValidModels();
	}
	else
	{
		creative_kernel::usePrepareScene();
		creative_kernel::showPrinter();
		uninstallEventHandlers();

		resetValidModels();
	}
	
	creative_kernel::requestVisUpdate(true);
}

void PaintOperateMode::handleColorsListChanged(const std::vector<trimesh::vec>& newColors)
{
	int oldSize = m_colorsList.size(), newSize = newColors.size();
	if (newSize == 0 || !isRunning())
		return;

	if (newSize < m_colorIndex) //m_colorIndex start from 1
	{
		setColorIndex(0);
	}

	int deleteCount = oldSize - newSize;
	while (deleteCount > 0)
	{	
		for (auto wrapper : m_meshSpreadWrappers)
		{
			std::vector<int> chunks;
			wrapper->change_state_all_triangles(oldSize + deleteCount - 1, 1, chunks);
			deleteCount--;
			onGotNewData(wrapper, chunks);
		}
	}

	m_colorsList = newColors;
	updateSphereColor();
}

void PaintOperateMode::wrappersUpdateData()
{
	for (auto wrapper : m_meshSpreadWrappers)
	{
		wrapper->updateData();
	}
}

void PaintOperateMode::wrappersUnselect()
{
	for (auto wrapper : m_meshSpreadWrappers)
	{
		wrapper->seed_fill_unselect_all_triangles();
	}
}

int PaintOperateMode::groupSize()
{
	return m_originModels.size();
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
	if (m_hoveredIndex == -1)
	{
		hideRoute();
		return;
	}

	resetRouteNum(1);
	qtuser_3d::OutlineEntity* entity = m_outlineEntitys.first();
	entity->setRoute(route);
	entity->setPose(m_spreadModels[m_hoveredIndex]->pose());
	// entity->setPose(m_colorModel->pose());
	creative_kernel::visShowCustom(entity);
}

void PaintOperateMode::updateRoute(const std::vector<std::vector<trimesh::vec3>>& routes)
{
	routeFlag = true;
	if (routes.size() == 0 || m_hoveredIndex == -1)
	{
		hideRoute();
		return;
	}

	resetRouteNum(1);
	auto entity = m_outlineEntitys.first();
	entity->setRoute(routes);
	entity->setPose(m_spreadModels[m_hoveredIndex]->pose());
	creative_kernel::visShowCustom(entity);
}

void PaintOperateMode::hideRoute()
{
	routeFlag = false;
	wrappersUnselect();
	for (auto entity : m_outlineEntitys)
		creative_kernel::visHideCustom(entity);

	getKernelUI()->hideToolTip();

	creative_kernel::visHideCustom(m_sphereEntity);
}

void PaintOperateMode::chooseTriangle()
{
	spread::SceneData data;
	int wrapperIndex = getSceneData(pos(), data);
	if (wrapperIndex == -1)
	{
		hideRoute();
		return;
	}

	std::vector<std::vector<trimesh::vec3>> contour;
	trimesh::vec normal(m_sectionNormal.x(), m_sectionNormal.y(), m_sectionNormal.z());
	m_meshSpreadWrappers[wrapperIndex]->bucket_fill_select_triangles_preview(data.center, data.faceId, m_colorIndex, contour, normal, m_offset, 30, false, false);
	updateRoute(contour);

	//static QList<qtuser_3d::AlonePointEntity*> points;
	//qDeleteAll(points);
	//points.clear();
	//for (std::vector<trimesh::vec3>& line : contour)
	//{
	//	for (trimesh::vec3 p : line)
	//	{
	//		qtuser_3d::AlonePointEntity* pentity = new qtuser_3d::AlonePointEntity();
	//		pentity->setPointSize(10);
	//		pentity->setFilterType("overlay");
	//		pentity->updateGlobal(QVector3D(p[0], p[1], p[2]));
	//		pentity->setColor(QVector4D(1, 0, 0, 1));
	//		creative_kernel::visShowCustom(pentity);
	//		points << pentity;
	//	}
	//}

}

void PaintOperateMode::chooseFillArea()
{
	spread::SceneData data;
	int wrapperIndex = getSceneData(pos(), data);
	if (wrapperIndex == -1)
	{
		hideRoute();
		return;
	}
	std::vector<std::vector<trimesh::vec3>> contour;
	trimesh::vec normal(-m_sectionNormal.x(), -m_sectionNormal.y(), -m_sectionNormal.z());
	m_meshSpreadWrappers[wrapperIndex]->bucket_fill_select_triangles_preview(data.center, data.faceId, m_colorIndex, contour, normal, m_offset, m_smartFillEnable ? m_smartFillAngle : 90, true, true, m_overHangsEnable);
	if (contour.size() == 0)
	{
		if (!m_meshSpreadWrappers[wrapperIndex]->judge_select_triangles())
			updateRoute(contour);
	}
	else 
	{
		updateRoute(contour);
	}
}

void PaintOperateMode::chooseCircle()
{
	spread::SceneData data;
	int wrapperIndex = getSceneData(pos(), data);
	if (wrapperIndex == -1)
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

void PaintOperateMode::chooseSphere()
{
	spread::SceneData data;
	int wrapperIndex = getSceneData(pos(), data);
	if (wrapperIndex == -1)
	{
		m_hoverOnModel = false;
		hideRoute();
		return;
	}
	m_hoverOnModel = true;

	QMatrix4x4 m = m_sphereEntity->pose();

	trimesh::vec worldSpaceCenter = m_meshSpreadWrappers[wrapperIndex]->getMeshGlobalMatrix() * data.center;

	m(0, 3) = worldSpaceCenter.x;
	m(1, 3) = worldSpaceCenter.y;
	m(2, 3) = worldSpaceCenter.z;
	m_sphereEntity->setPose(m);
	creative_kernel::visShowCustom(m_sphereEntity);
}

void PaintOperateMode::chooseHeightRange()
{
	spread::SceneData data;
	int wrapperIndex = getSceneData(pos(), data);
	if (wrapperIndex == -1)
	{
		hideRoute();
		return;
	}

	if (wrapperIndex >= m_meshSpreadWrappers.size())
	{
		return;
	}

	trimesh::vec world_space_center = m_meshSpreadWrappers[wrapperIndex]->getMeshGlobalMatrix() * data.center;

	std::vector<std::vector<trimesh::vec3>> contour;
	// 多个模型轮廓
	for (int i = 0, count = groupSize(); i < count; ++i)
	{
		auto wrapper = m_meshSpreadWrappers[i];
		std::vector<std::vector<trimesh::vec3>> singleContour;
		wrapper->get_height_contour(world_space_center, m_heightRange, singleContour);
		for (auto line : singleContour)
			contour.push_back(line);
	}
	//m_meshSpreadWrappers[wrapperIndex]->get_height_contour(data.center, m_heightRange, contour);
	updateRoute(contour);
}

void PaintOperateMode::chooseGapFillArea()
{
	spread::SceneData data;
	int wrapperIndex = getSceneData(pos(), data);
	if (!m_hasGap || wrapperIndex == -1)
	{
		for (auto spreadModel : m_spreadModels)
			spreadModel->setNeedHighlightEnabled(false);
		hideRoute();
		return;
	}

	for (auto spreadModel : m_spreadModels)
		spreadModel->setNeedHighlightEnabled(true);
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
	float D = -QVector3D::dotProduct(planeNormal, planePoint);
	float t = -(QVector3D::dotProduct(planeNormal, rayStart) + D) / QVector3D::dotProduct(planeNormal, rayDirection);
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

int PaintOperateMode::getSceneData(const QPoint& pos, spread::SceneData& data)
{
	qtuser_3d::ScreenCamera* screenCamera = creative_kernel::visCamera();
	qtuser_3d::Ray ray = screenCamera->screenRay(pos);
	QVector3D from = mapToSectionArea(ray.start, ray.dir);
	trimesh::vec rayOrigin(from.x(), from.y(), from.z());
	trimesh::vec rayDirection(ray.dir.x(), ray.dir.y(), ray.dir.z());
	trimesh::vec cross;

	trimesh::vec closest_hit = trimesh::vec(0.0f);
	double closest_hit_squared_distance = std::numeric_limits<double>::max();
	int closest_facet = -1;
	int closest_hit_mesh_id = -1;

	int wrapperIndex = -1, faceID;
	for (int i = 0, count = m_meshSpreadWrappers.count(); i < count; ++i)
	{
		auto wrapper = m_meshSpreadWrappers[i];
		faceID = wrapper->getFacet(rayOrigin, rayDirection, cross);
		if (faceID == -1)
			continue;

		trimesh::vec cameraPos;
		cameraPos.set(ray.start.x(), ray.start.y(), ray.start.z());
		double hit_squared_distance = (cameraPos - wrapper->getMeshGlobalMatrix() * cross).sumsqr();
		if (hit_squared_distance < closest_hit_squared_distance) {
			closest_hit_squared_distance = hit_squared_distance;
			closest_facet = faceID;
			closest_hit_mesh_id = i;
			closest_hit = cross;
			wrapperIndex = i;
		}

	}

	if (closest_facet == -1)
	{
		m_hoveredIndex = -1;
		return -1;
	}

	faceID = closest_facet;
	m_hoveredIndex = wrapperIndex;

	/* 配置SceneData参数 */ 
	data.center = closest_hit;
	//std::cout << "------cross------------ : " << cross << std::endl;
	QMatrix4x4 modelGlobalMat = m_originModels[m_hoveredIndex]->globalMatrix();
	QVector3D cameraPos = creative_kernel::cameraController()->getViewPosition();
	cameraPos = modelGlobalMat.inverted() * cameraPos;
	data.cameraPos = trimesh::vec(cameraPos.x(), cameraPos.y(), cameraPos.z());
	data.radius = colorRadius();
	data.faceId = faceID;// 获取原始的faceID
	data.planeNormal = trimesh::vec(m_normal.x(), m_normal.y(), m_normal.z());
	data.planeOffset = m_offset;
	data.support_angle_threshold_deg = m_overHangsEnable ? m_highlightOverhangsDeg : 0;

	/* 检验faceID是否有效 */
	//TriMeshPtr mesh = m_originModels[m_hoveredIndex]->globalMesh();
	TriMeshPtr mesh = m_originModels[m_hoveredIndex]->meshptr();
	if (faceID >= mesh->faces.size())
	{
		m_hoveredIndex = -1;
		return -1;
	}

	/* 检验是否背面 */
	auto face = mesh->faces[faceID];
	trimesh::vec3 edge1 = mesh->vertices[face[1]] - mesh->vertices[face[0]];
	trimesh::vec3 edge2 = mesh->vertices[face[2]] - mesh->vertices[face[1]];

	QMatrix4x4 normalMat = m_originModels[m_hoveredIndex]->modelNormalMatrix();
	trimesh::fxform normalXf = qtuser_3d::qMatrix2Xform(normalMat);
	trimesh::vec3 normal = normalXf * (edge1.cross(edge2));
	if (normal.dot(rayDirection) > 0)
	{
		m_hoveredIndex = -1;
		return -1;
	}

	return wrapperIndex;
}

void PaintOperateMode::color(const QPoint &pos, int colorIndex, bool isFirstColor)
{
	if (m_hoveredIndex == -1)
		return;
	
	auto wrapper = m_meshSpreadWrappers[m_hoveredIndex];
	m_colorExecutor->setColorIndex(colorIndex);

	if (m_colorMethod == spread::CursorType::TRIANGLE)
	{	
		m_colorExecutor->triangleColor(wrapper);
	}
	else if (m_colorMethod == spread::CursorType::CIRCLE)
	{
		spread::SceneData data;
		int wrapperIndex = getSceneData(pos, data);
		if (wrapperIndex == -1)
			return;
		wrapper = m_meshSpreadWrappers[wrapperIndex];
		m_colorExecutor->circleColor(wrapper, data, isFirstColor);
	}
	else if (m_colorMethod == spread::CursorType::SPHERE)
	{
		spread::SceneData data;
		int wrapperIndex = getSceneData(pos, data);
		if (wrapperIndex == -1)
			return;
		wrapper = m_meshSpreadWrappers[wrapperIndex];
		m_colorExecutor->sphereColor(wrapper, data, m_sphereSize, isFirstColor);
	}
	else if (m_colorMethod == spread::CursorType::FILL)
	{
		m_colorExecutor->fillColor(wrapper);
	}
	else if (m_colorMethod == spread::CursorType::HEIGHT_RANGE)
	{
		spread::SceneData data;
		int wrapperIndex = getSceneData(pos, data);
		if (wrapperIndex == -1)
			return;

		if (wrapperIndex >= m_meshSpreadWrappers.size())
		{
			return;
		}

		trimesh::vec world_space_center = m_meshSpreadWrappers[wrapperIndex]->getMeshGlobalMatrix() * data.center;
		data.center = world_space_center;

		//wrapper = m_meshSpreadWrappers[wrapperIndex];
		for (auto wrapper : m_meshSpreadWrappers)
			m_colorExecutor->heightRangeColor(wrapper, data, m_heightRange);

		//m_colorExecutor->heightRangeColor(wrapper, data, m_heightRange);
	}
	else if (m_colorMethod == spread::CursorType::GAP_FILL)
	{
		for (auto wrapper : m_meshSpreadWrappers)
			m_colorExecutor->fillGapColor(wrapper);
		// m_colorExecutor->fillGapColor(wrapper);
		onTriggerGapFill();
	}
}

void PaintOperateMode::updateSpreadModel()
{
	for (int i = 0, spreadCount = m_spreadModels.size(); i < spreadCount; ++i)
	{
		auto wrapper = m_meshSpreadWrappers[i];
		auto spreadModel = m_spreadModels[i];
		int count = wrapper->chunkCount();
		for (int i = 0; i < count; ++i)
		{
			std::vector<trimesh::vec3> positions;
			std::vector<int> flags;
			std::vector<int> indexMap;
			wrapper->chunk(i, positions, flags, indexMap);
			spreadModel->updateChunkData(i, positions, flags);
			spreadModel->setChunkIndexMap(i, indexMap);
		}
	}

	creative_kernel::updateFaceBases();
	creative_kernel::requestVisPickUpdate(false);
}

void PaintOperateMode::choose(const QPoint& pos)
{
	int hoveredIndexCache = m_hoveredIndex;
	if (m_colorMethod == spread::CursorType::TRIANGLE)
		chooseTriangle();
	else if(m_colorMethod == spread::CursorType::FILL)
		chooseFillArea();
	else if (m_colorMethod == spread::CursorType::CIRCLE)
		chooseCircle();
	else if (m_colorMethod == spread::CursorType::SPHERE)
		chooseSphere();
	else if (m_colorMethod == spread::CursorType::HEIGHT_RANGE)
		chooseHeightRange();
	else if (m_colorMethod == spread::CursorType::GAP_FILL)
		chooseGapFillArea();
	else 
		hideRoute();
		
	if (pressed() && m_hoveredIndex != -1)
	{
		if (hoveredIndexCache != m_hoveredIndex)
		{
			color(pos, m_erase ? 0 : m_colorIndex, true);
		}
		else 
		{
			color(pos, m_erase ? 0 : m_colorIndex, false);
		}
	}
		// color(pos, m_erase ? 0 : m_colorIndex, hoveredIndexCache != m_hoveredIndex);
}

void PaintOperateMode::onColorsListChanged()
{
	for (auto spreadModel : m_spreadModels)
		spreadModel->setPalette(m_colorsList);
}

void PaintOperateMode::onGotNewData(void* wrapperObj, std::vector<int> dirtyChunks)
{
	spread::MeshSpreadWrapper* wrapper = (spread::MeshSpreadWrapper*)(wrapperObj);
	if (!wrapper)
		return;

	int index = m_meshSpreadWrappers.indexOf(wrapper);
	auto spreadModel = m_spreadModels[index];
	for (auto chunkId : dirtyChunks)
	{
		std::vector<trimesh::vec3> positions; 
		std::vector<int> flags;
		std::vector<int> indexMap;
		wrapper->chunk(chunkId, positions, flags, indexMap);
		spreadModel->updateChunkData(chunkId, positions, flags);
		spreadModel->setChunkIndexMap(chunkId, indexMap);
	}
}

void PaintOperateMode::onTriggerGapFill()
{
	if (m_colorMethod == spread::CursorType::GAP_FILL)
	{
		for (int i = 0, count = groupSize(); i < count; ++i)
		{
			auto wrapper = m_meshSpreadWrappers[i];
			auto spreadModel = m_spreadModels[i];
			std::vector<int> dirtyChunks;
			m_hasGap = wrapper->get_triangles_per_patch(m_gapArea, dirtyChunks);
			for (int i = 0; i < dirtyChunks.size(); ++i)
			{
				std::vector<trimesh::vec3> positions;
				std::vector<int> flags;
				std::vector<int> flags_before;
				std::vector<int> splitIndices;

				int id = dirtyChunks[i];
				wrapper->chunk_gap_fill(dirtyChunks[i], positions, flags_before, flags, splitIndices);

				spreadModel->updateChunkData(id, positions, flags, flags_before);
				spreadModel->setChunkIndexMap(id, splitIndices);
			}
		}
	}
}

void PaintOperateMode::refreshRender()
{
	for (int i = 0, spreadCount = m_spreadModels.count(); i < spreadCount; ++i)
	{
		auto wrapper = m_meshSpreadWrappers[i];
		auto spreadModel = m_spreadModels[i];
		int count = wrapper->chunkCount();	
		for (int chunkId = 0; chunkId < count; ++chunkId)
		{
			std::vector<trimesh::vec3> positions; 
			std::vector<int> flags;
			std::vector<int> indexMap;
			wrapper->chunk(chunkId, positions, flags, indexMap);
			spreadModel->updateChunkData(chunkId, positions, flags);
			spreadModel->setChunkIndexMap(chunkId, indexMap);
		}
	}
}

void PaintOperateMode::setDefaultFlag(const QList<int>& flags)
{
	for (int i = 0, count = m_spreadModels.size(); i < count; ++i)
	{
		auto spreadModel = m_spreadModels[i];
		spreadModel->setDefaultFlag(flags[i]);
	}
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

void PaintOperateMode::initValidModels()
{
	if (m_originModels.size() > 0)
		return;

	QList<creative_kernel::ModelN*> parts = creative_kernel::selectedParts();
	QList<creative_kernel::ModelGroup*> groups = creative_kernel::selectedGroups();
	auto selectionsList = creative_kernel::selectedModelnsList();
	if (parts.size() > 0)
	{
		m_modelGroup = NULL;
		m_originModels.clear();
		for (creative_kernel::ModelN* model : parts)
		{
			if (model->modelNType() == creative_kernel::ModelNType::normal_part)
				m_originModels << model;
		}
	}
	else if (groups.size() > 0)
	{
		 creative_kernel::ModelGroup* group = groups.first();
		 m_modelGroup = group;

		 QList<creative_kernel::ModelN*> ms = group->models((int)creative_kernel::ModelNType::normal_part);
		 m_originModels = ms;
	} 
	else if (!selectionsList.isEmpty())
	{
		m_modelGroup = NULL;
		QList<creative_kernel::ModelN*> models = selectionsList[0];
		m_originModels.clear();
		for (creative_kernel::ModelN* model : models)
		{
			if (model->modelNType() == creative_kernel::ModelNType::normal_part)
				m_originModels << model;
		}
	}
	else 
	{
		 m_modelGroup = NULL;
		 m_originModels.clear();
	}

}

void PaintOperateMode::recordModelsState()
{
	if (m_modelGroup)
	{
		m_modelsVisibleStateMap.clear();

		QList<creative_kernel::ModelN*> models = m_modelGroup->models();
		for (creative_kernel::ModelN* model : models)
		{
			m_modelsVisibleStateMap[model] = model->isVisible();
		}
	}
}

void PaintOperateMode::showValidModels()
{
	//if (m_modelGroup)
	//{
	//	creative_kernel::visShowCustom(m_modelGroup->qEntity());

	//	QList<creative_kernel::ModelN*> models = m_modelGroup->models();
	//	for (creative_kernel::ModelN* model : models)
	//	{
	//		int type = (int)model->modelNType();
	//		if (m_visibleTypes.contains(type))
	//			model->qEntity()->setEnabled(true);
	//		else 
	//			model->qEntity()->setEnabled(false);
	//	}
	//	creative_kernel::setZProjectEnabled(false);
	//}
}

void PaintOperateMode::resetValidModels()
{
	//if (m_modelGroup)
	//{
	//	if (!m_modelGroup->qEntity())
	//		return;

 //   	creative_kernel::spaceShow(m_modelGroup->qEntity());

	//	auto it = m_modelsVisibleStateMap.begin(), end = m_modelsVisibleStateMap.end();
	//	for (; it != end; ++it)
	//	{
	//		creative_kernel::ModelN* model = it.key();
	//		bool visible = it.value();
	//		model->qEntity()->setEnabled(visible);
	//	}
	//	creative_kernel::setZProjectEnabled(true);
	//}
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
	creative_kernel::addModelNSelectorTracer(this);
	creative_kernel::traceSpace(this);

	resize();
	m_isWrapperReady = false;

	/* 初始化m_meshSpreadWrapper */
	QList<TriMeshPtr> meshs;
	for (auto model : m_originModels)
	{
		if (!model->meshptr().get())
			continue;

		spread::MeshSpreadWrapper* aSpreadWrapper = new spread::MeshSpreadWrapper();
		if (!aSpreadWrapper)
		{
			qWarning() << Q_FUNC_INFO << ", failed to create spread::MeshSpreadWrapper.";
			return;
		}

		aSpreadWrapper->setMeshGlobalMatrix(model->globalDMatrix());
		m_meshSpreadWrappers << aSpreadWrapper;
		//meshs << model->globalMesh();

		meshs << model->meshptr();

		SpreadModel* spreadModel = new SpreadModel(this);
		spreadModel->init();
		spreadModel->setPalette(m_colorsList);
		spreadModel->setMatrixInfo(model->globalMatrix());
		m_spreadModels << spreadModel;
	}

	recordModelsState();
	m_colorLoadJob = new ColorLoadJob(this);
	m_colorLoadJob->setMeshSpreadWrapper(m_meshSpreadWrappers);
	m_colorLoadJob->setTriMesh(meshs);
	m_colorLoadJob->setColorsList(getColorsList());
	m_colorLoadJob->setData(m_lastSpreadDatas);
	cxkernel::executeJob(qtuser_core::JobPtr(m_colorLoadJob));
	connect(m_colorLoadJob, &ColorLoadJob::sig_finished, this, [=] ()
	{
		creative_kernel::requestRender(this);
		m_isRunning = true;
		emit enter();

		for (int i = 0, spreadCount = m_spreadModels.count(); i < spreadCount; ++i)
		{
			auto wrapper = m_meshSpreadWrappers[i];
			auto spreadModel = m_spreadModels[i];
			int count = wrapper->chunkCount();		
			for (int i = 0; i < count; ++i)
			{
				std::vector<trimesh::vec3> positions;
				std::vector<int> flags;
				std::vector<int> indexMap;
				wrapper->chunk(i, positions, flags, indexMap);
				spreadModel->addChunk(positions, flags);
				spreadModel->setChunkIndexMap(i, indexMap);
			}
		}
		
		m_isWrapperReady = true;

		updateScene();
		updateSectionBoundary();
		updateSection();
		creative_kernel::updateFaceBases();
		creative_kernel::requestVisUpdate(true);
		creative_kernel::endRequestRender(this);

		setHighlightOverhangsDeg(m_highlightOverhangsDeg);
	});

	if (m_waitForLoadModel)
		m_colorLoadJob->wait();

}

void PaintOperateMode::onDettach() 
{
	uninstallEventHandlers();

	creative_kernel::removeModelNSelectorTracer(this);
	creative_kernel::unTraceSpace(this);
	creative_kernel::usePrepareScene();
	creative_kernel::showPrinter();
	creative_kernel::resetModelSection();	

	for (auto spreadModel : m_spreadModels)
	{
		spreadModel->clear();
		spreadModel->deleteLater();
	}
	m_spreadModels.clear();

	for (auto wrapper : m_meshSpreadWrappers)
	{
		delete wrapper;
	}
	m_meshSpreadWrappers.clear();

	hideRoute();
	m_originModels.clear();

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

void PaintOperateMode::onLeftMouseButtonClick(QMouseEvent* event) 
{
	if (m_hoveredIndex == -1)
		return;

	m_leftClickStatus = false;
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
	//m_leftPressStatus = false;
	m_leftPressStatus = true;
	m_pressed = true;
	m_erase = event->modifiers() == Qt::ShiftModifier;

	QPoint pos = event->pos();
	color(pos, m_erase ? 0 : m_colorIndex, true);

}
	
void PaintOperateMode::onLeftMouseButtonRelease(QMouseEvent* event) 
{
	m_leftReleaseStatus = false;
	creative_kernel::requestVisPickUpdate(true);
	m_pressed = false;
	m_erase = event->modifiers() == Qt::ShiftModifier;

	if (m_hoveredIndex == -1)
		return;

	m_leftReleaseStatus = false;
	wrappersUpdateData();

	QList<std::vector<std::string>> currentSpreadDatas;
	for (auto wrapper : m_meshSpreadWrappers)
	{
		currentSpreadDatas << wrapper->get_data_as_string();
	}

	QList<creative_kernel::ModelN*> undoModels;
	QList<std::vector<std::string>> undoDatas;
	QList<std::vector<std::string>> redoDatas;
	for (int i = 0, count = groupSize(); i < count; ++i)
	{
		//if (paintDataEqual(currentSpreadDatas[i], m_lastSpreadDatas[i]))
		//	continue;
		undoModels << m_originModels[i];
		undoDatas.push_back(m_lastSpreadDatas[i]);
		redoDatas.push_back(currentSpreadDatas[i]);
		m_originModels[i]->dirty();
	}

	creative_kernel::ModelSpaceUndo* stack = creative_kernel::getKernel()->modelSpaceUndo();
	ColorUndoCommand* command = new ColorUndoCommand();
	if (m_modelGroup)
		command->setObjects(this, m_modelGroup);
	else
		command->setObjects(this, undoModels.first());
	command->setData(undoDatas, redoDatas);

	// qDebug() << "******* push *******";
	// for (int i = 0, count = m_lastSpreadData.size(); i < count; ++i) 
	// {
	// 	if (m_lastSpreadData[i].size() > 0) 
	// 	{
	// 		qDebug() << i << m_lastSpreadData[i].data();
	// 	}
	// }

	m_lastSpreadDatas = currentSpreadDatas;

	stack->executeCommand(command);

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
		else if (m_colorMethod == spread::CursorType::SPHERE)
		{
			float size = sphereSize();
			size = getResult(size, plus, 0.1, 0.4, 8.0);
			setSphereSize(size);
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

void PaintOperateMode::onSelectionsBoxChanged()
{
	if (!isRunning())
		return;

    getKernelUI()->setCommandIndex(-1);
	// creative_kernel::setVisOperationMode(NULL);
}

void PaintOperateMode::onSelectionsChanged()
{
	if (!isRunning())
		return;

    getKernelUI()->setCommandIndex(-1);
	creative_kernel::setVisOperationMode(NULL);
}
