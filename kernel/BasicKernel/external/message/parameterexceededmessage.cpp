#include "external/message/parameterexceededmessage.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QVariant>

#include <qtusercore/util/translateutil.h>

#include "external/kernel/kernel.h"
#include "external/data/modelspace.h"
#include "external/interface/selectorinterface.h"
#include "external/interface/uiinterface.h"

#include "internal/models/parameterdatatool.h"

namespace creative_kernel {

  auto CreateParameterExceededMessage(const QString& key) -> QPointer<ParameterExceededMessage> {
    if (key == QStringLiteral("layer_height")) {
      return new LayerHeightExceededMessage{ key };
    }

    if (key == QStringLiteral("initial_layer_print_height")) {
      return new InitialLayerHeightExceededMessage{ key };
    }

    if (key == QStringLiteral("line_width")) {
      return new LineWidthExceededMessage{ key };
    }

    if (key == QStringLiteral("initial_layer_line_width")) {
      return new InitialLineWidthExceededMessage{ key };
    }

    if (key == QStringLiteral("inner_wall_line_width")) {
      return new InnerWallLineWidthExceededMessage{ key };
    }

    if (key == QStringLiteral("outer_wall_line_width")) {
      return new OuterWallLineWidthExceededMessage{ key };
    }

    if (key == QStringLiteral("top_surface_line_width")) {
      return new TopSurfaceLineWidthExceededMessage{ key };
    }

    if (key == QStringLiteral("sparse_infill_line_width")) {
      return new SparseInfillLineWidthExceededMessage{ key };
    }

    if (key == QStringLiteral("internal_solid_infill_line_width")) {
      return new InternalSolidInfillLineWidthExceededMessage{ key };
    }

    if (key == QStringLiteral("support_line_width")) {
      return new SupportLineWidthExceededMessage{ key };
    }

    if (key == QStringLiteral("enable_prime_tower")) {
      return new EnablePrimeTowerExceededMessage{ key };
    }

    if (key == QStringLiteral("tree_support_tip_diameter")) {
      return new TreeSupportTipDiameterExceededMessage{ key };
    }

    if (key == QStringLiteral("ironing_spacing")) {
      return new IroningSpacingExceededMessage{ key };
    }

    if (key == QStringLiteral("support_style")) {
      return new SupportStyleExceededMessage{ key };
    }

    return nullptr;
  }

  auto Translate(const QString& source) -> QString {
    return QCoreApplication::translate("ParameterExceededMessage", source.toUtf8().constData());
  }





  ParameterExceededMessage::ParameterExceededMessage(const QString& key)
      : MessageObject{ nullptr }
      , key_{ key }
      , minimum_exceeded_{ false }
      , tree_node_{ nullptr }
      , model_{ nullptr } {
    auto* kernel = getKernel();
    if (!kernel) {
      return;
    }

    auto model_space = kernel->modelSpace();
    if (!model_space) {
      return;
    }

    model_space->addSpaceTracer(this);
    model_space_ = model_space;
  }

  ParameterExceededMessage::~ParameterExceededMessage() {
    if (model_space_) {
      static_cast<ModelSpace*>(model_space_.data())->removeSpaceTracer(this);
    }
  }

  auto ParameterExceededMessage::getKey() const -> QString {
    return key_;
  }

  auto ParameterExceededMessage::getLabel() const -> QString {
    if (tree_node_) {
      return tree_node_->property("translation").toString();
    }

    return getKey();
  }

  auto ParameterExceededMessage::isMinimumExceeded() const -> bool {
    return minimum_exceeded_;
  }

  auto ParameterExceededMessage::setMinimumExceeded(bool minimum_exceeded) -> void {
    minimum_exceeded_ = minimum_exceeded;
  }

  auto ParameterExceededMessage::getOwner() const -> QPointer<QObject> {
    return owner_;
  }

  auto ParameterExceededMessage::setOwner(QPointer<QObject> owner) -> void {
    owner_ = owner;
  }

  auto ParameterExceededMessage::getTreeNode() const -> QPointer<QObject> {
    return tree_node_;
  }

