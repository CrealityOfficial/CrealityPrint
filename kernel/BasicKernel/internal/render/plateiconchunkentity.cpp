#include "plateiconchunkentity.h"
#include "renderpass/purerenderpass.h"
#include "qtuser3d/refactor/xeffect.h"
#include "qtuser3d/utils/texturecreator.h"
#include "external/kernel/kernelui.h"
#include "interface/camerainterface.h"
#include "interface/visualsceneinterface.h"
#include "qtuser3d/camera/screencamera.h"
#include "qtuser3d/math/space3d.h"
#include "qtuser3d/geometry/trianglescreatehelper.h"
#include <QDebug>


using namespace qtuser_3d;

namespace creative_kernel 
{

	MultiIconChunk::MultiIconChunk(QObject* parent)
		:Pickable(parent)
	{
	}

	MultiIconChunk::~MultiIconChunk()
	{
	}

	int MultiIconChunk::primitiveNum()
	{
		return 18;
	}

	bool MultiIconChunk::hoverInRange(int faceid)
	{
		int faceEnd = m_faceBase + (noPrimitive() ? 1 : primitiveNum());

		if (faceid >= m_faceBase && faceid < faceEnd && isVisible())
		{
			if (faceid - m_faceBase < 6)
				return true;
		}

		return false;
	}


	PlateIconChunkEntity::PlateIconChunkEntity(Qt3DCore::QNode* parent)
		: qtuser_3d::PickXEntity(parent)
		, m_type(IconType::Unknow)
		, m_callback(nullptr)
		, m_pickPass(nullptr)
	{
		m_multiIconPickable = new MultiIconChunk();
		bindPickable(m_multiIconPickable);
		m_pickable->setNoPrimitive(false);
		m_pickable->setEnableSelect(false);

		XRenderPass* viewPass = new XRenderPass("colormapex", parent);
		viewPass->addFilterKeyMask("view", 0);
		viewPass->setPassCullFace(Qt3DRender::QCullFace::Back);
		viewPass->setPassBlend();

		qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("purePickFace");
		pickPass->addFilterKeyMask("pick", 0);
		pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
		pickPass->setPassCullFace();
		pickPass->setPassDepthTest();
		m_pickPass = pickPass;

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(viewPass);
		effect->addRenderPass(pickPass);

		setEffect(effect);

	}

	PlateIconChunkEntity::~PlateIconChunkEntity()
	{
		m_callback = nullptr;
	}

	void PlateIconChunkEntity::setTexture(Qt3DRender::QTexture2D* tex, qtuser_3d::ControlState state)
	{
		m_map.insert(state, tex);

		ControlState currentState = pickable()->state();
		if (currentState == state)
		{
			//PlateTextureComponentEntityEx::setTexture(tex);
			if (tex)
			{
				setParameter("colorMap", QVariant::fromValue(tex));
			}
		}
	}

	void PlateIconChunkEntity::onStateChanged(ControlState state)
	{
		Qt3DRender::QTexture2D* tex = m_map[state];

		//PlateTextureComponentEntityEx::setTexture(tex);
		if (tex)
		{
			setParameter("colorMap", QVariant::fromValue(tex));
		}

		if (state == ControlState::hover && !m_tips.isEmpty())
		{
			QMatrix4x4 globalPose;
			qtuser_3d::XEntity* entity = this;
			do {
				globalPose *= entity->pose();
				entity = dynamic_cast<qtuser_3d::XEntity*>(entity->parent());
			} while (entity);

			QVector3D position = globalPose.column(3).toVector3D();

			qtuser_3d::ScreenCamera* screenCamera = visCamera();
			
			QString tips;

			int faceBase = m_pickable->faceBase();
			int curFaceId = m_pickable->getCurPrimitiveId();

			int faceOffset = curFaceId - faceBase;

			qInfo() << "==== curFaceId is : " << curFaceId;

			if(faceOffset < 2)
			{
				
				setParameter("primiveId", QVariant::fromValue(0));   // area 1;
				position += m_tipsOffset * 1;
				tips = "other";
			}

			else if(faceOffset >= 2 && faceOffset < 4)
			{
				setParameter("primiveId", QVariant::fromValue(1));  // area 3;
				position += m_tipsOffset * 2;
				tips = "close";
				m_type = IconType::Close;
			}

			else if(faceOffset >= 4 && faceOffset < 6 )
			{
				setParameter("primiveId", QVariant::fromValue(2));  // area 2;
				position += m_tipsOffset * 3;
				tips = "layout";
				m_type = IconType::Autolayout;
			}

			QPoint pos = screenCamera->mapToScreen(position);
			getKernelUI()->showToolTip(pos, tips);
		}
		else
		{
			// 10 is invalid value
			setParameter("primiveId", QVariant::fromValue(10));

			getKernelUI()->hideToolTip();
		}
	}

