#ifndef FANSPEED_H
#define FANSPEED_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class FanSpeedCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		FanSpeedCommand(QObject* parent = nullptr);
		virtual ~FanSpeedCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !MAXFLOWVOLUME_H
