#ifndef SPEEDTOWER_H
#define SPEEDTOWER_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class SpeedTowerCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		SpeedTowerCommand(QObject* parent = nullptr);
		virtual ~SpeedTowerCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !MAXFLOWVOLUME_H
