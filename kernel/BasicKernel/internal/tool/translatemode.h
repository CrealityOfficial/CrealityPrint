#ifndef _NULLSPACE_TRANSLATEMODE_1589765331924_H
#define _NULLSPACE_TRANSLATEMODE_1589765331924_H
#include "qtusercore/plugin/toolcommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class TranslateOp;
	class TranslateMode : public ToolCommand
		, public UIVisualTracer
	{
		Q_OBJECT
		Q_PROPERTY(QVector3D position READ position NOTIFY positionChanged)
		//Q_PROPERTY(bool message READ message WRITE setMessage NOTIFY supportMessage)
	public:
		TranslateMode(QObject* parent = nullptr);
		virtual ~TranslateMode();

		/*void setMessage(bool isRemove);
		bool message();
		Q_INVOKABLE void accept();
		Q_INVOKABLE void cancel();*/

		Q_INVOKABLE void execute();
		Q_INVOKABLE void reset();
		Q_INVOKABLE void center();
		Q_INVOKABLE void bottom();
		Q_INVOKABLE bool checkSelect() override;

		QVector3D position();
		float positionX();
		float positionY();
		float positionZ();
		Q_INVOKABLE void setQmlPosition(float val, int nXYZFlag);
		void setPosition(QVector3D position);
		bool selected();

		Q_INVOKABLE void blockSignalMoveChanged(bool block);
	signals:
		void positionChanged();
		//void supportMessage();

	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	protected:
		TranslateOp* m_translateOp;
	};
}
#endif // _NULLSPACE_TRANSLATEMODE_1589765331924_H
