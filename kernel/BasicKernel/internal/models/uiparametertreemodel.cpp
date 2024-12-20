#include "internal/models/uiparametertreemodel.h"

#include <fstream>

#include <QtCore/QCoreApplication>
#include <QtQml/QQmlEngine>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "internal/parameter/uiparametermanager.h"

#include "external/kernel/kernel.h"
#include "external/kernel/globalconst.h"

namespace creative_kernel {

  namespace {
    template<typename... _Args>
    auto CreateNode(_Args&&... args) -> std::shared_ptr<UiParameterTreeModel> {
      struct make_shared_helper_t : public UiParameterTreeModel {
        make_shared_helper_t(_Args&&... args)
            : UiParameterTreeModel{ std::forward<_Args>(args)... } {
          QQmlEngine::setObjectOwnership(this, QQmlEngine::ObjectOwnership::CppOwnership);
        }
      };

      return std::make_shared<make_shared_helper_t>(std::forward<_Args>(args)...);
    }
  }  // namespace

  auto UiParameterTreeModel::CreateRoot() -> std::shared_ptr<UiParameterTreeModel> {
    return CreateNode();
  }

  auto UiParameterTreeModel::createChild() -> std::shared_ptr<UiParameterTreeModel> {
    auto parent = sharedFromThis();
    auto child = CreateNode(parent);
    parent->emplaceBackChildNode(child);
    parent->countChanged();
    return child;
  }

  auto UiParameterTreeModel::eraseChild(const QString& uid) -> std::shared_ptr<UiParameterTreeModel> {
    auto parent = sharedFromThis();
    auto child = parent->eraseChildNode(uid);
    if (child) {
      parent->countChanged();
    }
    return std::static_pointer_cast<UiParameterTreeModel>(child);
  }

  UiParameterTreeModel::UiParameterTreeModel()
      : UiParameterTreeModel{ nullptr } {}

  UiParameterTreeModel::UiParameterTreeModel(std::shared_ptr<UiParameterTreeModel> parent)
      : qtuser_qml::FlattenTreeModel{ parent } {
    using super_t = qtuser_qml::CustomTreeModel;
    using self_t = UiParameterTreeModel;

    if (parent) {
      connect(parent.get(), &super_t::uidChanged, this, &self_t::updateUid);
      connect(parent.get(), &self_t::typeChanged, this, &self_t::typeChanged);

      connect(this, &self_t::levelChanged, parent.get(), &self_t::levelChanged);
      connect(this, &self_t::levelCustomizedChanged, parent.get(), &self_t::levelCustomizedChanged);

      connect(this, &self_t::modifyedChanged, parent.get(), &self_t::modifyedChanged);
      connect(this, &self_t::translationChanged, parent.get(), &self_t::modifyedTipChanged);
      connect(this, &self_t::modifyedTipChanged, parent.get(), &self_t::modifyedTipChanged);
    }

    connect(this, &self_t::keyChanged, this, &self_t::updateUid);
    connect(this, &self_t::labelChanged, this, &self_t::updateUid);
    connect(this, &super_t::uidChanged, this, &self_t::updateIcon);
    connect(this, &self_t::typeChanged, this, &self_t::updateIcon);
    connect(this, &self_t::modifyedChanged, this, &self_t::modifyedTipChanged);

    updateUid();
    updateIcon();
  }





  auto UiParameterTreeModel::getUid() const -> QString {
    if (isRootNode()) {
      return {};
    } else {
      return qtuser_qml::CustomTreeModel::getUid();
    }
  }

  auto UiParameterTreeModel::getType() const -> Type {
    if (isRootNode()) {
      return type_;
    }

    return parentNode()->getType();
  }

  auto UiParameterTreeModel::getTypeString() const -> QString {
    switch (getType()) {
      case Type::MATERIAL: {
        return QStringLiteral("material");
      }
      case Type::PROCESS: {
        return QStringLiteral("process");
      }
      case Type::MACHINE:
      default: {
        return QStringLiteral("machine");
      }
    }
  }

  auto UiParameterTreeModel::setType(Type type) -> void {
    if (isRootNode() && type_ != type) {
      type_ = type;
      typeChanged();
    }
  }

  auto UiParameterTreeModel::getName() const -> QString {
    return name_;
  }

  auto UiParameterTreeModel::setName(const QString& name) -> void {
    if (name_ != name) {
      name_ = name;
      nameChanged();
    }
  }

  auto UiParameterTreeModel::getVersion() const -> unsigned int {
    return version_;
  }

  auto UiParameterTreeModel::setVersion(unsigned int version) -> void {
    if (version_ != version) {
      version_ = version;
      versionChanged();
    }
  }

  auto UiParameterTreeModel::getLabel() const -> QString {
    return label_;
  }

  auto UiParameterTreeModel::setLabel(const QString& label) -> void {
    if (label_ != label) {
      label_ = label;
      labelChanged();
    }
  }

  auto UiParameterTreeModel::getKey() const -> QString {
    return key_;
  }

  auto UiParameterTreeModel::setKey(const QString& key) -> void {
    if (key_ != key) {
      key_ = key;
      keyChanged();
    }
  }

