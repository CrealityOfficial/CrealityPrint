#include "fontmeshwrapper.h"

#include "internal/alg/new_letter.h"

#include "data/modelgroup.h"
#include "data/modeln.h"

#include "interface/spaceinterface.h"
#include "interface/printerinterface.h"
#include "interface/shareddatainterface.h"
#include "interface/selectorinterface.h"
#include "interface/visualsceneinterface.h"

#include "qtuser3d/trimesh2/conv.h"
#include "entity/alonepointentity.h"
#include "entity/lineentity.h"

#include <sstream>
#define CHECK_OUTLINE 0
#define SHOW_RAY 0

using namespace creative_kernel;

FontConfig::FontConfig()
{

}

FontConfig::FontConfig(const FontConfig& other)
{
	reliefTargetID = other.reliefTargetID;
	text = other.text;
	font = other.font;
	fontSize = other.fontSize;
	wordSpace = other.wordSpace;
	lineSpace = other.lineSpace;
	height = other.height;
	distance = other.distance;
	embossType = other.embossType;
	bold = other.bold;
	italic = other.italic;
	angle = other.angle;
	fontMeshData = other.fontMeshData;
}

FontConfig& FontConfig::operator=(const FontConfig& other)
{
	reliefTargetID = other.reliefTargetID;
	text = other.text;
	font = other.font;
	fontSize = other.fontSize;
	wordSpace = other.wordSpace;
	lineSpace = other.lineSpace;
	height = other.height;
	distance = other.distance;
	embossType = other.embossType;
	bold = other.bold;
	italic = other.italic;
	angle = other.angle;
	fontMeshData = other.fontMeshData;

	return *this;
}

FontMeshWrapper::FontMeshWrapper(QObject* parent)
	:QObject(parent)
{
	m_outlineGenerator = new OutlineGenerator(this);
	m_fontMesh = new topomesh::FontMesh(m_fc.height, m_fc.distance, m_fc.angle);
	updateOutline();
}

FontMeshWrapper::FontMeshWrapper(const FontMeshWrapper &other)
{
	m_fontMesh = new topomesh::FontMesh(*other.m_fontMesh);

	m_outlineGenerator = new OutlineGenerator(other.m_outlineGenerator);
	m_outlineGenerator->setParent(this);

	m_fc = other.m_fc;
	m_lastFaceId = other.m_lastFaceId;
	m_lastCross = other.m_lastCross;
	m_targetMatrix = other.m_targetMatrix;
	m_firstEmbossMatrix = other.m_firstEmbossMatrix;
	m_useFirst = other.m_useFirst;
	m_hasEmbossed = other.m_hasEmbossed;
	m_initMatrix = other.m_initMatrix;
}

FontMeshWrapper::FontMeshWrapper(const std::string& reliefDescription)
{
	parseDescription(reliefDescription);
}

FontMeshWrapper::~FontMeshWrapper()
{
	delete m_fontMesh;
}

bool FontMeshWrapper::isClonedFontMesh()
{
	return m_isCloned;
}

void FontMeshWrapper::setRelief(creative_kernel::ModelN* relief)
{
	m_relief = relief;
	m_reliefId = relief->getFixedId();
}

void FontMeshWrapper::regenerateMesh()
{
	if (!m_relief)
		return;

	cxkernel::MeshDataPtr meshDataPtr = m_relief->data();

	trimesh::TriMesh* newMesh = new trimesh::TriMesh();
	*newMesh = *meshDataPtr->mesh.get();
	cxkernel::MeshDataPtr newMeshDataPtr(new cxkernel::MeshData(TriMeshPtr(newMesh), true));

	int meshId = registerMeshData(newMeshDataPtr, false);
	m_relief->setMeshID(meshId);
}

bool FontMeshWrapper::valid()
{
	return m_relief != NULL;
}

bool FontMeshWrapper::hasEmbossed()
{
	return m_hasEmbossed;
}

int FontMeshWrapper::lastTargetId()
{
	return m_fc.reliefTargetID;
}

