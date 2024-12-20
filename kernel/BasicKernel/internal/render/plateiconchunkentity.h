#ifndef PLATE_ICON_CHUNK_ENTITY_202402211355_H
#define PLATE_ICON_CHUNK_ENTITY_202402211355_H

#include "basickernelexport.h"
#include "qtuser3d/refactor/pxchunkentity.h"
#include "qtuser3d/geometry/roundedrectanglehelper.h"
#include "qtuser3d/geometry/attribute.h"
#include "qtuser3d/module/pickable.h"
#include "platetexturecomponententity.h"

#include <Qt3DRender/QTexture>
#include <QToolTip>

namespace qtuser_3d
{
	class XRenderPass;
}

namespace creative_kernel 
{
	class MultiIconChunk : public qtuser_3d::Pickable
	{
		Q_OBJECT
	public:
		MultiIconChunk(QObject* parent = nullptr);
		virtual ~MultiIconChunk();

		int primitiveNum() override;
		bool hoverInRange(int faceid) override;

	protected:

	};

	//class PlateTextureComponentEntityEx : public qtuser_3d::PickXEntity
	//{
	//public:
	//	PlateTextureComponentEntityEx(Qt3DCore::QNode* parent = nullptr);
	//	virtual ~PlateTextureComponentEntityEx();

	//	void setTexture(Qt3DRender::QTexture2D* tex);

	//	void setTexture(const QImage& image);

	//protected:
	//	MultiIconChunk* m_multiIconPickable;
	//};


	class PlateIconChunkEntity : public qtuser_3d::PickXEntity
	{
	public:
		PlateIconChunkEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PlateIconChunkEntity();

		void setTexture(Qt3DRender::QTexture2D* tex, qtuser_3d::ControlState state);

		void setClickCallback(OnClickFunc call);
		void setIconType(IconTypes type);
		void setTips(const QString& tips);
		void setTipsOffset(const QVector3D& offset);

		void enablePick(bool enable);

		// in a rectangle(width, height), create  n  vertical icons
		void createIcons(int width, int height, int n);

	protected:
		void onClick(int primitiveID) override;
		void onStateChanged(qtuser_3d::ControlState state) override;
		void makeMultiQuad(float w, float h, int n, std::vector<trimesh::vec3>* vertex);

	private:
		QMap<qtuser_3d::ControlState, Qt3DRender::QTexture2D*> m_map;

		qtuser_3d::XRenderPass* m_pickPass;

		OnClickFunc m_callback;
		IconTypes m_type;
		QString m_tips;
		QVector3D m_tipsOffset{ 10, 12, 0 };

		MultiIconChunk* m_multiIconPickable;
	};

}

#endif //PLATE_ICON_CHUNK_ENTITY_202402211355_H