	void PlateIconChunkEntity::makeMultiQuad(float w, float h, int n, std::vector<trimesh::vec3>* vertex)
	{
		float vStep = h / n;
		float step1 = vStep * 1.0;
		float step2 = vStep * 2.0;
		float step3 = vStep * 3.0;
		float step4 = vStep * 4.0;
		float step5 = vStep * 5.0;
		float step6 = vStep * 6.0;

		float hUnit = w / 3.0f;
		float vUnit = vStep / 3.0f;

		trimesh::vec3 ori = trimesh::vec3(0.0, 0.0, 0.0);

		vertex->push_back(trimesh::vec3(hUnit, vUnit, 0.0f));        //  v0
		vertex->push_back(trimesh::vec3(w - hUnit, vUnit, 0.0));       //  v1
		vertex->push_back(trimesh::vec3(hUnit, step1 - vUnit, 0.0));   //  v2
		vertex->push_back(trimesh::vec3(w - hUnit, step1 - vUnit, 0.0)); //  v3


		vertex->push_back(trimesh::vec3(hUnit, step1 + vUnit, 0.0f));        //  v4
		vertex->push_back(trimesh::vec3(w - hUnit, step1 + vUnit, 0.0));     // v5
		vertex->push_back(trimesh::vec3(hUnit, step2 - vUnit, 0.0));   //  v6
		vertex->push_back(trimesh::vec3(w - hUnit, step2 - vUnit, 0.0)); // v7


		vertex->push_back(trimesh::vec3(hUnit, step2 + vUnit, 0.0f));        //  v8
		vertex->push_back(trimesh::vec3(w - hUnit, step2 + vUnit, 0.0));       //  v9
		vertex->push_back(trimesh::vec3(hUnit, step3 - vUnit, 0.0));   //  v10
		vertex->push_back(trimesh::vec3(w - hUnit, step3 - vUnit, 0.0)); // v11

		vertex->push_back(trimesh::vec3(0.0, 0.0, 0.0));        //  v12
		vertex->push_back(trimesh::vec3(hUnit, 0.0, 0.0));       //  v13
		vertex->push_back(trimesh::vec3(0.0, step3, 0.0));   //  v14
		vertex->push_back(trimesh::vec3(hUnit, step3, 0.0)); // v15

		vertex->push_back(trimesh::vec3(w - hUnit, 0.0, 0.0f));        //  v16
		vertex->push_back(trimesh::vec3(w, 0.0, 0.0));       //  v17
		vertex->push_back(trimesh::vec3(w - hUnit, step3, 0.0));   //  v18
		vertex->push_back(trimesh::vec3(w, step3, 0.0)); // v19
	}

	void PlateIconChunkEntity::onClick(int primitiveID)
	{
		if (m_callback)
		{
			m_callback(m_type);
		}
	}

	void PlateIconChunkEntity::setClickCallback(OnClickFunc call)
	{
		m_callback = call;
	}

	void PlateIconChunkEntity::setIconType(IconTypes type)
	{
		m_type = type;
	}

	void PlateIconChunkEntity::setTips(const QString& tips)
	{
		m_tips = tips;
	}

	void PlateIconChunkEntity::setTipsOffset(const QVector3D& offset)
	{
		m_tipsOffset = offset;
	}

	void PlateIconChunkEntity::enablePick(bool enable)
	{
		m_pickPass->setEnabled(enable);
		if (enable)
		{
			onStateChanged(ControlState::none);
		}
		else {

		}
	}

	void PlateIconChunkEntity::createIcons(int width, int height, int n)
	{
		std::vector<trimesh::vec3> vertexs;
		std::vector<trimesh::vec2> uvs;

		trimesh::vec3 origin(0.0);

		// total 3 quads, each quad 30.0f height
		makeMultiQuad(50.0f, 90.0f, 3, &vertexs);

		unsigned multi_quad_triangles_indices[54] = {
2, 0, 1,
2, 1, 3,
6, 4, 5,
6, 5, 7,
10, 8, 9,
10, 9, 11,

14, 12, 13,
14, 13, 15,

18, 16, 17,
18, 17, 19,

0, 13, 16,
0, 16, 1,

4, 2, 3,
4, 3, 5,

8, 6, 7,
8, 7, 9,

15, 10, 11,
15, 11, 18


		};

		makeTextureCoordinate(&vertexs,
			origin,
			trimesh::vec2(50.0, 90.0), &uvs);

		Qt3DRender::QGeometry* shareGeometry = TrianglesCreateHelper::createWithTex(vertexs.size(), &vertexs.front().front(), nullptr, &uvs.front().front(), 18, multi_quad_triangles_indices, this);

		{
			setObjectName("close");
			setGeometry(shareGeometry);
			setIconType(IconType::Close);
			setTips("Close");

		}
	}

}

