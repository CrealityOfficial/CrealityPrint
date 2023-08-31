#ifndef CREATIVE_KERNEL_VISSCENEHOVERHANDLER_1594483094926_H
#define CREATIVE_KERNEL_VISSCENEHOVERHANDLER_1594483094926_H
#include "basickernelexport.h"
#include "qtuser3d/event/eventhandlers.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API VisSceneHandler : public QObject
		, public qtuser_3d::HoverEventHandler
		, public qtuser_3d::KeyEventHandler
		, public qtuser_3d::LeftMouseEventHandler
	{
		Q_OBJECT
	public:
		VisSceneHandler(QObject* parent = nullptr);
		virtual ~VisSceneHandler();

	protected:
		void onHoverEnter(QHoverEvent* event) override;
		void onHoverLeave(QHoverEvent* event) override;
		void onHoverMove(QHoverEvent* event) override;

		void onKeyPress(QKeyEvent* event) override;
		void onKeyRelease(QKeyEvent* event) override;

		void onLeftMouseButtonPress(QMouseEvent* event) override;
		void onLeftMouseButtonRelease(QMouseEvent* event) override;
		void onLeftMouseButtonMove(QMouseEvent* event) override;
		void onLeftMouseButtonClick(QMouseEvent* event) override;

	private:
		QPoint m_posOfLeftMousePress;
		bool m_didSelectModelAtPress;

	};
}
#endif // CREATIVE_KERNEL_VISSCENEHOVERHANDLER_1594483094926_H