int FontMeshWrapper::lastFaceId()
{
	return m_lastFaceId;
}

QVector3D FontMeshWrapper::lastCross()
{
	return m_lastCross;
}

int FontMeshWrapper::lastEmbossType()
{
	return m_fc.embossType;
}

void FontMeshWrapper::syncFontMesh()
{
	ModelN* relief = getModelNByFixedId(m_reliefId);
	if (!relief)
		return;
	
	m_relief = relief;

	QMatrix4x4 matrix = m_relief->globalMatrix();
	trimesh::xform xf = trimesh::xform(qtuser_3d::qMatrix2Xform(matrix));
	 m_fontMesh->updateModelXform(xf);
}

void FontMeshWrapper::updateOutline()
{
	m_outlineGenerator->applyFontConfig(m_fc);
	if (m_outlineGenerator->isDirty())
	{
		m_outlineGenerator->generate();
		std::vector<OutLine> outline = m_outlineGenerator->result();

#if CHECK_OUTLINE
		qDebug() << "generate ********";
		static QList<qtuser_3d::LineEntity*> lines;
		static QList<qtuser_3d::AlonePointEntity*> points;
		qDeleteAll(lines);
		lines.clear();
		qDeleteAll(points);
		points.clear();

		QList<QVector4D> palette;
		palette << QVector4D(1, 0, 0, 1);
		palette << QVector4D(0, 1, 0, 1);
		palette << QVector4D(0, 0, 1, 1);
		palette << QVector4D(1, 1, 0, 1);
		palette << QVector4D(0, 0, 0, 1);
		palette << QVector4D(1, 1, 1, 1);

		for (OutLine ol : outline)
		{
			int a = 0;
			int b = 0;
			for (std::vector<trimesh::vec2> pl : ol)
			{
				for (int i = 0, count = pl.size() - 1; i < count; ++i)
				{
					trimesh::vec2 p1 = pl[i];
					trimesh::vec2 p2 = pl[i + 1];

					qtuser_3d::LineEntity* line = new qtuser_3d::LineEntity();
					line->setLineWidth(2);

					QVector<QVector3D> positions;
					QVector<QVector4D> colors;
					positions << QVector3D(p1[0], p1[1] - 50, 5);
					positions << QVector3D(p2[0], p2[1] - 50, 5);
					colors << palette[a % 6];
					colors << palette[a % 6];

					line->updateGeometry(positions, colors, false);
					visShow(line);

					lines << line;
				}

				for (trimesh::vec2 p : pl)
				{
					qtuser_3d::AlonePointEntity* pentity = new qtuser_3d::AlonePointEntity();
					pentity->setPointSize(6);
					pentity->setFilterType("overlay");
					pentity->updateGlobal(QVector3D(p[0], p[1] -50, 5));
					if (b == 0)
						pentity->setColor(palette[5]);
					else 
						pentity->setColor(palette[b % 3]);
					visShow(pentity);
					points << pentity;
					b++;
				}
				a++;
			}
		}
		qDebug() << "generate end ********";
#endif // 

#if !CHECK_OUTLINE
		qDebug() << "generate font mesh ********";
		if (m_relief)
		{
			ModelN* target = getModelNByFixedId(m_fc.reliefTargetID);
			if (target)
			{
				TriMeshPtr meshPtr = target->globalMesh();
				m_fontMesh->updateFontPoly(meshPtr.get(), outline);
			}
			else
			{
				m_fontMesh->updateFontPoly(NULL, outline);
			}
		}
		else
		{
			m_fontMesh->CreateFontMesh(outline);
		}
		qDebug() << "generate font mesh end********";
#endif // 
		m_outlineGenerator->resetDirty();
	}
}

void FontMeshWrapper::setHeight(float height)
{
	if (height != m_fc.height)
	{
		m_fc.height = height;
		m_fontMesh->updateFontHeight(height);
	}
}

void FontMeshWrapper::setDistance(float distance)
{
	if (distance != m_fc.distance)
	{
		m_fc.distance = distance;
		m_fontMesh->updateFontDepth(distance);
	}
}

