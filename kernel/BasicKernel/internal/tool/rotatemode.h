#ifndef _NULLSPACE_ROTATEMODE_1589765331924_H
#define _NULLSPACE_ROTATEMODE_1589765331924_H
#include "qtusercore/plugin/toolcommand.h"
#include "data/interface.h"

class RotateOp;
namespace creative_kernel
{
	class RotateMode : public ToolCommand
		, public UIVisualTracer
	{
		Q_OBJECT
		Q_PROPERTY(QVector3D rotate READ rotate NOTIFY rotateChanged)
		Q_PROPERTY(bool message READ message WRITE setMessage NOTIFY supportMessage)
	public:
		RotateMode(QObject* parent = nullptr);
		virtual ~RotateMode();

		void setMessage(bool isRemove);
		bool message();

		Q_INVOKABLE void execute();
		Q_INVOKABLE void reset();
		Q_INVOKABLE void startRotate();
		Q_INVOKABLE bool checkSelect() override;
		Q_INVOKABLE void accept();
		Q_INVOKABLE void cancel();
		QVector3D rotate();
		void setRotate(QVector3D position);
		Q_INVOKABLE void setQmlRotate(float val, int nXYZFlag);

		Q_INVOKABLE void blockSignalScaleChanged(bool block);
	signals:
		void rotateChanged();
		void supportMessage();
	private  slots:
		void testMessage();
		void slotMouseLeftClicked();

	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	protected:
		RotateOp* m_op;
	};
}
#endif // _NULLSPACE_ROTATEMODE_1589765331924_H
