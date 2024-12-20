#ifndef RETRACTIONCOMMAND_H
#define RETRACTIONCOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class RetractionCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		RetractionCommand(QObject* parent = nullptr);
		virtual ~RetractionCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !PACOMMAND_H
