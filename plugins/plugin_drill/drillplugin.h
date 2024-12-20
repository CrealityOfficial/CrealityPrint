#pragma once

#include <QObject>
#include <QPointer>

#include "qtusercore/module/creativeinterface.h"
#include "data/interface.h"
#include "drillcommand.h"

class DrillPlugin : public QObject
                  , public CreativeInterface
                  , public creative_kernel::UIVisualTracer {
  Q_PLUGIN_METADATA(IID "creative.DrillPlugin");
  Q_INTERFACES(CreativeInterface);
  Q_OBJECT;

public:
  explicit DrillPlugin(QObject* parent = nullptr);
  virtual ~DrillPlugin();

protected:
  virtual QString name() override;
  virtual QString info() override;

  virtual void initialize() override;
  virtual void uninitialize() override;

protected:
  virtual void onThemeChanged(creative_kernel::ThemeCategory category) override;
  virtual void onLanguageChanged(creative_kernel::MultiLanguage language) override;

protected:
  QPointer<DrillCommand> command_;
};
