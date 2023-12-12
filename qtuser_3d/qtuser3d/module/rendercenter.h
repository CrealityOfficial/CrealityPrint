#ifndef CREATIVE_KERNEL_RENDERCENTER_1594435187692_H
#define CREATIVE_KERNEL_RENDERCENTER_1594435187692_H
#include "qtuser3d/qtuser3dexport.h"
#include <Qt3DCore/QNode>

//class GLQuickItem;
namespace qtuser_3d
{
	class EventSubdivide;
	class RenderGraph;
	class RawOGL;
	class QuickScene3DWrapper;

	class QTUSER_3D_API RenderCenter : public QObject
	{
		Q_OBJECT
	public:
		RenderCenter(QObject* parent = nullptr);
		virtual ~RenderCenter();

		//void setGLQuickItem(GLQuickItem* item);
		//GLQuickItem* glQuickItem();
		qtuser_3d::RawOGL* rawOGL();

		Q_INVOKABLE void exposureScene3D(QObject* scene3d);
		void setScene3DWrapper(qtuser_3d::QuickScene3DWrapper* item);
		qtuser_3d::QuickScene3DWrapper* scene3DWrapper();
		

		void uninitialize();

		qtuser_3d::EventSubdivide* eventSubdivide();

		void registerResidentNode(Qt3DCore::QNode* node);
		void unRegisterResidentNode(Qt3DCore::QNode* node);

		void renderRenderGraph(qtuser_3d::RenderGraph* graph);
		void registerRenderGraph(qtuser_3d::RenderGraph* graph);
		void unRegisterRenderGraph(qtuser_3d::RenderGraph* graph);
		void renderOneFrame();

		bool isRenderRenderGraph(qtuser_3d::RenderGraph* graph);
		qtuser_3d::RenderGraph* currentRenderGraph();
	protected:
		//GLQuickItem* m_glQuickItem;
		qtuser_3d::EventSubdivide* m_eventSubdivide;
		qtuser_3d::QuickScene3DWrapper* m_scene3DWrapper;
	};
}
#endif // CREATIVE_KERNEL_RENDERCENTER_1594435187692_H