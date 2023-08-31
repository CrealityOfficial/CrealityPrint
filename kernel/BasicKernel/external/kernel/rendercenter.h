#ifndef CREATIVE_KERNEL_RENDERCENTER_1594435187692_H
#define CREATIVE_KERNEL_RENDERCENTER_1594435187692_H
#include "basickernelexport.h"
#include "qtusercore/module/singleton.h"
#include <Qt3DCore/QNode>

class GLQuickItem;
namespace qtuser_3d
{
	class EventSubdivide;
	class RenderGraph;
}

namespace qtuser_qml
{
	class RawOGL;
}

namespace creative_kernel
{
	class BASIC_KERNEL_API RenderCenter : public QObject
	{
		Q_OBJECT
	public:
		RenderCenter(QObject* parent = nullptr);
		virtual ~RenderCenter();

		void setGLQuickItem(GLQuickItem* item);
		GLQuickItem* glQuickItem();
		qtuser_qml::RawOGL* rawOGL();

		void uninitialize();

		qtuser_3d::EventSubdivide* eventSubdivide();

		void registerResidentNode(Qt3DCore::QNode* node);
		void unRegisterResidentNode(Qt3DCore::QNode* node);
		void renderRenderGraph(qtuser_3d::RenderGraph* graph);
		void registerRenderGraph(qtuser_3d::RenderGraph* graph);
		void unRegisterRenderGraph(qtuser_3d::RenderGraph* graph);
		void renderOneFrame();

		bool isRenderRenderGraph(qtuser_3d::RenderGraph* graph);
	protected:
		GLQuickItem* m_glQuickItem;
		qtuser_3d::EventSubdivide* m_eventSubdivide;
	};
}
#endif // CREATIVE_KERNEL_RENDERCENTER_1594435187692_H