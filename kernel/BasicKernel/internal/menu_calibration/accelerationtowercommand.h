#ifndef ACCELERATIONTOWER_H
#define ACCELERATIONTOWER_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class AccelerationTowerCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		AccelerationTowerCommand(QObject* parent = nullptr);
		virtual ~AccelerationTowerCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !MAXFLOWVOLUME_H