  auto UiParameterTreeModel::getComponent() const -> QString {
    return component_;
  }

  auto UiParameterTreeModel::setComponent(const QString& component) -> void {
    if (component_ != component) {
      component_ = component;
      componentChanged();
    }
  }

  auto UiParameterTreeModel::getLevel() const -> unsigned int {
    auto max_level = level_;

    forEach([&max_level](const auto& child) mutable {
      const auto child_level = child->getLevel();
      max_level = std::max(max_level, child_level);
    });

    return max_level;
  }

  auto UiParameterTreeModel::setLevel(unsigned int level) -> void {
    if (level_ != level) {
      level_ = level;
      levelChanged();
    }
  }

  auto UiParameterTreeModel::getDefaultLevel() const -> unsigned int {
    return default_level_;
  }

  auto UiParameterTreeModel::setDefaultLevel(unsigned int default_level) -> void {
    if (default_level_ != default_level) {
      default_level_ = default_level;
      defaultLevelChanged();
    }
  }

  auto UiParameterTreeModel::isLevelCustomized() const -> bool {
    if (isLeafNode()) {
      return level_customized_;
    }

    bool has_customized_child{ false };
    forEach([&has_customized_child](const auto& child, bool& break_loop) mutable {
      has_customized_child = child->isLevelCustomized();
      break_loop = has_customized_child;
    });

    return has_customized_child;
  }

  auto UiParameterTreeModel::setLevelCustomized(bool customized) -> void {
    if (level_customized_ != customized) {
      level_customized_ = customized;
      levelCustomizedChanged();
    }
  }

  auto UiParameterTreeModel::isOverrideable() const -> bool {
    return overrideable_;
  }

  auto UiParameterTreeModel::setOverrideable(bool overrideable) -> void {
    if (overrideable_ != overrideable) {
      overrideable_ = overrideable;
      overrideableChanged();
    }
  }

  auto UiParameterTreeModel::isOverrode() const -> bool {
    return overrode_;
  }

  auto UiParameterTreeModel::setOverrode(bool overrode) -> void {
    if (overrode_ != overrode) {
      overrode_ = overrode;
      overrodeChanged();
    }
  }

  auto UiParameterTreeModel::isModifyed() const -> bool {
    if (isLeafNode()) {
      return modifyed_;
    }

    auto modifyed{ false };
    forEach([&modifyed](const auto& child, bool& break_loop) {
      modifyed = child->isModifyed();
      break_loop = modifyed;
    });

    return modifyed;
  }

  auto UiParameterTreeModel::getTranslation() const -> QString {
    return translation_;
  }

  auto UiParameterTreeModel::setTranslation(const QString& translation) -> void {
    if (translation_ != translation) {
      translation_ = translation;
      translationChanged();
    }
  }

  auto UiParameterTreeModel::setModifyed(bool modifyed) -> void {
    if (isLeafNode() && modifyed_ != modifyed) {
      modifyed_ = modifyed;
      modifyedChanged();
    }
  }

  auto UiParameterTreeModel::getIcon() const -> QString {
    return icon_;
  }

  auto UiParameterTreeModel::setIcon(const QString& icon) -> void {
    if (icon_ != icon) {
      icon_ = icon;
      iconChanged();
    }
  }

  auto UiParameterTreeModel::getCheckedIcon() const -> QString {
    return checked_icon_;
  }

  auto UiParameterTreeModel::setCheckedIcon(const QString& checked_icon) -> void {
    if (checked_icon_ != checked_icon) {
      checked_icon_ = checked_icon;
      checkedIconChanged();
    }
  }

  auto UiParameterTreeModel::getModifyedIcon() const -> QString {
    return modifyed_icon_;
  }

  auto UiParameterTreeModel::setModifyedIcon(const QString& modifyed_icon) -> void {
    if (modifyed_icon_ != modifyed_icon) {
      modifyed_icon_ = modifyed_icon;
      modifyedIconChanged();
    }
  }

  auto UiParameterTreeModel::getLocalIcon() const -> QString {
    // BIN/resources/sliceconfig/ENGINE/param/ui/image/THEME/KEY.svg
    if (!isLeafNode()) {
      return {};
    }

    const auto bin = QCoreApplication::applicationDirPath();

    const auto engine = [] {
      switch (getKernel()->globalConst()->getEngineType()) {
        case EngineType::ET_CURA: {
          return QStringLiteral("cura");
        }
        case EngineType::ET_ORCA: {
          return QStringLiteral("orca");
        }
      }
      return QStringLiteral("orca");
    }();

    const auto theme = getThemeName();

    const auto key = [this] {
      auto key = getKey();
      const auto& map = getKernel()->uiParameterManager()->uiImageMap();
      const auto iter = map.find(key);
      return iter != map.cend() ? iter->second : key;
    }();

    const auto path = QStringLiteral("%1/resources/sliceconfig/%2/param/ui/image/%3/%4.svg")
                        .arg(bin, engine, theme, key);

    return std::ifstream{ path.toStdString() }.good()
             ? QStringLiteral("file:///%1").arg(path)
             : QStringLiteral("qrc:/UI/photo/parameter/tip_default.png");
  }

