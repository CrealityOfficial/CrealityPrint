#ifndef ARCFITTING_H
#define ARCFITTING_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class ArcFittingCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		ArcFittingCommand(QObject* parent = nullptr);
		virtual ~ArcFittingCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !MAXFLOWVOLUME_H
