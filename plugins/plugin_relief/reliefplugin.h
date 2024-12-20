#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include <qtusercore/module/creativeinterface.h>

#include "reliefcommand.h"

class ReliefPlugin : public QObject, public CreativeInterface {
  Q_PLUGIN_METADATA(IID "creative.ReliefPlugin");
  Q_INTERFACES(CreativeInterface);
  Q_OBJECT;

 public:
  explicit ReliefPlugin(QObject* parent = nullptr);
  virtual ~ReliefPlugin() override;

 protected:
  auto name() -> QString override;
  auto info() -> QString override;

  auto initialize() -> void override;
  auto uninitialize() -> void override;

 private:
  QPointer<ReliefCommand> command_{ nullptr };
};
