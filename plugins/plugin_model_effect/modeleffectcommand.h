#ifndef _NULLSPACE_ModelEffectCommand_202312111727_H
#define _NULLSPACE_ModelEffectCommand_202312111727_H
#include "qtusercore/plugin/toolcommand.h"
#include "data/interface.h"

class ModelEffectOp;
class ModelEffectCommand: public ToolCommand
	, public creative_kernel::UIVisualTracer
{
	Q_OBJECT
public:
	ModelEffectCommand(QObject* parent = nullptr);
	virtual ~ModelEffectCommand();

	Q_INVOKABLE void execute();
    Q_INVOKABLE bool checkSelect() override;

	Q_INVOKABLE void accept();
	Q_INVOKABLE void cancel();

	void setMessage(bool isRemove);
	bool message();

protected:
	void onThemeChanged(creative_kernel::ThemeCategory category) override {};
	void onLanguageChanged(creative_kernel::MultiLanguage language) override {};
protected:
	ModelEffectOp* m_op;
};
#endif // _NULLSPACE_ModelEffectCommand_202312111727_H
