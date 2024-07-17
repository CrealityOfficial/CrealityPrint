#include "external/message/parameterupdateablemessage.h"

#include <QtCore/QCoreApplication>

namespace creative_kernel {

  ParameterUpdateableMessage::ParameterUpdateableMessage() : MessageObject{ nullptr } {}

  ParameterUpdateableMessage::ParameterUpdateableMessage(std::function<void(void)> callback)
      : MessageObject{ nullptr }
      , callback_{ std::move(callback) } {}

  auto ParameterUpdateableMessage::getCallback() const -> std::function<void(void)> {
    return callback_;
  }

  auto ParameterUpdateableMessage::setCallback(std::function<void(void)> callback) -> void {
    callback_ = callback;
  }

  auto ParameterUpdateableMessage::codeImpl() -> int {
    return MessageType::ParameterUpdateable;
  }

  auto ParameterUpdateableMessage::levelImpl() -> int {
    return MessageLevel::Tip;
  }

  auto ParameterUpdateableMessage::messageImpl() -> QString {
    return QCoreApplication::translate(
      "parameter_update",
      "A new parameter package update is detected. Would you like to update it?");
  }

  auto ParameterUpdateableMessage::linkStringImpl() -> QString {
    return QCoreApplication::translate("parameter_update", "Update Now");
  }

  auto ParameterUpdateableMessage::handleImpl() -> void {
    if (callback_) {
      callback_();
    }
  }

}  // namespace creative_kernel
