#pragma once

#include <memory>
#include <set>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtQml/QJSValue>

#include "external/message/parameterexceededmessage.h"
#include "external/us/usettings.h"

namespace creative_kernel {

  class ParameterDataModel;

  class ParameterMessageContext;





  auto CheckBooleanType(const QString& type, const QString& value) -> bool;

  auto CheckEnumType(const QString& type, const QString& value) -> bool;

  auto CheckIntegerType(const QString& type, const QString& value) -> bool;

  auto CheckFloatType(const QString& type, const QString& value) -> bool;

  auto CheckPercentType(const QString& type, const QString& value) -> bool;

  auto CalculateFuzzyValue(const QString& type,
                           const QString& key,
                           const QString& value) -> QString;

  auto CheckJsScriptValid(const QString& script) -> bool;

  auto InvokeJsScript(const QString& script, ParameterDataModel* context = nullptr) -> QJSValue;

  auto CheckEmptySettings(us::USettings* settings) -> bool;

  auto CheckEmptySettings(us::USettings* settings, const std::set<QString>& ignore_keys) -> bool;





  class ParameterMessageContext : public QObject {
    Q_OBJECT;

    auto operator=(const ParameterMessageContext&) -> ParameterMessageContext = delete;
    auto operator=(ParameterMessageContext&&) -> ParameterMessageContext = delete;
    ParameterMessageContext(const ParameterMessageContext&) = delete;
    ParameterMessageContext(ParameterMessageContext&&) = delete;

   public:
    using Message = ParameterExceededMessage;

   public:
    explicit ParameterMessageContext();
    virtual ~ParameterMessageContext() override;

   public:
    auto initialize() -> void;
    auto uninitialize() -> void;
    auto isInitialized() const -> bool;

   public:
    auto emplaceMessage(QObject* owner, std::unique_ptr<Message>&& message) -> void;
    auto eraseMessage(QObject* owner) -> void;

   private:
    struct Internal;

   private:
    auto showMessage(Message* message) -> void;
    auto hideMessage(Message* message) -> void;
    auto refreshMessage() -> void;

    auto onOwnerDestroyed() -> void;

   private:
    bool initialized_{ false };
    std::unique_ptr<Internal> internal_{ nullptr };
  };

  auto EmplaceParameterMessage(QObject*                                            owner,
                               std::unique_ptr<ParameterMessageContext::Message>&& message) -> void;
  auto EraseParameterMessage(QObject* owner) -> void;

}  // namespace creative_kernel
