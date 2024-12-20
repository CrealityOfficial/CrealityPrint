#include "spreadmodel.h"
#include "msbase/space/intersect.h"
#include "spreadchunk.h"
#include "qtuser3d/refactor/xeffect.h"
#include <QtMath>
#include "kernel/kernel.h"
#include "external/kernel/reuseablecache.h"
#include "kernel/visualscene.h"

#define MAX_COLOR_NUM 16

using namespace creative_kernel;

SpreadModel::SpreadModel(QObject* object)
	: QObject(object)
{
	m_effect = new qtuser_3d::XEffect();
	{
		qtuser_3d::XRenderPass* viewPass = new qtuser_3d::XRenderPass("spread", m_effect);
		viewPass->addFilterKeyMask("view", 0);
		viewPass->setPassCullFace();
		viewPass->setPassDepthTest();
		m_renderModeParameter = viewPass->setParameter("renderModel", 1);
		m_effect->addRenderPass(viewPass);

		VisualScene* visScene = getKernel()->visScene();
		connect(visScene, &VisualScene::modelVisualModeChanged, this, &SpreadModel::updateRenderMode);

		qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("pickFace_pwd", m_effect);
		pickPass->addFilterKeyMask("pick", 0);
		pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
		pickPass->setPassCullFace();
		pickPass->setPassDepthTest();
		m_effect->addRenderPass(pickPass);
	}

	updateRenderMode();

}

SpreadModel::~SpreadModel()
{
	clear();
	m_effect->deleteLater();
}

void SpreadModel::init()
{
	setSection(QVector3D(0.0, 0.0, 100000.0), QVector3D(0.0, 0.0, -10000.0), QVector3D(0.0, 0.0, -1.0));
	m_effect->setParameter("internalColor", QVector4D(0.88, 0.55, 0.18, 1.0));
}

void SpreadModel::clear()
{
	m_effect->setParent((Qt3DCore::QNode*)NULL);
	qDeleteAll(m_chunks);
	m_chunks.clear();
}

void SpreadModel::addChunk(const std::vector<trimesh::vec3>& positions, const std::vector<int>& flags)
{
	SpreadChunk* chunk = new SpreadChunk(m_chunks.size(), this);
	chunk->setDefaultFlag(m_defaultFlag);
	chunk->updateData(positions, flags);
	chunk->setEffect(m_effect);
	m_chunks << chunk;
}

int SpreadModel::chunkCount()
{
	return m_chunks.size(); 
}

void SpreadModel::updateChunkData(int id, const std::vector<trimesh::vec3>& position, const std::vector<int>& flags)
{
	if ((chunkCount() <= id) || id < 0)
		return;

	SpreadChunk* chunk = m_chunks[id];
	chunk->updateData(position, flags);
}

void SpreadModel::updateChunkData(int id, const std::vector<trimesh::vec3>& position, const std::vector<int>& flags, const std::vector<int>& originFlags)
{
	if ((chunkCount() <= id) || id < 0)
		return;

	SpreadChunk* chunk = m_chunks[id];
	chunk->updateData(position, flags, originFlags);
}

void SpreadModel::setChunkIndexMap(int id, const std::vector<int>& indexMap)
{
	if ((chunkCount() <= id) || id < 0)
		return;

	SpreadChunk* chunk = m_chunks[id];
	chunk->setIndexMap(indexMap);
}

void SpreadModel::setChunkMatrixInfo(int id, const QMatrix4x4& modelGlobalMat)
{
	if ((chunkCount() <= id) || id < 0)
		return;

	SpreadChunk* chunk = m_chunks[id];
}

void SpreadModel::setPose(const QMatrix4x4& pose)
{
	for (int i = 0, count = m_chunks.size(); i < count; ++i)
	{
		m_chunks[i]->setPose(pose);
		/* test */
		// QMatrix4x4 p = pose;
		// p.translate(30 * i, 0);
		// m_chunks[i]->setPose(p);
	}
}

QMatrix4x4 SpreadModel::pose() const
{
	QMatrix4x4 p;
	if (m_chunks.size() > 0)
		p = m_chunks.first()->pose();
	return p;
}

void SpreadModel::setSection(const QVector3D &frontPos, const QVector3D &backPos, const QVector3D &normal)
{
	m_effect->setParameter("sectionNormal", normal);
	m_effect->setParameter("sectionFrontPos", frontPos);
	m_effect->setParameter("sectionBackPos", backPos);
}

void SpreadModel::setPalette(const std::vector<trimesh::vec>& colorsList)
{
	if (colorsList.size() <= 0)
		return;

	QVariantList palette;
	for (int i = 0, colorsCount = colorsList.size(); i < MAX_COLOR_NUM; ++i)
	{
		int _i = i < colorsCount ? i : 0;	
		auto color = colorsList[_i];
		QVector4D qcolor = QVector4D(color[0], color[1], color[2], 1.0);
		palette << qcolor;
	}

	m_effect->setParameter("palette[0]", palette);
}

int SpreadModel::chunkId(qtuser_3d::Pickable* pickable)
{
	for (int i = 0; i < m_chunks.count(); ++i)
	{
		if (m_chunks[i] == pickable)
			return i;
	}
	return -1;
}

bool SpreadModel::getChunkFace(int chunkId, int faceId, trimesh::vec3& p1, trimesh::vec3& p2, trimesh::vec3& p3)
{
	if (chunkId >= m_chunks.count() || chunkId < 0)
		return false;

	return m_chunks[chunkId]->getFace(faceId, p1, p2, p3);
}

int SpreadModel::spreadFaceParentId(int chunkId, int faceId)
{
	if (chunkId >= m_chunks.count() || chunkId < 0)
		return false;

	return m_chunks[chunkId]->parentId(faceId);
}

void SpreadModel::setHighlightOverhangsDeg(float deg)
{
	if (deg == 90)
		deg = 89.99;
	float rad = qDegreesToRadians(deg);
	m_effect->setParameter("highlightOverhangsRadian", rad);
}

void SpreadModel::setNeedHighlightEnabled(bool enabled)
{
	m_effect->setParameter("needHighLight", enabled);
}

void SpreadModel::setNeedMixEnabled(bool enabled)
{
	m_effect->setParameter("needMix", enabled);
}

void SpreadModel::setDefaultFlag(int defaultFlag)
{
	m_defaultFlag = defaultFlag;
	
	for (int i = 0, count = m_chunks.size(); i < count; ++i)
	{
		m_chunks[i]->setDefaultFlag(defaultFlag);
	}
}

int SpreadModel::defaultFlag()
{
	return m_defaultFlag;
}

void SpreadModel::updateRenderMode()
{
	VisualScene* visScene = getKernel()->visScene();
	ModelVisualMode mode = visScene->modelVisualMode();
	if (mode == ModelVisualMode::mvm_face)
		m_renderModeParameter->setValue(1);
	else 
		m_renderModeParameter->setValue(3);
}

void SpreadModel::setMatrixInfo(const QMatrix4x4& globalMat)
{
	m_modelGlobalMatrix = globalMat;

	if (m_effect)
	{
		m_effect->setParameter("model_matrix", globalMat);
	}
}
	