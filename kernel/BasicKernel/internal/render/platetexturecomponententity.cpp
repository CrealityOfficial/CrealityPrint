#include "platetexturecomponententity.h"
#include "renderpass/purerenderpass.h"
#include "qtuser3d/refactor/xeffect.h"
#include "qtuser3d/utils/texturecreator.h"
#include "external/kernel/kernelui.h"
#include "interface/camerainterface.h"
#include "qtuser3d/camera/screencamera.h"
#include <QDebug>

using namespace qtuser_3d;

namespace creative_kernel {

	PlateTextureComponentEntity::PlateTextureComponentEntity(Qt3DCore::QNode* parent)
		: PickXEntity(parent)
		, m_texture(nullptr)
	{
		m_pickable->setNoPrimitive(true);
		m_pickable->setEnableSelect(false);

		XRenderPass* viewPass = new XRenderPass("colormap", this);
		viewPass->addFilterKeyMask("view", 0);
		viewPass->setPassCullFace(Qt3DRender::QCullFace::Back);
		viewPass->setPassBlend();
		viewPass->setPassDepthTest();

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(viewPass);

		setEffect(effect);
	}

	PlateTextureComponentEntity::~PlateTextureComponentEntity()
	{
		//if (m_texture)
		//{
		//	//delete m_texture;
		//	deleteTexture(m_texture);
		//	m_texture = nullptr;
		//}
	}

	void PlateTextureComponentEntity::setTexture(Qt3DRender::QTexture2D* tex, bool deleteOldTexture)
	{
#if DEBUG
		//assert(tex != nullptr);
#endif
		if (tex)
		{
			setParameter("colorMap", QVariant::fromValue(tex));
		}

		if (deleteOldTexture && m_texture)
		{
			//delete m_texture;
			deleteTexture(m_texture);
		}

		m_texture = tex;
	}

	void PlateTextureComponentEntity::setTextureFromImage(const QImage& image, bool deleteOldTexture)
	{
		Qt3DRender::QTexture2D* tex = qtuser_3d::createFromImage(image);
		setTexture(tex, deleteOldTexture);
	}

	void PlateTextureComponentEntity::deleteTexture(Qt3DRender::QTexture2D* t)
	{
		if (t == nullptr)
			return;

		QVector<Qt3DRender::QAbstractTextureImage*> textureImages = t->textureImages();
		for (auto image : textureImages)
		{
			t->removeTextureImage(image);
			delete image;
		}
		delete t;
	}

	/***************************************************************************************************/


	PlateIconComponentEntity::PlateIconComponentEntity(Qt3DCore::QNode* parent)
		: PlateTextureComponentEntity(parent)
		, m_type(IconType::Unknow)
		, m_callback(nullptr)
		, m_pickPass(nullptr)
	{
		XEffect* effect = xeffect();
		if (effect)
		{
			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("purePickFace", this);
			pickPass->addFilterKeyMask("pick", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace(Qt3DRender::QCullFace::Back);
			pickPass->setPassDepthTest();
			effect->addRenderPass(pickPass);
			m_pickPass = pickPass;
		}
		//pickPass->setEnabled(enablePick);
	}

	PlateIconComponentEntity::~PlateIconComponentEntity()
	{
		m_callback = nullptr;
	}

	void PlateIconComponentEntity::setTexture(Qt3DRender::QTexture2D* tex, qtuser_3d::ControlState state)
	{
		if (tex == nullptr)
		{
			return;
		}

		Qt3DRender::QTexture2D* exitTexture = nullptr;
		if (m_map.contains(state)) {
			exitTexture = m_map[state];
		}
			
		m_map.insert(state, tex);

		ControlState currentState = pickable()->state();
		if (currentState == state)
		{
			PlateTextureComponentEntity::setTexture(tex, false);
		}
	}

	void PlateIconComponentEntity::onStateChanged(ControlState state)
	{
		Qt3DRender::QTexture2D* tex = m_map.contains(state) ? m_map[state] : nullptr;
		PlateTextureComponentEntity::setTexture(tex, false);

		if (state == ControlState::hover && !m_tips.isEmpty())
		{
			QMatrix4x4 globalPose;
			qtuser_3d::XEntity* entity = this;
			do {
				globalPose *= entity->pose();
				entity = dynamic_cast<qtuser_3d::XEntity*>(entity->parent());
			} while (entity);

			QVector3D position = globalPose.column(3).toVector3D();
			position += m_tipsOffset;
			qtuser_3d::ScreenCamera* screenCamera = visCamera();
			QPoint pos = screenCamera->mapToScreen(position);
			getKernelUI()->showToolTip(pos, m_tips);
		}
		else
		{
			getKernelUI()->hideToolTip();
		}
	}

	void PlateIconComponentEntity::onClick(int primitiveID)
	{
		if (m_callback)
		{
			m_callback(m_type);
		}
	}

	void PlateIconComponentEntity::setClickCallback(OnClickFunc call)
	{
		m_callback = call;
	}

	void PlateIconComponentEntity::setIconType(IconTypes type)
	{
		m_type = type;
	}

	void PlateIconComponentEntity::setTips(const QString& tips)
	{
		m_tips = tips;
	}

	void PlateIconComponentEntity::setTipsOffset(const QVector3D& offset)
	{
		m_tipsOffset = offset;
	}

	void PlateIconComponentEntity::enablePick(bool enable)
	{
		m_pickPass->setEnabled(enable);
	}
}

