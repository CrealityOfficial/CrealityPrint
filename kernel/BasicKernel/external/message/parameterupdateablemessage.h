#pragma once

#include <functional>

#include "basickernelexport.h"
#include "message/messageobject.h"

namespace creative_kernel {

  class BASIC_KERNEL_API ParameterUpdateableMessage : public MessageObject {
    Q_OBJECT;

   public:
    explicit ParameterUpdateableMessage();
    explicit ParameterUpdateableMessage(std::function<void(void)> callback);
    virtual ~ParameterUpdateableMessage() override = default;

   public:
    auto getCallback() const -> std::function<void(void)>;
    auto setCallback(std::function<void(void)> callback) -> void;

   protected:
    auto codeImpl() -> int final;
    auto levelImpl() -> int final;
    auto messageImpl() -> QString override;
    auto linkStringImpl() -> QString override;
    auto handleImpl() -> void override;

   private:
    std::function<void(void)> callback_{ nullptr };
  };

}
