#ifndef CREATIVE_KERNEL_VISSCENEHOVERHANDLER_1594483094926_H
#define CREATIVE_KERNEL_VISSCENEHOVERHANDLER_1594483094926_H
#include "basickernelexport.h"
#include "qtuser3d/event/eventhandlers.h"
#include "external/interface/selectorinterface.h"
#include "qtuser3d/module/pickableselecttracer.h"
#include <QPixmap>
#include <QCursor>
#include "external/data/interface.h"
#include <QTimer>

namespace qtuser_3d 
{
	class AlonePointEntity;
};

namespace creative_kernel
{
	class BASIC_KERNEL_API VisSceneHandler : public QObject
		, public qtuser_3d::HoverEventHandler
		, public qtuser_3d::KeyEventHandler
		, public qtuser_3d::LeftMouseEventHandler
		, public qtuser_3d::RightMouseEventHandler
		// , public qtuser_3d::SelectorTracer
    	, public UIVisualTracer
	{
		Q_OBJECT
	public:
		VisSceneHandler(QObject* parent = nullptr);
		virtual ~VisSceneHandler();

		void setEnabled(bool enabled);

	private:
		void updateCameraCenter();

		void cancelRectangleSelect();

		void notifyPositionChanged();
		void notifyRotationChanged();

	private slots:
		void activeCameraCenterChange();
		void onPhaseChanged();

	protected:
		void onThemeChanged(ThemeCategory category);
		void onLanguageChanged(MultiLanguage language);

		// void onSelectionsChanged();
		// void selectChanged(qtuser_3d::Pickable* pickable);

		void onHoverEnter(QHoverEvent* event) override;
		void onHoverLeave(QHoverEvent* event) override;
		void onHoverMove(QHoverEvent* event) override;

		void onKeyPress(QKeyEvent* event) override;
		void onKeyRelease(QKeyEvent* event) override;

		void onLeftMouseButtonPress(QMouseEvent* event) override;
		void onLeftMouseButtonRelease(QMouseEvent* event) override;
		void onLeftMouseButtonMove(QMouseEvent* event) override;
		void onLeftMouseButtonClick(QMouseEvent* event) override;

		void onRightMouseButtonPress(QMouseEvent* event) override;
		void onRightMouseButtonRelease(QMouseEvent* event) override;
		void onRightMouseButtonMove(QMouseEvent* event) override;
		void onRightMouseButtonClick(QMouseEvent* event) override;

				// QApplication::setOverrideCursor(QCursor(m_rotatePixmap));
				// QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
	private:
		QPoint m_posOfLeftMousePress;
		bool m_didSelectModelAtPress;

		bool m_rotating { false };
		bool m_selecting{ false };
		QPixmap m_selectPixmap;
		QPixmap m_movePixmap;
		QPixmap m_rotatePixmap;

		QTimer m_updateCameraCenterTimer;

		qtuser_3d::Pickable* m_selectedPickable {NULL};
		qtuser_3d::AlonePointEntity* m_rotateCenterEntity;

	};
}
#endif // CREATIVE_KERNEL_VISSCENEHOVERHANDLER_1594483094926_H