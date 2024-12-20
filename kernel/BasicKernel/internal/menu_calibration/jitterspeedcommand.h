#ifndef JITTERSPEED_H
#define JITTERSPEED_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class JitterSpeedCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		JitterSpeedCommand(QObject* parent = nullptr);
		virtual ~JitterSpeedCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !MAXFLOWVOLUME_H