  auto ParameterExceededMessage::setTreeNode(QPointer<QObject> tree_node) -> void {
    tree_node_ = tree_node;
  }

  auto ParameterExceededMessage::getModel() const -> QPointer<ModelN> {
    return model_;
  }

  auto ParameterExceededMessage::setModel(QPointer<ModelN> model) -> void {
    model_ = model;
  }

  auto ParameterExceededMessage::getModelGroup() const -> QPointer<ModelGroup> {
    return group_;
  }

  auto ParameterExceededMessage::setModelGroup(QPointer<ModelGroup> group) -> void {
    group_ = group;
  }

  auto ParameterExceededMessage::levelImpl() -> int {
    return MessageLevel::Error;
  }

  auto ParameterExceededMessage::messageImpl() -> QString {
    const auto exceeded_string =
      minimum_exceeded_ ? Translate("minimum") : Translate("maximum");

    if (model_) {
      return Translate("The %1's parameter %2 exceeds the %3 value.")
          .arg(model_->name(), getLabel(), exceeded_string);
    } else if (group_) {
      return Translate("The %1's parameter %2 exceeds the %3 value.")
          .arg(group_->groupName(), getLabel(), exceeded_string);
    } else {
      return Translate("The parameter %1 exceeds the %2 value.").arg(getLabel(), exceeded_string);
    }
  }

  auto ParameterExceededMessage::linkStringImpl() -> QString {
    if (model_) {
      return Translate("Jump to %1's %2").arg(model_->name(), getLabel());
    } else if (group_) {
      return Translate("Jump to %1's %2").arg(group_->groupName(), getLabel());
    } else {
      return Translate("Jump to %1").arg(getLabel());
    }
  }

  auto ParameterExceededMessage::handleImpl() -> void {
    auto* window = getUIAppWindow();
    if (!window) {
      return;
    }

    if (model_) {
      selectModelN(model_);
      QMetaObject::invokeMethod(window,
                                "requestModelSettingPanel",
                                Q_ARG(QVariant, QVariant::fromValue<bool>(false)));
    } else if (group_) {
      selectGroup(group_);
      QMetaObject::invokeMethod(window,
                                "requestModelSettingPanel",
                                Q_ARG(QVariant, QVariant::fromValue<bool>(false)));
    } else {
      QMetaObject::invokeMethod(window, "requestProcessSettingPanel");
    }

    if (tree_node_) {
      QMetaObject::invokeMethod(window,
                                "highlightProcessTreeNode",
                                Q_ARG(QVariant, QVariant::fromValue<QObject*>(tree_node_)));
    }
  }

  auto ParameterExceededMessage::onModelGroupRemoved(ModelGroup* group) -> void {
    if (group_ == group && owner_) {
      EraseParameterMessage(owner_);
    }
  }

  auto ParameterExceededMessage::onModelGroupModified(ModelGroup*           group,
                                                      const QList<ModelN*>& removes,
                                                      const QList<ModelN*>& adds) -> void {
    if (owner_ && model_ && removes.contains(model_)) {
      EraseParameterMessage(owner_);
    }
  }

  auto LayerHeightExceededMessage::codeImpl() -> int {
    return MessageType::LayerHeightExceeded;
  }

  auto InitialLayerHeightExceededMessage::codeImpl() -> int {
    return MessageType::InitialLayerHeightExceeded;
  }

  auto LineWidthExceededMessage::codeImpl() -> int {
    return MessageType::LineWidthExceeded;
  }

  auto InitialLineWidthExceededMessage::codeImpl() -> int {
    return MessageType::InitialLineWidthExceeded;
  }

  auto InnerWallLineWidthExceededMessage::codeImpl() -> int {
    return MessageType::InnerWallLineWidthExceeded;
  }

  auto OuterWallLineWidthExceededMessage::codeImpl() -> int {
    return MessageType::OuterWallLineWidthExceeded;
  }

  auto TopSurfaceLineWidthExceededMessage::codeImpl() -> int {
    return MessageType::TopSurfaceLineWidthExceeded;
  }

