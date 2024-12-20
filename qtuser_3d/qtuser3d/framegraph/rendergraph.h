#ifndef QTUSER_3D_RENDERGRAPH_1594441548195_H
#define QTUSER_3D_RENDERGRAPH_1594441548195_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DCore/QNode>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QFrameGraphNode>
#include <QtCore/QSize>

namespace qtuser_3d
{
	class SceneOperateMode;
	class QTUSER_3D_API RenderGraph : public Qt3DRender::QFrameGraphNode
	{
		Q_OBJECT
	public:
		RenderGraph(Qt3DCore::QNode* parent = nullptr);
		virtual ~RenderGraph();

		void setOperateMode(SceneOperateMode* operateMode);
		SceneOperateMode* operateMode();
		void reinOperationMode();

		virtual Qt3DCore::QEntity* sceneGraph();

		virtual void begineRender();
		virtual void endRender();
		virtual void updateRenderSize(const QSize& size);

		virtual void onRegistered();
		virtual void onUnRegistered();

#ifdef DEBUG
		virtual bool showDebugOverlay() { return false; }
		virtual void setShowDebugOverlay(bool showDebugOverlay){}
#endif
	signals:
		void signalUpdate();

	protected:
		SceneOperateMode* m_operateMode;
	};
}
#endif // QTUSER_3D_RENDERGRAPH_1594441548195_H