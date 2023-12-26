#include "platetexturecomponententity.h"
#include "renderpass/purerenderpass.h"
#include "qtuser3d/refactor/xeffect.h"

using namespace qtuser_3d;

namespace creative_kernel {

	PlateTextureComponentEntity::PlateTextureComponentEntity(Qt3DCore::QNode* parent)
		: PickXEntity(parent)
	{
		m_pickable->setNoPrimitive(true);
		m_pickable->setEnableSelect(false);

		XRenderPass* viewPass = new XRenderPass("colormap", parent);
		viewPass->addFilterKeyMask("view", 0);
		viewPass->setPassBlend();

		XEffect* effect = new XEffect(this);
		effect->addRenderPass(viewPass);

		setEffect(effect);
	}

	PlateTextureComponentEntity::~PlateTextureComponentEntity()
	{
	}

	void PlateTextureComponentEntity::setTexture(Qt3DRender::QTexture2D* tex)
	{
		/*if (tex->generateMipMaps() == false)
			tex->setGenerateMipMaps(true);*/
		setParameter("colorMap", QVariant::fromValue(tex));
	}



	/***************************************************************************************************/


	PlateIconComponentEntity::PlateIconComponentEntity(Qt3DCore::QNode* parent)
		: PlateTextureComponentEntity(parent)
		, m_type(IconType::Unknow)
		, m_callback(nullptr)
	{
		XEffect* effect = xeffect();
		if (effect)
		{
			qtuser_3d::XRenderPass* pickPass = new qtuser_3d::XRenderPass("purePickFace");
			pickPass->addFilterKeyMask("pick2nd", 0);
			pickPass->setPassBlend(Qt3DRender::QBlendEquationArguments::One, Qt3DRender::QBlendEquationArguments::Zero);
			pickPass->setPassCullFace();
			pickPass->setPassDepthTest();
			effect->addRenderPass(pickPass);
		}
		//pickPass->setEnabled(enablePick);
	}

	PlateIconComponentEntity::~PlateIconComponentEntity()
	{
	}

	void PlateIconComponentEntity::setTexture(Qt3DRender::QTexture2D* tex, qtuser_3d::ControlState state)
	{
		m_map.insert(state, tex);

		ControlState currentState = pickable()->state();
		if (currentState == state)
		{
			PlateTextureComponentEntity::setTexture(tex);
		}
	}

	void PlateIconComponentEntity::onStateChanged(ControlState state)
	{
		Qt3DRender::QTexture2D* tex = m_map[state];
		PlateTextureComponentEntity::setTexture(tex);
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
}

