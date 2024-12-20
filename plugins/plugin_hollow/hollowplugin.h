#ifndef HOLLOWPLUGIN_H_
#define HOLLOWPLUGIN_H_

#include <QObject>
#include <QPointer>

#include "qtusercore/module/creativeinterface.h"
#include "data/interface.h"
#include "hollowcommand.h"

class HollowPlugin : public QObject
                   , public CreativeInterface
                   , public creative_kernel::UIVisualTracer {
  Q_PLUGIN_METADATA(IID "creative.HollowPlugin");
  Q_INTERFACES(CreativeInterface);
  Q_OBJECT;

public:
  explicit HollowPlugin(QObject* parent = nullptr);
  virtual ~HollowPlugin();

protected:
  virtual QString name() override;
  virtual QString info() override;

  virtual void initialize() override;
  virtual void uninitialize() override;

protected:
  virtual void onThemeChanged(creative_kernel::ThemeCategory category) override;
  virtual void onLanguageChanged(creative_kernel::MultiLanguage language) override;

protected:
  QPointer<HollowCommand> command_;
};

#endif // HOLLOWPLUGIN_H_