void FontMeshWrapper::setEmbossType(bool isSurround)
{
	int type = isSurround * 2;
	if (m_fc.embossType != type)
	{	
		m_fc.embossType = type;
		autoEmboss();
		updateModel();
	}	
}

void FontMeshWrapper::rotate(float angle)
{
	ModelN* target = getModelNByFixedId(m_fc.reliefTargetID);
	if (target)
	{
		TriMeshPtr meshPtr = target->globalMesh();
		m_fontMesh->rotateFontMesh(meshPtr.get(), angle);
	}
	else 
	{
		m_fontMesh->rotateFontMesh(NULL, angle);
	}
	m_fc.angle = m_fontMesh->angle();
}

void FontMeshWrapper::initEmboss()
{
	if (!m_hasEmbossed)	// only can init from emboss
		return;

	m_fc.reliefTargetID = -1;
	m_hasEmbossed = false;
	m_fontMesh->setState(0);
	m_fontMesh->InitFontMesh();
	updateMeshData();
	m_relief->setMatrix(trimesh::xform(qtuser_3d::qMatrix2Xform(m_initMatrix)));
}

void FontMeshWrapper::FontMeshWrapper::setEmbossTarget(creative_kernel::ModelN* target)
{
	if (target == NULL)
	{
		m_fc.reliefTargetID = -1;
	}
	else 
	{
		m_fc.reliefTargetID = target->getFixedId();
		syncTarget();
	}
}

void FontMeshWrapper::syncTarget()
{
	ModelN* target = getModelNByFixedId(m_fc.reliefTargetID);
	if (target)
		m_targetMatrix = target->globalMatrix();
}

void FontMeshWrapper::emboss(int faceId, trimesh::vec3 cross)
{
	ModelN* target = getModelNByFixedId(m_fc.reliefTargetID);
	if (!target)
		return;

#if SHOW_RAY
	static qtuser_3d::AlonePointEntity* pentity = new qtuser_3d::AlonePointEntity();
	pentity->setPointSize(4);
	pentity->setFilterType("overlay");
	pentity->updateGlobal(QVector3D(cross[0], cross[1], cross[2]));
	pentity->setColor(QVector4D(1.0, 0.0, 0.0, 1.0));
	visShow(pentity);
#endif 

	TriMeshPtr targetMesh = target->globalMesh(false);
	bool isSurround = m_fc.embossType >= 2;
	m_fontMesh->setState(isSurround);
	m_fontMesh->FontTransform(targetMesh.get(), faceId, cross, isSurround);

	m_lastFaceId = faceId;
	QVector3D qCross(cross[0], cross[1], cross[2]);
	m_lastCross = qCross;

	if (!m_hasEmbossed)
	{
		m_hasEmbossed = true;
		m_initMatrix = m_relief->localMatrix();
	}
}

void FontMeshWrapper::syncScale()
{

}

FontConfig* FontMeshWrapper::config()
{
	return &m_fc;
}
	
trimesh::vec3 FontMeshWrapper::faceTo()
{
	return m_fontMesh->currentFaceTo();
}

QString FontMeshWrapper::text()
{
	QString text = QString(m_fc.text.data());
	return text;
}

float FontMeshWrapper::angle()
{
	return m_fc.angle;
}

float FontMeshWrapper::height()
{
	return m_fontMesh->height();
}

/* modeln */
ModelN* FontMeshWrapper::model()
{
	return m_relief;
}

ModelGroup* FontMeshWrapper::group()
{
	return m_relief->getModelGroup();
}