  auto SparseInfillLineWidthExceededMessage::codeImpl() -> int {
    return MessageType::SparseInfillLineWidthExceeded;
  }

  auto InternalSolidInfillLineWidthExceededMessage::codeImpl() -> int {
    return MessageType::InternalSolidInfillLineWidthExceeded;
  }

  auto SupportLineWidthExceededMessage::codeImpl() -> int {
    return MessageType::SupportLineWidthExceeded;
  }

  EnablePrimeTowerExceededMessage::EnablePrimeTowerExceededMessage(const QString& key)
    : EnablePrimeTowerExceededMessage{ key, State::UNVALID, nullptr, nullptr } {}

  EnablePrimeTowerExceededMessage::EnablePrimeTowerExceededMessage(const QString& key, State state)
    : EnablePrimeTowerExceededMessage{ key, state, nullptr, nullptr } {}

  EnablePrimeTowerExceededMessage::EnablePrimeTowerExceededMessage(
      const QString& key, State state, QPointer<ModelGroup> group, QPointer<QObject> tree_node)
      : ParameterExceededMessage{ key }, state_{ state } {
    setModelGroup(group);
    setTreeNode(tree_node);
  }

  auto EnablePrimeTowerExceededMessage::getState() const -> State {
    return state_;
  }

  auto EnablePrimeTowerExceededMessage::setState(State state) -> void {
    state_ = state;
  }

  auto EnablePrimeTowerExceededMessage::codeImpl() -> int {
    return MessageType::EnablePrimeTowerExceeded;
  }

  auto EnablePrimeTowerExceededMessage::messageImpl() -> QString {
    if (state_ == State::ADAPTIVE_LAYER_HEIGHT_OPENED) {
      return Translate(QStringLiteral(
          "The layer height of each object is different, so the prime tower cannot be enabled."));
    }

    if (state_ == State::DIFFERENT_LAYER_HEIGHT) {
      return Translate(
          QStringLiteral("The prime tower requires that all objects have the same layer height."));
    }

    return {};
  }

  auto EnablePrimeTowerExceededMessage::linkStringImpl() -> QString {
    if (state_ == State::DIFFERENT_LAYER_HEIGHT && group_) {
      return Translate(QStringLiteral("Jump to %1's %2")).arg(group_->groupName(), getLabel());
    }

    return {};
  }

  auto TreeSupportTipDiameterExceededMessage::codeImpl() -> int {
    return MessageType::TreeSupportTipDiameterExceeded;
  }

  auto TreeSupportTipDiameterExceededMessage::messageImpl() -> QString {
    if (!isMinimumExceeded()) {
      return ParameterExceededMessage::messageImpl();
    }

    return Translate(
        QStringLiteral("%1 must not be smaller than support material extrusion width."))
            .arg(getLabel());
  }





  TreeSupportBranchDiameterExceededMessage::TreeSupportBranchDiameterExceededMessage(
      const QString& key, bool small_than_tip_diameter, bool small_than_minimum_value)
      : ParameterExceededMessage{ key }
      , small_than_tip_diameter_{ small_than_tip_diameter }
      , small_than_minimum_value_{ small_than_minimum_value } {}

  auto TreeSupportBranchDiameterExceededMessage::codeImpl() -> int {
    return MessageType::TreeSupportBranchDiameterExceeded;
  }

  auto TreeSupportBranchDiameterExceededMessage::messageImpl() -> QString {
    if (small_than_tip_diameter_ && !small_than_minimum_value_) {
      return Translate(QStringLiteral("%1 must not be smaller than support tree tip diameter."))
          .arg(getLabel());
    }

    return ParameterExceededMessage::messageImpl();
  }

  auto IroningSpacingExceededMessage::codeImpl() -> int {
    return MessageType::IroningSpacingExceeded;
  }





  auto SupportStyleExceededMessage::codeImpl() -> int {
    return MessageType::SupportStyleExceeded;
  }

  auto SupportStyleExceededMessage::messageImpl() -> QString {
    using namespace cx::literals;
    return u8"adaptive_layer_height_and_organic_tree_are_mutually_exclusive"_tr;
  }

}  // namespace creative_kernel