  auto UiParameterTreeModel::getModifyedTip() const -> QString {
    if (!isForkNode() || !isModifyed()) {
      return {};
    }

    QString translation{};

    using node_t = std::shared_ptr<const UiParameterTreeModel>;
    const std::function<void(const node_t&)> node_handler =
      [&translation, &node_handler](const node_t& node) mutable {
        if (!node->isLeafNode()) {
          node->forEach(node_handler);
          return;
        }

        if (!node->isModifyed()) {
          return;
        }

        if (!translation.isEmpty()) {
          translation.push_back(QStringLiteral(", "));
        }

        translation.push_back(node->getTranslation());
      };

    node_handler(sharedFromThis());

    const auto format = QCoreApplication::translate("ParameterComponent",
                                                    "fork_node_modifyed_tip_format");
    return format.arg(translation);
  }





  auto UiParameterTreeModel::get(int index) const -> QAbstractItemModel* {
    if (childNodes().size() <= static_cast<size_t>(index)) {
      return nullptr;
    }

    return childNodes().at(static_cast<size_t>(index)).get();
  }

  auto UiParameterTreeModel::getChildNode(const QString& uid,
                                          bool recursive) const -> QAbstractItemModel* {
    QAbstractItemModel* result = findChildNode(uid).get();
    if (!recursive || result) {
      return result;
    }

    forEach([uid, recursive, &result](const auto& child, bool& break_loop) mutable {
      result = child->getChildNode(uid, recursive);
      break_loop = result != nullptr;
    });

    return result;
  }


  auto UiParameterTreeModel::resetLevel() -> void {
    if (childNodes().empty()) {
      setLevel(getDefaultLevel());
    } else {
      forEach([](const auto& child) {
        child->resetLevel();
      });
    }
  }





  auto UiParameterTreeModel::sharedFromThis() -> std::shared_ptr<UiParameterTreeModel> {
    return std::static_pointer_cast<UiParameterTreeModel>(shared_from_this());
  }

  auto UiParameterTreeModel::sharedFromThis() const -> std::shared_ptr<const UiParameterTreeModel> {
    return std::static_pointer_cast<const UiParameterTreeModel>(shared_from_this());
  }

  auto UiParameterTreeModel::parentNode() const -> std::shared_ptr<UiParameterTreeModel> {
    return std::static_pointer_cast<UiParameterTreeModel>(
      qtuser_qml::CustomTreeModel::parentNode());
  }

  auto UiParameterTreeModel::copy(const std::shared_ptr<UiParameterTreeModel>& that) -> void {
    setLabel(that->getLabel());
    setKey(that->getKey());
  }

  auto UiParameterTreeModel::forEach(const LoopHandler& handler) const -> void {
    if (!handler) {
      return;
    }

    for (const auto& child : childNodes()) {
      handler(std::static_pointer_cast<UiParameterTreeModel>(child));
    }
  }

  auto UiParameterTreeModel::forEach(const BreakableLoopHandler& handler) const -> void {
    if (!handler) {
      return;
    }

    for (const auto& child : childNodes()) {
      bool break_loop = false;
      handler(std::static_pointer_cast<UiParameterTreeModel>(child), break_loop);
      if (break_loop) {
        break;
      }
    }
  }

  auto UiParameterTreeModel::getThemeName() const -> QString {
    return theme_name_;
  }

  auto UiParameterTreeModel::setThemeName(const QString& theme_name) -> void {
    if (theme_name_ != theme_name) {
      theme_name_ = theme_name;
      updateIcon();
    }

    forEach([theme_name](const auto& child) {
      child->setThemeName(theme_name);
    });
  }





  auto UiParameterTreeModel::updateUid() -> void {
    if (isRootNode()) {
      setUid({});
      return;
    }

    auto id = getKey();
    if (id.isEmpty()) {
      id = getLabel().toLower()
                     .replace(QStringLiteral(" "), QStringLiteral("_"))
                     .replace(QStringLiteral("-"), QStringLiteral("_"))
                     .replace(QStringLiteral("/"), QStringLiteral("_"));
    }

    setUid(QStringLiteral("%1/%2").arg(parentNode()->getUid()).arg(id));
  }

  auto UiParameterTreeModel::updateIcon() -> void {
    const auto type = getTypeString();
    const auto uid = getUid().replace(QStringLiteral("/"), QStringLiteral("-"));
    const auto theme = getThemeName();

    // qrc:/UI/profile/type-label-key--theme.svg
    setIcon(QStringLiteral("qrc:/UI/photo/profile/%1%2--%3.svg").arg(type, uid, theme));

    // qrc:/UI/photo/profile/type-label-key--theme-checked.svg
    setCheckedIcon(
      QStringLiteral("qrc:/UI/photo/profile/%1%2--%3-checked.svg").arg(type, uid, theme));

    // qrc:/UI/photo/profile/type-label-key--theme-modifyed.svg
    setModifyedIcon(
      QStringLiteral("qrc:/UI/photo/profile/%1%2--%3-modifyed.svg").arg(type, uid, theme));

    localIconChanged();
  }

}