void FontMeshWrapper::createModel(creative_kernel::ModelGroup* group)
{
	if (m_relief)	// model is exist already
		return;

	traceSpace(this);

	updateMeshData();

	if (group == NULL)
	{
		SharedDataID id;
		id(MeshID) = registerMeshData(m_meshData, false);

		ModelCreateData createData;
		createData.dataID = id;
		createData.name = generateReliefName(group);
		// createData.xf = trimesh::xform::identity();
		createData.xf = trimesh::xform(qtuser_3d::qMatrix2Xform(m_meshMatrix));

		GroupCreateData groupCreateData;
		groupCreateData.models << createData;
		groupCreateData.name = createData.name;
		groupCreateData.defaultColorIndex = 1;

		QVector3D location = generateDefaultLoaction(group, m_meshData->calculateBox());		
		trimesh::xform xf = trimesh::xform::identity();
		trimesh::dbox3 box = m_meshData->calculateBox();
		xf = xf.trans(location.x(), location.y(), location.z() - box.size()[2] / 2.0);
		groupCreateData.xf = xf;

		SceneCreateData sceneCreateData;
		sceneCreateData.add_groups << groupCreateData;

		modifyScene(sceneCreateData);
	}
	else 
	{
		SceneCreateData scene_data;

		GroupCreateData group_create_data;
		group_create_data.model_group_id = group->getObjectId();
		group_create_data.xf = group->getMatrix();

		ModelCreateData mesh_model_data;
		mesh_model_data.dataID(MeshID) = registerMeshData(m_meshData, false);
		mesh_model_data.dataID(ModelType) = (int)ModelNType::normal_part;
		mesh_model_data.dataID(MaterialID) = group->defaultColorIndex();
		mesh_model_data.name = generateReliefName(group);

		trimesh::dbox3 box = m_meshData->calculateBox();
		QVector3D offset = generateDefaultLoaction(group, box);
// - box.size()[2]

		float groupatrixZ = group_create_data.xf[14];

		QMatrix4x4 textXf = qtuser_3d::xform2QMatrix(trimesh::fxform(group_create_data.xf));
		textXf = textXf.inverted();
		textXf.setColumn(3, QVector4D(0, 0, 0, 1));
		textXf.translate(offset.x(), offset.y(), -groupatrixZ);
		textXf *= m_meshMatrix;

		mesh_model_data.xf = trimesh::xform(qtuser_3d::qMatrix2Xform(textXf));

		addModel(mesh_model_data, group);
	}

	unTraceSpace(this);
}

void FontMeshWrapper::updateModel(bool reset)
{
	if (!m_relief)
		return;

	if (reset)
	{
		ModelGroup* group = m_relief->getModelGroup();
		trimesh::xform m = trimesh::inv(group->getMatrix());

		trimesh::dvec3 src_w = m_relief->globalBoundingBox().center();
		trimesh::dvec3 src_l = m * src_w;
		trimesh::dvec3 dst_l = m * trimesh::dvec3(0, 0, 0);

		trimesh::dvec3 tDelta = dst_l - src_l;
		trimesh::xform xf = trimesh::xform::trans(tDelta);
		m_relief->applyMatrix(xf);

		QMatrix4x4 reliefMatrix = m_relief->localMatrix();
		updateMeshData();
		reliefMatrix = m_meshMatrix * reliefMatrix;
		trimesh::xform matrix = trimesh::xform(qtuser_3d::qMatrix2Xform(reliefMatrix));
		m_relief->setMatrix(matrix);
	}
	else
	{
		QMatrix4x4 reliefMatrix = m_relief->localMatrix();
		reliefMatrix *= m_meshMatrix.inverted();
		updateMeshData();
		reliefMatrix *= m_meshMatrix;
		trimesh::xform matrix = trimesh::xform(qtuser_3d::qMatrix2Xform(reliefMatrix));
		m_relief->setMatrix(matrix);
	}

	int meshID = m_relief->meshID();
	coverMeshData(m_meshData, meshID);

	m_relief->updateNode();
	m_relief->updateRender(false);
	m_relief->needRegenerate();
	m_relief->setCacheGlobalBoxDirty();
	m_relief->dirtyNode();
	notifySpaceChange();
}	

void FontMeshWrapper::updateInitModel()
{
	if (!m_relief)
		return;

	int meshID = m_relief->meshID();
	coverMeshData(m_meshData, meshID);

	m_relief->updateNode();
	m_relief->updateRender(false);
	m_relief->setCacheGlobalBoxDirty();
	m_relief->dirtyNode();
	notifySpaceChange();
}	

