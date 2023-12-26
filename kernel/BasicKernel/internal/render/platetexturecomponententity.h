#ifndef PLATE_ICON_COMPONENT_ENTITY_202311151203_H
#define PLATE_ICON_COMPONENT_ENTITY_202311151203_H

#include "basickernelexport.h"
#include "qtuser3d/refactor/pxentity.h"
#include "qtuser3d/geometry/roundedrectanglehelper.h"

#include <Qt3DRender/QTexture>

namespace creative_kernel {

	enum IconType {
		Unknow,
		Autolayout,
		PickBottom
	};
	typedef int IconTypes;
	typedef std::function<void(IconTypes type)> OnClickFunc;

	class PlateTextureComponentEntity : public qtuser_3d::PickXEntity
	{
	public:
		PlateTextureComponentEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PlateTextureComponentEntity();

		void setTexture(Qt3DRender::QTexture2D* tex);
		
	protected:

	};



	class PlateIconComponentEntity : public PlateTextureComponentEntity
	{
	public:
		PlateIconComponentEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PlateIconComponentEntity();

		void setTexture(Qt3DRender::QTexture2D* tex, qtuser_3d::ControlState state);

		void setClickCallback(OnClickFunc call);
		void setIconType(IconTypes type);
	protected:
		void onClick(int primitiveID) override;

	protected:
		void onStateChanged(qtuser_3d::ControlState state) override;

	private:
		QMap<qtuser_3d::ControlState, Qt3DRender::QTexture2D*> m_map;

		OnClickFunc m_callback;
		IconTypes m_type;
	};

}

#endif //PLATE_ICON_COMPONENT_ENTITY_202311151203_H