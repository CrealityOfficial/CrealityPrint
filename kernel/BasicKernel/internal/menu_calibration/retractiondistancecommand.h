#ifndef RETRACTIONDISTANCE_H
#define RETRACTIONDISTANCE_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class RetractionDistanceCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		RetractionDistanceCommand(QObject* parent = nullptr);
		virtual ~RetractionDistanceCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !MAXFLOWVOLUME_H