/***** private *****/
void FontMeshWrapper::updateMeshData()
{
#if CHECK_OUTLINE
	 trimesh::TriMesh* mesh = new trimesh::TriMesh;
	 mesh->vertices.push_back(trimesh::vec3(0, 0, 0));
	 mesh->vertices.push_back(trimesh::vec3(1, 0, 0));
	 mesh->vertices.push_back(trimesh::vec3(2, 0, 0));
	 TriMeshPtr meshPtr(mesh);
	 trimesh::vec3 offset = meshPtr->bbox.center();

#else
	TriMeshPtr meshPtr(m_fontMesh->getFontMesh());
	meshPtr->need_bbox();
	trimesh::vec3 offset = meshPtr->bbox.center();
	if (m_relief)
	{
		trimesh::xform ixf = trimesh::inv(m_relief->globalDMatrix());
		trimesh::apply_xform(meshPtr.get(), ixf);

		ModelGroup* group = m_relief->getModelGroup();
		trimesh::xform m = trimesh::inv(group->getMatrix());
		m[12] = m[13] = m[14] = 0;
		offset = m * offset;
	}
#endif

	cxkernel::MeshData* meshData = new cxkernel::MeshData(meshPtr, true);
	meshData->offset = trimesh::vec3(0, 0, 0);
	cxkernel::MeshDataPtr meshDataPtr(meshData);

	QMatrix4x4 transMatrix;
	transMatrix.setColumn(3, QVector4D(offset[0], offset[1], offset[2], 1));
	m_meshMatrix = transMatrix;
	m_meshData = meshDataPtr;
}

