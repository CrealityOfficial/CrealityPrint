#ifndef SLOWACCELERATION_H
#define SLOWACCELERATION_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class SlowAccelerationCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		SlowAccelerationCommand(QObject* parent = nullptr);
		virtual ~SlowAccelerationCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !MAXFLOWVOLUME_H
