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

		//ԭ���ڵײ�����
		const qtuser_3d::Box3D& localBox();
		void setLocalBox(const qtuser_3d::Box3D& box);
		void setLocalBoxOnly(const qtuser_3d::Box3D& box);

		void setColors(const QList<QVector4D>& colors);

		bool selected();
		void setSelected(bool selected);

		//������ԭ��(�ڵײ�����)�ڵ�ǰ��ӡƽ̨�����λ��
		QVector3D position();
		void translateTo(const QVector3D& position);
		bool shouldTranslateTo(const QVector3D& position, QVector3D& betterPosition);

		//���������½��ڵ�ǰ��ӡƽ̨�����λ��
		QVector3D positionOfLeftBottom();
		void setLeftBottomPosition(float x, float y);

		//��ǰ��ӡƽ̨�İ�Χ��
		const qtuser_3d::Box3D& printerBox();
		void setPrinterBox(const qtuser_3d::Box3D& box);

		//����������ƽ̨�߽�ľ���
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


