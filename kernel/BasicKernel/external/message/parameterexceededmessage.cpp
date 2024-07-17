#include "external/message/parameterexceededmessage.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QVariant>

#include "external/interface/selectorinterface.h"
#include "external/interface/uiinterface.h"
#include "external/kernel/kernelui.h"

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
      , model_{ nullptr } {}

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

  auto ParameterExceededMessage::levelImpl() -> int {
    return MessageLevel::Error;
  }

  auto ParameterExceededMessage::messageImpl() -> QString {
    const auto exceeded_string =
      minimum_exceeded_ ? Translate("minimum") : Translate("maximum");

    if (model_) {
      return Translate("The %1's parameter %2 exceeds the %3 value.")
          .arg(model_->objectName(), getLabel(), exceeded_string);
    } else {
      return Translate("The parameter %1 exceeds the %2 value.").arg(getLabel(), exceeded_string);
    }
  }

  auto ParameterExceededMessage::linkStringImpl() -> QString {
    if (model_) {
      return Translate("Jump to %1's %2").arg(model_->objectName(), getLabel());
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
      selectOne(model_);

      // @see MainWindow2.qml
      // @param highlight (bool) need to highlight the *Model Setting Switch*
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

}  // namespace creative_kernel
