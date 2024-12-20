#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

#include "basickernelexport.h"
#include "data/interface.h"
#include "data/modelgroup.h"
#include "data/modeln.h"
#include "message/messageobject.h"

namespace creative_kernel {

  class ParameterExceededMessage;

  BASIC_KERNEL_API
  auto CreateParameterExceededMessage(const QString& key) -> QPointer<ParameterExceededMessage>;





  class BASIC_KERNEL_API ParameterExceededMessage : public MessageObject
                                                  , public SpaceTracer {
    Q_OBJECT;

   public:
    explicit ParameterExceededMessage(const QString& key);
    virtual ~ParameterExceededMessage() override;

   public:
    auto getKey() const -> QString;

    auto getLabel() const -> QString;

    auto isMinimumExceeded() const -> bool;
    auto setMinimumExceeded(bool minimum_exceeded) -> void;

    auto getOwner() const -> QPointer<QObject>;
    auto setOwner(QPointer<QObject> tree_node) -> void;

    auto getTreeNode() const -> QPointer<QObject>;
    auto setTreeNode(QPointer<QObject> tree_node) -> void;

    auto getModel() const -> QPointer<ModelN>;
    auto setModel(QPointer<ModelN> model) -> void;

    auto getModelGroup() const -> QPointer<ModelGroup>;
    auto setModelGroup(QPointer<ModelGroup> group) -> void;

   protected:
    auto levelImpl() -> int override;
    auto messageImpl() -> QString override;
    auto linkStringImpl() -> QString override;
    auto handleImpl() -> void override;

    auto onModelGroupRemoved(ModelGroup* group) -> void override;
    auto onModelGroupModified(ModelGroup*           group,
                              const QList<ModelN*>& removes,
                              const QList<ModelN*>& adds) -> void override;

   protected:
    QString key_{};
    bool minimum_exceeded_{ true };
    QPointer<QObject> owner_{ nullptr };
    QPointer<QObject> tree_node_{ nullptr };
    QPointer<ModelN> model_{ nullptr };
    QPointer<ModelGroup> group_{ nullptr };
    QPointer<QObject> model_space_{ nullptr };
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

  class BASIC_KERNEL_API InternalSolidInfillLineWidthExceededMessage : public ParameterExceededMessage {
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

  class BASIC_KERNEL_API EnablePrimeTowerExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    enum class State : int {
      UNVALID,
      ADAPTIVE_LAYER_HEIGHT_OPENED,
      DIFFERENT_LAYER_HEIGHT,
    };

   public:
    explicit EnablePrimeTowerExceededMessage(const QString& key);
    explicit EnablePrimeTowerExceededMessage(const QString& key, State state);
    explicit EnablePrimeTowerExceededMessage(
        const QString& key, State state, QPointer<ModelGroup> group, QPointer<QObject> tree_node);
    virtual ~EnablePrimeTowerExceededMessage() override = default;

   public:
    auto getState() const -> State;
    auto setState(State state) -> void;

   protected:
    auto codeImpl() -> int override;
    auto messageImpl() -> QString override;
    auto linkStringImpl() -> QString override;

   private:
    State state_{ State::UNVALID };
  };

  class BASIC_KERNEL_API TreeSupportTipDiameterExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~TreeSupportTipDiameterExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
    auto messageImpl() -> QString override;
  };

  class BASIC_KERNEL_API TreeSupportBranchDiameterExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    explicit TreeSupportBranchDiameterExceededMessage(const QString& key,
                                                      bool           small_than_tip_diameter,
                                                      bool           small_than_minimum_value);
    virtual ~TreeSupportBranchDiameterExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
    auto messageImpl() -> QString override;

   private:
    bool small_than_tip_diameter_{ false };
    bool small_than_minimum_value_{ false };
  };

  class BASIC_KERNEL_API IroningSpacingExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~IroningSpacingExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
  };

  class BASIC_KERNEL_API SupportStyleExceededMessage : public ParameterExceededMessage {
    Q_OBJECT;

   public:
    using ParameterExceededMessage::ParameterExceededMessage;
    virtual ~SupportStyleExceededMessage() override = default;

   protected:
    auto codeImpl() -> int override;
    auto messageImpl() -> QString override;
  };

}  // namespace creative_kernel
