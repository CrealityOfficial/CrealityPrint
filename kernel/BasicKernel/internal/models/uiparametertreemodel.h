#pragma once

#ifndef INTERNAL_MODELS_UIPARAMTREEMODEL_H_
#define INTERNAL_MODELS_UIPARAMTREEMODEL_H_

#include <rapidjson/document.h>

#include <qtusercore/plugin/customtreemodel.h>

namespace creative_kernel {

  /// @brief Custom tree model for json ui param file.
  ///
  /// For the example json (part of c3d/resource/sliceconfig/cura/param/ui/machine.json): {
  ///   "name": "FDM Printer Machine Description",
  ///   "version": 2,
  ///   "children": [
  ///     {
  ///       "label": "Basic information",
  ///       "children": [
  ///         {
  ///           "label": "Information",
  ///           "children": [
  ///             {
  ///               "key": "machine_name",
  ///               "level": 2
  ///             },
  ///             }
  ///               "key": "machine_shape",
  ///               "level": 1
  ///             },
  ///             // more children ...
  ///           ]
  ///         },
  ///         // more children ...
  ///       ]
  ///     },
  ///     // more children ...
  ///   ]
  /// }
  ///
  /// the corresponding tree model will be like:
  /// [root] {}
  ///   |
  ///   |--[fork] { label: "Basic information" }
  ///   |     |
  ///   |     |--[fork] { label: "Information" }
  ///   |     |     |
  ///   |     |     |--[leaf] { key: "machine_name", level: 2, type: "string" }
  ///   |     |     |
  ///   |     |     |--[leaf] { key: "machine_shape", level: 1, type: "string" }
  ///   |     |     |
  ///   |     |     |-- more leafs ...
  ///   |     |
  ///   |     |-- more forks ...
  ///   |
  ///   |-- more forks...
  class UiParameterTreeModel : public qtuser_qml::FlattenTreeModel {
    Q_OBJECT;

    auto operator=(const UiParameterTreeModel&) -> UiParameterTreeModel = delete;
    auto operator=(UiParameterTreeModel&&) -> UiParameterTreeModel = delete;
    UiParameterTreeModel(const UiParameterTreeModel&) = delete;
    UiParameterTreeModel(UiParameterTreeModel&&) = delete;

   public:
    /// @brief corresponds with machine.json / material.json / process.json
    enum class Type : int {
      MACHINE  = 0,
      MATERIAL = 1,
      PROCESS  = 2,
    };

   public:
    static auto CreateRoot() -> std::shared_ptr<UiParameterTreeModel>;
    auto createChild() -> std::shared_ptr<UiParameterTreeModel>;
    auto eraseChild(const QString& uid) -> std::shared_ptr<UiParameterTreeModel>;

    virtual ~UiParameterTreeModel() = default;

   public:  // properties from tree model
    /// @brief [readonly] uid in entire tree like: "parent_uid/key"
    auto getUid() const -> QString override;

   public:  // properties from json ui param file
    /// @brief [qml-readonly] type of tree, consistent with root
    auto getType() const -> Type;
    /// @return one of : "process" or "machine" or "material"
    auto getTypeString() const -> QString;
    auto setType(Type type) -> void;
    Q_SIGNAL void typeChanged() const;
    Q_PROPERTY(QString type READ getTypeString NOTIFY typeChanged);

    /// @brief [qml-readonly] 'name' in root node
    auto getName() const -> QString;
    auto setName(const QString& name) -> void;
    Q_SIGNAL void nameChanged() const;
    Q_PROPERTY(QString name READ getName NOTIFY nameChanged);

    /// @brief [qml-readonly] 'version' in root node
    auto getVersion() const -> unsigned int;
    auto setVersion(unsigned int version) -> void;
    Q_SIGNAL void versionChanged() const;
    Q_PROPERTY(unsigned int version READ getVersion NOTIFY versionChanged);

    /// @brief [qml-readonly] 'label' in non-root node
    auto getLabel() const -> QString;
    auto setLabel(const QString& label) -> void;
    Q_SIGNAL void labelChanged() const;
    Q_PROPERTY(QString label READ getLabel NOTIFY labelChanged);

    /// @brief [qml-readonly] 'key' in non-root node
    auto getKey() const -> QString;
    auto setKey(const QString& key) -> void;
    Q_SIGNAL void keyChanged() const;
    Q_PROPERTY(QString key READ getKey NOTIFY keyChanged);

    /// @brief [qml-readonly] 'component' in non-root node, one of: [
    ///   "text_field",  // default
    ///   "int_text_field",
    ///   "float_text_field",
    ///   "text_edit",
    ///   "array_text_edit",
    ///   "check_box",
    ///   "combo_box",
    ///   "image_combo_box",
    ///   "text_field_combo_box",
    ///   "int_text_field_combo_box",
    ///   "material_combo_box",
    ///   "fork_tiny_label",
    /// ]
    auto getComponent() const -> QString;
    auto setComponent(const QString& component) -> void;
    Q_SIGNAL void componentChanged() const;
    Q_PROPERTY(QString component READ getComponent NOTIFY componentChanged);

    /// @brief 'level' in non-root node: 0-expert, 1-advanced, 2-normal;
    ///        for override node: 0-not enabled, 1-enabled;
    auto getLevel() const -> unsigned int;
    auto setLevel(unsigned int level) -> void;
    Q_SIGNAL void levelChanged() const;
    Q_PROPERTY(unsigned int level READ getLevel WRITE setLevel NOTIFY levelChanged);

    /// @brief [qml-readonly] 'defaultLevel' in non-root node
    auto getDefaultLevel() const -> unsigned int;
    auto setDefaultLevel(unsigned int default_level) -> void;
    Q_SIGNAL void defaultLevelChanged() const;
    Q_PROPERTY(unsigned int defaultLevel READ getDefaultLevel NOTIFY defaultLevelChanged);