void FontMeshWrapper::autoEmboss()
{
	ModelN* target = getModelNByFixedId(m_fc.reliefTargetID);
	if (!target)
	{	/* auto find target */
		if (!m_relief)
			return;
		
		/* find first normal part */
		QList<ModelN*> parts = m_relief->getModelGroup()->models();
		for (ModelN* part : parts)
		{
			if (part->isRelief() || 
				part->modelNType() != ModelNType::normal_part)
				continue;

			target = part;
			break;
		}

		if (!target)	// cannot find target part
			return;

		// cxkernel::MeshDataPtr meshData = targetPart->data();
		TriMeshPtr meshPtr = target->globalMesh();
		int faceCount = meshPtr->faces.size();
		if (faceCount == 0)
			return;

		/* create ray, relief center line to target center  */
		qtuser_3d::Box3D targetBox = target->globalSpaceBox();
		qtuser_3d::Box3D reliefBox = m_relief->globalSpaceBox();
		QVector3D targetCenter = targetBox.center();
		QVector3D reliefCenter = reliefBox.center();
		reliefCenter.setZ(targetCenter.z());
		QVector3D rayDir = targetCenter - reliefCenter;
		qtuser_3d::Ray ray(reliefCenter, rayDir);

#if SHOW_RAY
		static qtuser_3d::LineEntity* line = new qtuser_3d::LineEntity();
		line->setLineWidth(2);

		QVector<QVector3D> positions;
		QVector<QVector4D> colors;
		positions << targetCenter;
		positions << reliefCenter;
		colors << QVector4D(1.0, 1.0, 0.0, 1.0);
		colors << QVector4D(1.0, 1.0, 0.0, 1.0);

		line->updateGeometry(positions, colors, false);
		visShow(line);
#endif 

		m_fc.reliefTargetID = target->getFixedId();

		int faceId = 0;
		QVector3D faceNormal;
		bool foundDefaultFace = false;
		trimesh::vec3 defaultCross;
		int defaultFace = -1;
		float maxZ = -9999;
		float maxDot = -9999;

		for (; faceId < faceCount; ++faceId)
		{
			QVector3D collide;
			if (target->rayCheck(faceId, ray, collide, &faceNormal, true) &&
				QVector3D::dotProduct(faceNormal, rayDir) < 0)
			{
#if SHOW_RAY
				static qtuser_3d::AlonePointEntity* pentity = new qtuser_3d::AlonePointEntity();
				pentity->setPointSize(8);
				pentity->setFilterType("alpha");
				pentity->updateGlobal(QVector3D(collide[0], collide[1], collide[2]));
				pentity->setColor(QVector4D(1.0, 1.0, 1.0, 1.0));
				visShow(pentity);
#endif 

				trimesh::vec3 cross(collide.x(), collide.y(), collide.z());
				emboss(faceId, cross);
				return;
			}

			// if (!foundDefaultFace)
			{
				auto face = meshPtr->faces[faceId];
				trimesh::vec3 v1 = meshPtr->vertices[face[0]];
				trimesh::vec3 v2 = meshPtr->vertices[face[1]];
				trimesh::vec3 v3 = meshPtr->vertices[face[2]];
				if (v1[2] >= maxZ)
				{
					QVector3D qv1(v1[0], v1[1], v1[2]);
					QVector3D qv2(v2[0], v2[1], v2[2]);
					QVector3D qv3(v3[0], v3[1], v3[2]);
					QVector3D normal = QVector3D::normal(qv1, qv2, qv3);
					float dot = QVector3D::dotProduct(QVector3D(0, 0, 1), normal);
					if (dot > 0)
					{
						if (v1[2] == maxZ)
						{
							if (maxDot < dot)
							{
								maxDot = dot;
								maxZ = v1[2];
								foundDefaultFace = true;
								defaultCross = (v1 + v2 + v3) / 3.0;
								defaultFace = faceId;
							}
						}
						else 
						{
							maxDot = dot;
							maxZ = v1[2];
							foundDefaultFace = true;
							defaultCross = (v1 + v2 + v3) / 3.0;
							defaultFace = faceId;
						}
					}
				}
			}
		}

		if (!foundDefaultFace)
		{
			auto face = meshPtr->faces[0];
			trimesh::vec3 v1 = meshPtr->vertices[face[0]];
			trimesh::vec3 v2 = meshPtr->vertices[face[1]];
			trimesh::vec3 v3 = meshPtr->vertices[face[2]];
			defaultCross = (v1 + v2 + v3) / 3.0;
			defaultFace = 0;
		}

#if SHOW_RAY
		static qtuser_3d::AlonePointEntity* pentity = new qtuser_3d::AlonePointEntity();
		pentity->setPointSize(16);
		pentity->setFilterType("alpha");
		pentity->updateGlobal(QVector3D(defaultCross[0], defaultCross[1], defaultCross[2]));
		pentity->setColor(QVector4D(0.7, 0.6, 0.2, 1.0));
		visShow(pentity);
#endif 
		emboss(defaultFace, defaultCross);
	}
	else 
	{	/* emboss to last cross */
		QMatrix4x4 currentMatrix = target->globalMatrix();
		QMatrix4x4 transMatrix = currentMatrix * m_targetMatrix.inverted();
		QVector3D globalCross = transMatrix.map(m_lastCross);
		trimesh::vec3 tCross(globalCross[0], globalCross[1], globalCross[2]);
		emboss(m_lastFaceId, tCross);

#if SHOW_RAY
		static qtuser_3d::AlonePointEntity* pentity = new qtuser_3d::AlonePointEntity();
		pentity->setPointSize(16);
		pentity->setFilterType("alpha");
		pentity->updateGlobal(QVector3D(globalCross[0], globalCross[1], globalCross[2]));
		pentity->setColor(QVector4D(0.0, 0.0, 1.0, 1.0));
		visShow(pentity);
#endif 
	}
}

QString FontMeshWrapper::generateReliefName(ModelGroup* group)
{
	return "Relief";
}

