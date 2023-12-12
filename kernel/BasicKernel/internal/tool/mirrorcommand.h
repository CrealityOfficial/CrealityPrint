#pragma once

#include <memory>

#include "data/interface.h"
#include "external/operation/mirroroperatormode.h"
#include "qtusercore/plugin/toolcommand.h"

namespace creative_kernel {
  
class MirrorCommand : public ToolCommand
                    , public UIVisualTracer {
  Q_OBJECT;

public:
  explicit MirrorCommand(QObject* parent = nullptr);
  virtual ~MirrorCommand();

public:
  Q_INVOKABLE virtual void execute();

protected:
  virtual void onThemeChanged(ThemeCategory category) override;
  virtual void onLanguageChanged(MultiLanguage language) override;

private:
  std::unique_ptr<MirrorOperateMode> operate_mode_;
};

}  // namespace creative_kernel
