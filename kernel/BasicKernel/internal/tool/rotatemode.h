#ifndef _NULLSPACE_ROTATEMODE_1589765331924_H
#define _NULLSPACE_ROTATEMODE_1589765331924_H
#include "qtusercore/plugin/toolcommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class RotateOp;
	class RotateMode : public ToolCommand
		, public UIVisualTracer
	{
		Q_OBJECT
		Q_PROPERTY(QVector3D rotate READ rotate NOTIFY rotateChanged)
	public:
		RotateMode(QObject* parent = nullptr);
		virtual ~RotateMode();

		Q_INVOKABLE void execute();
		Q_INVOKABLE void reset();
		//Q_INVOKABLE void startRotate();
		Q_INVOKABLE bool checkSelect() override;
		
		QVector3D rotate();
		//void setRotate(QVector3D position);
		Q_INVOKABLE void setQmlRotate(float val, int nXYZFlag);

		Q_INVOKABLE void blockSignalScaleChanged(bool block);
	signals:
		void rotateChanged();

	private  slots:
		void slotMouseLeftClicked();

	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	protected:
		RotateOp* m_op;
	};
}
#endif // _NULLSPACE_ROTATEMODE_1589765331924_H
