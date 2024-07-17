#pragma once

#include <memory>

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





  class ParameterMessageContext final : public QObject {
    Q_OBJECT;

    auto operator=(const ParameterMessageContext&) -> ParameterMessageContext = delete;
    auto operator=(ParameterMessageContext&&) -> ParameterMessageContext = delete;
    ParameterMessageContext(const ParameterMessageContext&) = delete;
    ParameterMessageContext(ParameterMessageContext&&) = delete;

   public:
    using Message = ParameterExceededMessage;

   public:
    static auto Instance() -> ParameterMessageContext&;

   public:
    auto emplaceMessage(QPointer<QObject> owner, std::unique_ptr<Message>&& message) -> void;
    auto eraseMessage(QPointer<QObject> owner) -> void;
    auto clearMessage() -> void;

   private:
    struct Internal;

   private:
    explicit ParameterMessageContext();
    ~ParameterMessageContext() final;

   private:
    auto initialize() -> void;
    auto uninitialize() -> void;
    auto isInitialized() const -> bool;

   private:
    auto showMessage(Message* message) -> void;
    auto hideMessage(Message* message) -> void;
    auto refreshMessage() -> void;

   private:
    bool initialized_{ false };
    std::unique_ptr<Internal> internal_{ nullptr };
  };

}  // namespace creative_kernel