QVector3D FontMeshWrapper::generateDefaultLoaction(ModelGroup* group, const trimesh::dbox3& reliefBox)
{
	if (group)
	{
		qtuser_3d::Box3D box;
		for (ModelN* model : group->models())
		{
			if (!model->isRelief())
			{
				box += model->globalSpaceBox();
			}
		}
		QVector3D groupCenter = box.center();
		QVector3D groupSize = box.size();
		auto size = reliefBox.size();
		groupCenter.setZ(0);

		QVector3D result(0, (-groupSize.y() - size[1]) / 2.0 - 5, 0);
		result += groupCenter;

		QMatrix4x4 groupMatrix = group->globalMatrix();
		groupMatrix = groupMatrix.inverted();
		result = groupMatrix.map(result);

		return result;
	}
	else
	{
		qtuser_3d::Box3D printerBox = currentPrinterGlobalBox();
		QVector3D center = printerBox.center();

		ModelGroup* baseGroup = NULL;
		QList<ModelGroup*> groups = getCurrentPrinterModelGroups();
		for (ModelGroup* group : groups)
		{
			QList<ModelN*> models = group->models();
			if (models.size() == 1 && models.first()->isRelief())
				continue;

			qtuser_3d::Box3D box = group->globalBox();
			if (box.min.x() < center.x() && box.max.x() > center.x() &&
				box.min.y() < center.y() && box.max.y() > center.y())
			{
				baseGroup = group;
				break;
			}
		}

		if (baseGroup)
		{
			auto size = reliefBox.size();
			qtuser_3d::Box3D box;
			for (ModelN* model : baseGroup->models())
			{
				if (!model->isRelief())
				{
					box += model->globalSpaceBox();
				}
			}
			QVector3D result(center.x(), box.min.y() - 5 - size[1] / 2.0, size[2] / 2.0);
			return result;
		}
		else
		{
			auto size = reliefBox.size();
			center.setZ(size[2] / 2.0);
			return center;
		}
	}
}

std::string FontMeshWrapper::matrix2String(const QMatrix4x4& matrix)
{
	std::stringstream stream;
	for (int c = 0; c < 4; ++c)
	{
		QVector4D column = matrix.column(c);
		stream << column[0] << " " <<
							column[1] << " " <<
							column[2] << " " <<
							column[3];
		if (c != 3)
			stream << " ";
	}
	return stream.str();
}

std::string FontMeshWrapper::serialize(bool isClone)
{
	std::stringstream stream;
	std::string wrapperStr = std::to_string(m_lastFaceId) + " " +
		std::to_string(m_lastCross[0]) + " " + std::to_string(m_lastCross[1]) + " " + std::to_string(m_lastCross[2]) + " " +
		matrix2String(m_targetMatrix) + " " +
		matrix2String(m_firstEmbossMatrix) + " " +
		std::to_string((int)m_useFirst) + " " +
		std::to_string((int)m_hasEmbossed) + " " +
		matrix2String(m_initMatrix);

	m_fc.fontMeshData = m_fontMesh->getDataToString();

	stream << "    <relief key=\"description\" value=\"text=" << std::string(m_fc.text.toLatin1()) << "| |" <<
		"font=" << std::string(m_fc.font.toLatin1()) << "| |" <<
		"font_size=" << std::to_string(m_fc.fontSize) << "| |" <<
		"word_space=" << std::to_string(m_fc.wordSpace) + "| |" <<
		"line_space=" << std::to_string(m_fc.lineSpace) + "| |" <<
		"height=" << std::to_string(m_fc.height) + "| |" <<
		"distance=" << std::to_string(m_fc.distance) + "| |" <<
		"emboss_type=" << std::to_string(m_fc.embossType) + "| |" <<
		"bold=" << std::to_string((int)m_fc.bold) + "| |" <<
		"italic=" << std::to_string((int)m_fc.italic) + "| |" <<
		"angle=" << std::to_string(m_fc.angle) + "| |" <<
		"wrapper=" << wrapperStr + "| |"  << 
		"font_mesh=" + m_fc.fontMeshData + "| |"  << 
 		"is_clone=" + std::to_string((int)isClone) + "| |\"/>\n";

	std::string out = stream.str();
	// qDebug() << out.data();

	return out;
}

