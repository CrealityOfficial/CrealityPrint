#ifndef _NULLSPACE_PICKBOTTOMCOMMAND_1589850876397_H
#define _NULLSPACE_PICKBOTTOMCOMMAND_1589850876397_H
#include "qtusercore/plugin/toolcommand.h"
#include "data/interface.h"

class PickBottomOp;
class PickBottomCommand: public ToolCommand
	, public creative_kernel::UIVisualTracer
{
	Q_OBJECT
public:
	PickBottomCommand(QObject* parent = nullptr);
	virtual ~PickBottomCommand();

	Q_INVOKABLE void execute();
    Q_INVOKABLE bool checkSelect() override;

	Q_INVOKABLE void accept();
	Q_INVOKABLE void cancel();

	Q_INVOKABLE void autoPickBottom();

	void setMessage(bool isRemove);
	bool message();

private  slots:
	void slotMouseLeftClicked();
protected:
	void onThemeChanged(creative_kernel::ThemeCategory category) override;
	void onLanguageChanged(creative_kernel::MultiLanguage language) override;
protected:
	PickBottomOp* m_op;
};
#endif // _NULLSPACE_PICKBOTTOMCOMMAND_1589850876397_H
