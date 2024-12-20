#ifndef MAXACCELERATION_H
#define MAXACCELERATION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class MaxAccelerationCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		MaxAccelerationCommand(QObject* parent = nullptr);
		virtual ~MaxAccelerationCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !MAXFLOWVOLUME_H