void FontMeshWrapper::parseDescription(const std::string& reliefDescription)
{
	auto startsWith = [=](const std::string& baseStr, const std::string& subStr, std::string& otherStr)
	{
		int length = subStr.length();
		if (baseStr.substr(0, length) == subStr)
		{
			otherStr = baseStr.substr(length);
			return true;
		}
		else
			return false;
	};

	int p1 = 0, p2 = 0;
	while (true)
	{
		p1 = reliefDescription.find("| |", p1);
		if (p1 == -1)
			break;
		
		
		std::string attrStr = reliefDescription.substr(p2, p1 - p2);
		//qDebug() << attrStr.data();
		p1 += 2;
		p2 = p1 + 1;

		std::string value;
		if (startsWith(attrStr, "text=", value))
		{
			m_fc.text = QString(value.data());
		}
		else if (startsWith(attrStr, "font=", value))
		{
			m_fc.font = QString(value.data());
		}
		else if (startsWith(attrStr, "font_size=", value))
		{
			m_fc.fontSize = std::stoi(value);
		}
		else if (startsWith(attrStr, "word_space=", value))
		{
			m_fc.wordSpace = std::stof(value);
		}
		else if (startsWith(attrStr, "line_space=", value))
		{
			m_fc.lineSpace = std::stof(value);
		}
		else if (startsWith(attrStr, "height=", value))
		{
			m_fc.height = std::stof(value);
		}
		else if (startsWith(attrStr, "distance=", value))
		{
			m_fc.distance = std::stof(value);
		}
		else if (startsWith(attrStr, "emboss_type=", value))
		{
			m_fc.embossType = std::stoi(value);
		}
		else if (startsWith(attrStr, "bold=", value))
		{
			m_fc.bold = std::stoi(value);
		}
		else if (startsWith(attrStr, "italic=", value))
		{
			m_fc.italic = std::stoi(value);
		}
		else if (startsWith(attrStr, "angle=", value))
		{
			m_fc.angle = std::stof(value);
		}
		else if (startsWith(attrStr, "wrapper=", value))
		{
			QStringList nums = QString(value.data()).split(" ");
			if (nums.length() == 54)
			{
				int i = 0;
				m_lastFaceId = nums[i++].toInt();
				m_lastCross = QVector3D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat());

				QMatrix4x4 matrix;
				matrix.setColumn(0, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				matrix.setColumn(1, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				matrix.setColumn(2, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				matrix.setColumn(3, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				m_targetMatrix = matrix;

				matrix.setColumn(0, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				matrix.setColumn(1, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				matrix.setColumn(2, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				matrix.setColumn(3, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				m_firstEmbossMatrix = matrix;

				m_useFirst = nums[i++].toInt();
				m_hasEmbossed = nums[i++].toInt();

				matrix.setColumn(0, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				matrix.setColumn(1, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				matrix.setColumn(2, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				matrix.setColumn(3, QVector4D(nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat(), nums[i++].toFloat()));
				m_initMatrix = matrix;
			}
		}
		else if (startsWith(attrStr, "font_mesh=", value))
		{
			m_fc.fontMeshData = value;
		}
		else if (startsWith(attrStr, "is_clone=", value))
		{
			m_isCloned = std::stoi(value);
		}

	}
	m_outlineGenerator = new OutlineGenerator(this);
	m_outlineGenerator->applyFontConfig(m_fc);
	m_outlineGenerator->generate();
	std::vector<OutLine> outline = m_outlineGenerator->result();
	// m_fontMesh = new topomesh::FontMesh(m_fc.fontMeshData);
	m_fontMesh = new topomesh::FontMesh(m_fc.fontMeshData, outline);
}

/* protected */
void FontMeshWrapper::onModelGroupModified(ModelGroup* _model_group, const QList<ModelN*>& removes, const QList<ModelN*>& adds)
{
	if (!adds.isEmpty())
	{
		ModelN* newModel = adds.first();
		selectModelN(newModel);
		m_relief = newModel;
		m_relief->setFontMesh(this);
		m_reliefId = m_relief->getFixedId();
	}

}
