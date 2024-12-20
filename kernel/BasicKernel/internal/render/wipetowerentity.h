#ifndef WIPE_TOWER_ENTITY_202401031608_H
#define WIPE_TOWER_ENTITY_202401031608_H

#include "basickernelexport.h"
#include "qtuser3d/refactor/pxentity.h"
#include "qtuser3d/math/box3d.h"

namespace qtuser_3d
{
	class BoxEntity;
}

namespace creative_kernel {

	class BASIC_KERNEL_API WipeTowerEntity : public qtuser_3d::PickXEntity
	{
		Q_OBJECT
	public:
		WipeTowerEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~WipeTowerEntity();

		void setVisibility(bool visibility);
		bool isVisible();

		//原点在底部中心
		const qtuser_3d::Box3D& localBox();
		void setLocalBox(const qtuser_3d::Box3D& box);
		void setLocalBoxOnly(const qtuser_3d::Box3D& box);

		void setColors(const QList<QVector4D>& colors);

		bool selected();
		void setSelected(bool selected);

		//擦拭塔原点(在底部中心)在当前打印平台的相对位置
		QVector3D position();
		void translateTo(const QVector3D& position);
		bool shouldTranslateTo(const QVector3D& position, QVector3D& betterPosition);

		//擦拭塔左下角在当前打印平台的相对位置
		QVector3D positionOfLeftBottom();
		void setLeftBottomPosition(float x, float y);

		//当前打印平台的包围盒
		const qtuser_3d::Box3D& printerBox();
		void setPrinterBox(const qtuser_3d::Box3D& box);

		//擦拭塔距离平台边界的距离
		QVector3D borderMargin();

	signals:
		void signalPositionChange(QVector3D newPosition);

	protected:
		void setBoundingBoxVisibility(bool visible);
		void updateBoundingBoxEntity();

		Qt3DRender::QGeometry* createSandwichBox(const qtuser_3d::Box3D& box, const QList<QVector4D>& colors);
		void updateSandwichBoxLocal(const qtuser_3d::Box3D& box, const QList<QVector4D>& colors);

	protected:
		void onClick(int primitiveID) override;
		void onStateChanged(qtuser_3d::ControlState state) override;

	private:
		bool m_boundingBoxVisible;
		qtuser_3d::BoxEntity* m_boundingBoxEntity;

		qtuser_3d::Box3D m_printerBox;
		qtuser_3d::Box3D m_localBox;
		QList<QVector4D> m_colors;

		QVector3D m_position;
	};
}



#endif // !WIPE_TOWER_ENTITY_202401031608_H


