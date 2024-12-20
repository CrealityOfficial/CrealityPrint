#ifndef VFACOMMAND_H
#define VFACOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"

namespace creative_kernel
{
	class VFACommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		VFACommand(QObject* parent = nullptr);
		virtual ~VFACommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE QString getWebsite();
		Q_INVOKABLE QString getCurrentVersion();
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}
#endif // !VFACOMMAND_H
