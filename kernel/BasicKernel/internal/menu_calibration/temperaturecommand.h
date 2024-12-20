#ifndef TEMPERATURECOMMAND_H
#define TEMPERATURECOMMAND_H
#include "menu/actioncommand.h"
#include "data/interface.h"
//#include "calibratescenecreator.h"

namespace creative_kernel
{
	class TemperatureCommand : public ActionCommand
		, public UIVisualTracer
	{
		Q_OBJECT
	public:
		TemperatureCommand(QObject* parent = nullptr);
		virtual ~TemperatureCommand();

		Q_INVOKABLE void execute();
		Q_INVOKABLE void generate(int _statrt,int _end,int _step);
	protected:
		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;

	private:
		//CalibrateSceneCreator m_creator;
	};
}
#endif // !TEMPERATURECOMMAND_H
