#ifndef QTUSER_3D_X_PICKENTITY_1595086493891_H
#define QTUSER_3D_X_PICKENTITY_1595086493891_H
#include "qtuser3d/refactor/xentity.h"
#include "qtuser3d/module/pickable.h"

namespace qtuser_3d
{
	class QTUSER_3D_API PickXEntity : public XEntity
	{
		Q_OBJECT
	public:
		PickXEntity(Qt3DCore::QNode* parent = nullptr);
		virtual ~PickXEntity();

		Pickable* pickable();
		void bindPickable(Pickable* pickable);

		virtual void setVisualState(qtuser_3d::ControlState state);
		void setVisualVertexBase(const QPoint& vertexBase);
	protected slots:
		void slotStateChanged(ControlState state);
		void slotFaceBaseChanged(int faceBase);
	protected:
		virtual void onStateChanged(ControlState state);

	protected:
		Pickable* m_pickable;
		Qt3DRender::QParameter* m_vertexBaseParameter;
		Qt3DRender::QParameter* m_stateParameter;
	};
}
#endif // QTUSER_3D_X_PICKENTITY_1595086493891_H