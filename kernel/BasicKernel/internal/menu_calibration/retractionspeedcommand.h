#ifndef RETRACTIONSPEED_H
#define RETRACTIONSPEED_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class RetractionSpeedCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		RetractionSpeedCommand(QObject* parent = nullptr);
		virtual ~RetractionSpeedCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !MAXFLOWVOLUME_H
