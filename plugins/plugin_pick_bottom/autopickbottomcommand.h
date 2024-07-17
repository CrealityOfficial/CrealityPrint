#ifndef _NULLSPACE_AUTOPICKBOTTOMCOMMAND_1589850876397_H
#define _NULLSPACE_AUTOPICKBOTTOMCOMMAND_1589850876397_H
#include "qtusercore/plugin/toolcommand.h"
#include "data/interface.h"

class PickBottomOp;
class AutoPickBottomCommand: public ToolCommand
	, public creative_kernel::UIVisualTracer
{
	Q_OBJECT
public:
	AutoPickBottomCommand(QObject* parent = nullptr);
	virtual ~AutoPickBottomCommand();

	Q_INVOKABLE void execute();
    Q_INVOKABLE bool checkSelect() override;

	Q_INVOKABLE void accept();
	Q_INVOKABLE void cancel();

	Q_INVOKABLE void maxFaceBottom();
	Q_INVOKABLE bool isCFunction();

protected:
	void onThemeChanged(creative_kernel::ThemeCategory category) override;
	void onLanguageChanged(creative_kernel::MultiLanguage language) override;
protected:
	PickBottomOp* m_op;
};
#endif // _NULLSPACE_AUTOPICKBOTTOMCOMMAND_1589850876397_H
