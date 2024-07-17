#pragma once

#include <memory>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "basickernelexport.h"
#include "data/modeln.h"
#include "message/messageobject.h"

namespace creative_kernel {

  class ParameterExceededMessage;

  BASIC_KERNEL_API
  auto CreateParameterExceededMessage(const QString& key) -> QPointer<ParameterExceededMessage>;





  class BASIC_KERNEL_API ParameterExceededMessage : public MessageObject {
    Q_OBJECT;

   public:
    explicit ParameterExceededMessage(const QString& key);
    virtual ~ParameterExceededMessage() override = default;

   public:
    auto getKey() const -> QString;

    auto getLabel() const -> QString;

    auto isMinimumExceeded() const -> bool;
    auto setMinimumExceeded(bool minimum_exceeded) -> void;

    auto getTreeNode() const -> QPointer<QObject>;
    auto setTreeNode(QPointer<QObject> tree_node) -> void;

    auto getModel() const -> QPointer<ModelN>;
    auto setModel(QPointer<ModelN> model) -> void;

   protected:
    auto levelImpl() -> int override;
    auto messageImpl() -> QString override;
    auto linkStringImpl() -> QString override;
    auto handleImpl() -> void override;

   protected:
    QString key_{};
    bool minimum_exceeded_{ true };
    QPointer<QObject> tree_node_{ nullptr };
    QPointer<ModelN> model_{ nullptr };
  };

  class BASIC_KERNEL_API LayerHeightExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~LayerHeightExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
  };

  class BASIC_KERNEL_API InitialLayerHeightExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~InitialLayerHeightExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
  };

  class BASIC_KERNEL_API LineWidthExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~LineWidthExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
  };

  class BASIC_KERNEL_API InitialLineWidthExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~InitialLineWidthExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
  };

  class BASIC_KERNEL_API InnerWallLineWidthExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~InnerWallLineWidthExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
  };

  class BASIC_KERNEL_API OuterWallLineWidthExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~OuterWallLineWidthExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
  };

  class BASIC_KERNEL_API TopSurfaceLineWidthExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~TopSurfaceLineWidthExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
  };

  class BASIC_KERNEL_API SparseInfillLineWidthExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~SparseInfillLineWidthExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
  };

  class BASIC_KERNEL_API InternalSolidInfillLineWidthExceededMessage
      : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~InternalSolidInfillLineWidthExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
  };

  class BASIC_KERNEL_API SupportLineWidthExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~SupportLineWidthExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
  };

}  // namespace creative_kernel
