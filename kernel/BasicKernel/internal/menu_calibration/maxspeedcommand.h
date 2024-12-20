#ifndef MAXSPEED_H
#define MAXSPEED_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class MaxSpeedCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		MaxSpeedCommand(QObject* parent = nullptr);
		virtual ~MaxSpeedCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !MAXFLOWVOLUME_H
