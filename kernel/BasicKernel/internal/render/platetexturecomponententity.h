#ifndef PLATE_ICON_COMPONENT_ENTITY_202311151203_H
#define PLATE_ICON_COMPONENT_ENTITY_202311151203_H

#include "basickernelexport.h"
#include "qtuser3d/refactor/pxentity.h"
#include "qtuser3d/geometry/roundedrectanglehelper.h"

#include <Qt3DRender/QTexture>
#include <QToolTip>

namespace qtuser_3d
{
	class XRenderPass;
}

namespace creative_kernel {

	enum IconType {
		Unknow,
		Close,
		Autolayout,
		PickBottom,
		Lock,
		Setting,
		Order,
		EditName,
		Platform
	};
	typedef int IconTypes;
	typedef std::function<void(IconTypes type)> OnClickFunc;

	class PlateTextureComponentEntity : public qtuser_3d::PickXEntity
	{
	public:
		PlateTextureComponentEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PlateTextureComponentEntity();

		void setTexture(Qt3DRender::QTexture2D* tex, bool deleteOldTexture = false);
		
		void setTextureFromImage(const QImage& image, bool deleteOldTexture = false);
		
	protected:
		void deleteTexture(Qt3DRender::QTexture2D* t);

	protected:
		Qt3DRender::QTexture2D* m_texture;
	};

	class PlateIconComponentEntity : public PlateTextureComponentEntity
	{
	public:
		PlateIconComponentEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PlateIconComponentEntity();

		void setTexture(Qt3DRender::QTexture2D* tex, qtuser_3d::ControlState state);

		void setClickCallback(OnClickFunc call);
		void setIconType(IconTypes type);
		void setTips(const QString& tips);
		void setTipsOffset(const QVector3D& offset);

		void enablePick(bool enable);

	protected:
		void onClick(int primitiveID) override;

	protected:
		void onStateChanged(qtuser_3d::ControlState state) override;

	private:
		QMap<qtuser_3d::ControlState, Qt3DRender::QTexture2D*> m_map;

		qtuser_3d::XRenderPass* m_pickPass;

		OnClickFunc m_callback;
		IconTypes m_type;
		QString m_tips;
		QVector3D m_tipsOffset{ 10, 12, 0 };
	};

}

#endif //PLATE_ICON_COMPONENT_ENTITY_202311151203_H