    auto isLevelCustomized() const -> bool;
    auto setLevelCustomized(bool customized) -> void;
    Q_SIGNAL void levelCustomizedChanged() const;
    Q_PROPERTY(bool levelCustomized
               READ isLevelCustomized
               WRITE setLevelCustomized
               NOTIFY levelCustomizedChanged);

    /// @brief [qml-readonly] 'overrideable' in material override node
    auto isOverrideable() const -> bool;
    auto setOverrideable(bool overrideable) -> void;
    Q_SIGNAL void overrideableChanged() const;
    Q_PROPERTY(bool overrideable READ isOverrideable NOTIFY overrideableChanged);

    /// @brief [qml-readonly] 'overrode' in material override node
    auto isOverrode() const -> bool;
    auto setOverrode(bool overrode) -> void;
    Q_SIGNAL void overrodeChanged() const;
    Q_PROPERTY(bool overrode READ isOverrode NOTIFY overrodeChanged);

   public:  // properties from qml
    /// @brief [qml-readonly] icon file url (start with "qrc:") for non-root node
    auto getIcon() const -> QString;
    Q_SIGNAL void iconChanged() const;
    Q_PROPERTY(QString icon READ getIcon NOTIFY iconChanged);

    /// @brief [qml-readonly] checked status icon file url (start with "qrc:") for non-root node
    auto getCheckedIcon() const -> QString;
    Q_SIGNAL void checkedIconChanged() const;
    Q_PROPERTY(QString checkedIcon READ getCheckedIcon NOTIFY checkedIconChanged);

    /// @brief [qml-readonly] modifyed status icon file url (start with "qrc:") for non-root node
    auto getModifyedIcon() const -> QString;
    Q_SIGNAL void modifyedIconChanged() const;
    Q_PROPERTY(QString modifyedIcon READ getModifyedIcon NOTIFY modifyedIconChanged);

    /// @brief [qml-readonly] icon file url (start with "file:") for non-root node
    auto getLocalIcon() const -> QString;
    Q_SIGNAL void localIconChanged() const;
    Q_PROPERTY(QString localIcon READ getLocalIcon NOTIFY localIconChanged);

    /// @brief [qml-readonly] modifyed status tool tip for fork node
    auto getModifyedTip() const -> QString;
    Q_SIGNAL void modifyedTipChanged() const;
    Q_PROPERTY(QString modifyedTip READ getModifyedTip NOTIFY modifyedTipChanged);

   public:  // for qml
    Q_INVOKABLE QAbstractItemModel* get(int index) const;

    Q_INVOKABLE QAbstractItemModel* getChildNode(const QString& uid, bool recursive = false) const;
    Q_INVOKABLE void resetLevel();

    Q_SIGNAL void requestHighlight() const;

    auto isModifyed() const -> bool;
    auto setModifyed(bool modifyed) -> void;
    Q_SIGNAL void modifyedChanged();
    Q_PROPERTY(bool modifyed READ isModifyed WRITE setModifyed NOTIFY modifyedChanged);

    auto getTranslation() const -> QString;
    auto setTranslation(const QString& translation) -> void;
    Q_SIGNAL void translationChanged() const;
    Q_PROPERTY(QString translation
               READ getTranslation
               WRITE setTranslation
               NOTIFY translationChanged);

   public:  // for cpp
    auto sharedFromThis() -> std::shared_ptr<UiParameterTreeModel>;
    auto sharedFromThis() const -> std::shared_ptr<const UiParameterTreeModel>;

    auto parentNode() const -> std::shared_ptr<UiParameterTreeModel>;

    auto copy(const std::shared_ptr<UiParameterTreeModel>& that) -> void;

    /// @brief use return to continue loop
    using LoopHandler = std::function<void(const std::shared_ptr<UiParameterTreeModel>&)>;
    auto forEach(const LoopHandler& handler) const -> void;

    /// @param bool& break_loop : set true to break loop, default with false
    using BreakableLoopHandler =
      std::function<void(const std::shared_ptr<UiParameterTreeModel>&, bool&)>;
    auto forEach(const BreakableLoopHandler& handler) const -> void;

    auto getThemeName() const -> QString;
    auto setThemeName(const QString& theme) -> void;

   protected:
    explicit UiParameterTreeModel();
    explicit UiParameterTreeModel(std::shared_ptr<UiParameterTreeModel> parent);

   protected:
    using qtuser_qml::CustomTreeModel::setUid;
    using qtuser_qml::CustomTreeModel::setParentNode;
    using qtuser_qml::CustomTreeModel::emplaceBackChildNode;
    using qtuser_qml::CustomTreeModel::eraseChildNode;

    auto setIcon(const QString& icon) -> void;
    auto setCheckedIcon(const QString& checked_icon) -> void;
    auto setModifyedIcon(const QString& modifyed_icon) -> void;

    Q_SLOT void updateUid();
    Q_SLOT void updateIcon();

   private:
    Type type_{ Type::MACHINE };
    QString name_{};
    unsigned int version_{ 0u };

    QString label_{};
    QString key_{};
    QString component_{ QStringLiteral("text_field") };
    unsigned int level_{ 0u };
    unsigned int default_level_{ 0u };
    bool level_customized_{ false };
    bool overrideable_{ false };
    bool overrode_{ false };

    QString theme_name_{};
    QString icon_{};
    QString checked_icon_{};
    QString modifyed_icon_{};
    QString local_icon_{};

    bool modifyed_{ false };
    QString translation_{};
  };

}  // namespace creative_kernel

#endif
