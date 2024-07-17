#include "internal/parameter/uiparametermanager.h"

#include <functional>
#include <set>
#include <stack>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <qtusercore/string/resourcesfinder.h>

#include "external/interface/appsettinginterface.h"
#include "external/interface/commandinterface.h"
#include "external/kernel/enginedatatype.h"
#include "external/kernel/globalconst.h"
#include "external/kernel/kernel.h"

#include "internal/models/uiparametersearchmodel.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printmaterial.h"
#include "internal/parameter/printprofile.h"
#include "internal/parameter/processoverridecontext.h"

namespace creative_kernel {

  namespace {

    const auto MACHINE_FILE_NAME{ QStringLiteral("machine.json") };
    const auto MATERIAL_FILE_NAME{ QStringLiteral("material.json") };
    const auto PROCESS_FILE_NAME{ QStringLiteral("process.json") };

    const auto OVERRIDE_LABEL{ QStringLiteral("Overrides") };
    const auto EXTRUDER_LABEL{ QStringLiteral("Extruder") };
    const auto PROCESS_LABEL{ QStringLiteral("Process") };

    auto FindReadonlyFile(const QString& file_name) -> QFileInfo {
      const auto dir_path = getKernel()->globalConst()->getUiParameterDirPath();
      return { QStringLiteral("%1/%2").arg(dir_path).arg(file_name) };
    }

    auto FindWriteableFile(const QString& file_name) -> QFileInfo {
      const auto dir_path = qtuser_core::getOrCreateAppDataLocation(getEnginePathPrefix() +
                                                                    QStringLiteral("ui_parameter"));
      return { QStringLiteral("%1/%2").arg(dir_path).arg(file_name) };
    }

    auto ReadJsonFile(const QString& file_path) -> rapidjson::Document {
      QFile json_file{ file_path };
      if (!json_file.open(QFile::OpenModeFlag::ReadOnly)) {
        return {};
      }

      const auto json_data = json_file.readAll();

      rapidjson::StringStream json_stream{ json_data.constData() };
      rapidjson::Document json_document;
      if (json_document.ParseStream(json_stream).HasParseError()) {
        return {};
      }

      return json_document;
    }

    auto WriteJsonFile(const rapidjson::Document& json_document, const QString& file_path) -> void {
      rapidjson::StringBuffer json_buffer;
      rapidjson::PrettyWriter<rapidjson::StringBuffer> json_writer{ json_buffer };
#ifdef QT_DEBUG
      json_writer.SetIndent(' ', 2);
#endif
      json_document.Accept(json_writer);

      QFile json_file{ file_path };
      if (!json_file.open(QFile::OpenModeFlag::WriteOnly)) {
        return;
      }

      json_file.write(json_buffer.GetString());
    }

    auto JsonToTreeModel(
      const rapidjson::Document& json_root,
      const std::shared_ptr<UiParameterTreeModel>& model_root,
      std::function<void(const std::shared_ptr<UiParameterTreeModel>&)> after_node_init = nullptr)
        -> void {
      using json_node_t = rapidjson::Value;
      using model_node_t = std::shared_ptr<UiParameterTreeModel>;

      std::function<bool(const json_node_t&)> has_children{ nullptr };
      std::function<void(const json_node_t&, const model_node_t&)> load_properties{ nullptr };
      std::function<void(const json_node_t&, const model_node_t&)> load_children{ nullptr };
      std::function<void(const json_node_t&, const model_node_t&)> load_node{ nullptr };

      has_children = [](const json_node_t& json_node) -> bool {
        return json_node.HasMember("children") &&
               json_node["children"].IsArray() &&
               !json_node["children"].GetArray().Empty();
      };

      load_properties = [&after_node_init]
                        (const json_node_t& json_node, const model_node_t& model_node) -> void {
        if (json_node.HasMember("name") && json_node["name"].IsString()) {
          model_node->setName(json_node["name"].GetString());
        }

        if (json_node.HasMember("version") && json_node["version"].IsUint()) {
          model_node->setVersion(json_node["version"].GetUint());
        }

        if (json_node.HasMember("label") && json_node["label"].IsString()) {
          model_node->setLabel(json_node["label"].GetString());
        }

        if (json_node.HasMember("key") && json_node["key"].IsString()) {
          model_node->setKey(json_node["key"].GetString());
        }

        if (json_node.HasMember("level") && json_node["level"].IsUint()) {
          model_node->setLevel(json_node["level"].GetUint());
          model_node->setDefaultLevel(model_node->getLevel());
        }

        if (json_node.HasMember("component") && json_node["component"].IsString()) {
          model_node->setComponent(json_node["component"].GetString());
        }

        if (json_node.HasMember("overrideable") && json_node["overrideable"].IsBool()) {
          model_node->setOverrideable(json_node["overrideable"].GetBool());
        }
      };

      load_children = [&has_children, &load_node]
                      (const json_node_t& json_node, const model_node_t& model_node) -> void {
        for (const auto& child_json_node : json_node["children"].GetArray()) {
          auto child_model_node = model_node->createChild();
          load_node(child_json_node, child_model_node);
        }
      };

      load_node = [&load_properties, &has_children, &load_children, &after_node_init]
                  (const json_node_t& json_node, const model_node_t& model_node) {
        load_properties(json_node, model_node);

        if (has_children(json_node)) {
          load_children(json_node, model_node);
        }

        if (after_node_init) {
          after_node_init(model_node);
        }
      };

      load_node(json_root, model_root);
    }

    auto TreeModelToJson(const std::shared_ptr<const UiParameterTreeModel>& model_root,
                         rapidjson::Document& json_root) -> void {
      using json_node_t = rapidjson::Value;
      using model_node_t = std::shared_ptr<const UiParameterTreeModel>;

      std::function<bool(const model_node_t&)> has_children{ nullptr };
      std::function<void(const model_node_t&, json_node_t&)> save_properties{ nullptr };
      std::function<void(const model_node_t&, json_node_t&)> save_children{ nullptr };
      std::function<void(const model_node_t&, json_node_t&)> save_node{ nullptr };

      auto& allocator = json_root.GetAllocator();

      has_children = [](const model_node_t& model_node) -> bool {
        return !model_node->childNodes().empty();
      };

      save_properties = [allocator]
                        (const model_node_t& model_node, json_node_t& json_node) mutable -> void {
        const auto model_name = model_node->getName();
        if (!model_name.isEmpty()) {
          json_node_t json_name{ rapidjson::Type::kStringType };
          json_name.Set(model_name.toUtf8().constData(), allocator);
          json_node.AddMember("name", std::move(json_name), allocator);

          json_node_t json_version{ rapidjson::Type::kNumberType };
          json_version.Set(model_node->getVersion());
          json_node.AddMember("version", std::move(json_version), allocator);
        }

        const auto model_label = model_node->getLabel();
        if (!model_label.isEmpty()) {
          json_node_t json_label{ rapidjson::Type::kStringType };
          json_label.Set(model_label.toUtf8().constData(), allocator);
          json_node.AddMember("label", std::move(json_label), allocator);
        }

        const auto model_key = model_node->getKey();
        if (!model_key.isEmpty()) {
          json_node_t json_key{ rapidjson::Type::kStringType };
          json_key.Set(model_key.toUtf8().constData(), allocator);
          json_node.AddMember("key", std::move(json_key), allocator);

          json_node_t json_level{ rapidjson::Type::kNumberType };
          json_level.Set(model_node->getLevel());
          json_node.AddMember("level", std::move(json_level), allocator);
        }
      };

      save_children = [allocator, &save_node]
                      (const model_node_t& model_node, json_node_t& json_node) mutable -> void {
        json_node_t json_children{ rapidjson::Type::kArrayType };

        model_node->forEach([allocator, &save_node, &json_children]
                            (const auto& model_child_node) mutable {
          json_node_t json_child_node{ rapidjson::Type::kObjectType };
          save_node(model_child_node, json_child_node);
          json_children.PushBack(std::move(json_child_node), allocator);
        });

        if (!json_node.HasMember("children")) {
          json_node.AddMember("children", std::move(json_children), allocator);
        } else if (!json_node["children"].IsArray()) {
          json_node["children"] = std::move(json_children);
        }
      };

      save_node = [&save_properties, &has_children, &save_children]
                  (const model_node_t& model_node, json_node_t& json_node) {
        save_properties(model_node, json_node);

        if (has_children(model_node)) {
          save_children(model_node, json_node);
        }
      };

      save_node(model_root, json_root);
    }


    auto GetLevelCustomizedKeys() -> std::set<QString> {
      const auto file_info = FindWriteableFile(PROCESS_FILE_NAME);
      if (!file_info.isFile()) {
        return {};
      }

      const auto document = ReadJsonFile(file_info.absoluteFilePath());
      if (document.HasParseError() || !document.IsObject()) {
        return {};
      }

      const auto json_root = document.GetObject();
      if (!json_root.HasMember("level_customized_keys") ||
          !json_root["level_customized_keys"].IsArray()) {
        return {};
      }

      std::set<QString> keys{};
      for (const auto& value : json_root["level_customized_keys"].GetArray()) {
        if (value.IsString()) {
          keys.emplace(QString::fromUtf8(value.GetString()));
        }
      }

      return keys;
    }

    auto SetLevelCustomizedKeys(const std::set<QString>& keys) -> void {
      const auto file_path = FindWriteableFile(PROCESS_FILE_NAME).absoluteFilePath();
      auto document = ReadJsonFile(file_path);
      if (document.HasParseError() || !document.IsObject()) {
        document = rapidjson::Document{ rapidjson::Type::kObjectType };
      }

      auto& allocator = document.GetAllocator();

      rapidjson::Value json_key_array{ rapidjson::Type::kArrayType };
      for (const auto& key : keys) {
        rapidjson::Value json_key{ rapidjson::Type::kStringType };
        json_key.Set(key.toUtf8().constData(), allocator);
        json_key_array.PushBack(std::move(json_key), allocator);
      }

      if (document.HasMember("level_customized_keys")) {
        document["level_customized_keys"] = std::move(json_key_array);
      } else {
        document.AddMember("level_customized_keys", std::move(json_key_array), allocator);
      }

      WriteJsonFile(document, file_path);
    }

  }  // namespace

  UiParameterManager::UiParameterManager(QPointer<ParameterManager> parameter_manager,
                                         SliceFlow::EngineType engine_type,
                                         QObject* parent)
      : QObject{ parent }
      , parameter_manager_{ std::move(parameter_manager) }
      , engine_type_{ engine_type }
      , initialized_{ false }
      , machine_tree_model_{ UiParameterTreeModel::CreateRoot() }
      , process_tree_model_{ UiParameterTreeModel::CreateRoot() }
      , material_tree_model_{ UiParameterTreeModel::CreateRoot() }
      , override_tree_model_{ UiParameterTreeModel::CreateRoot() }
      , machine_compare_tree_model_{ UiParameterTreeModel::CreateRoot() }
      , process_compare_tree_model_{ UiParameterTreeModel::CreateRoot() }
      , material_compare_tree_model_{ UiParameterTreeModel::CreateRoot() }
      , override_compare_tree_model_{ UiParameterTreeModel::CreateRoot() }
      , process_override_context_{ std::make_unique<ProcessOverrideContext>(process_tree_model_) }
      , process_level_list_model_{ std::make_unique<ProcessLevelListModel>(nullptr) } {
    machine_tree_model_->setFlattenable(false);
    process_tree_model_->setFlattenable(false);
    material_tree_model_->setFlattenable(false);

    connect(this, &UiParameterManager::focusedMaterialNameChanged, this, [this] {
      syncOverrideTreeModel();
    });

    connect(this, &UiParameterManager::focusedProcessNameChanged, this, [this] {
      syncOverrideTreeModel();
    });

    assert(parameter_manager_ && "set a vaild ParameterManager plz");
    if (parameter_manager_) {
      connect(parameter_manager_.data(), &ParameterManager::curMachineIndexChanged, this, [this] {
        setFocusedMaterialName({});
        setFocusedProcessName({});

        if (!parameter_manager_) {
          return;
        }

        auto* const machine = parameter_manager_->currentMachine();
        if (!machine) {
          return;
        }

        const auto material_list = machine->materialsNameList();
        if (!material_list.empty()) {
          setFocusedMaterialName(material_list.front());
        }

        auto* const process = machine->profile(machine->curProfileIndex());
        if (process) {
          setFocusedProcessName(process->uniqueName());
        }
      });
    }

    machine_tree_model_->setType(UiParameterTreeModel::Type::MACHINE);
    process_tree_model_->setType(UiParameterTreeModel::Type::PROCESS);
    material_tree_model_->setType(UiParameterTreeModel::Type::MATERIAL);
    override_tree_model_->setType(UiParameterTreeModel::Type::MATERIAL);
    machine_compare_tree_model_->setType(UiParameterTreeModel::Type::MACHINE);
    process_compare_tree_model_->setType(UiParameterTreeModel::Type::PROCESS);
    material_compare_tree_model_->setType(UiParameterTreeModel::Type::MATERIAL);
    override_compare_tree_model_->setType(UiParameterTreeModel::Type::MATERIAL);

    addUIVisualTracer(this,this);
  }

  UiParameterManager::~UiParameterManager() {
    removeUIVisualTracer(this);

    if (isInitialized()) {
      uninitialize();
    }
  }

  auto UiParameterManager::initialize() -> void {
    if (isInitialized()) {
      assert(false && "UiParameterManager initialize repeatly!");
      return;
    }

    if (!loadUiImageMap()) {
      // assert(false && "UiParameterManager initialize failed!");
    }

    if (!loadKeyLabelMap()) {
      assert(false && "UiParameterManager initialize failed!");
    }

    if (!loadMachineTreeModel()) {
      assert(false && "UiParameterManager initialize failed!");
    }

    if (!loadMaterialTreeModel()) {
      assert(false && "UiParameterManager initialize failed!");
    }

    if (!loadProcessTreeModel()) {
      assert(false && "UiParameterManager initialize failed!");
    }

    if (!loadOverrideTreeModel()) {
      assert(false && "UiParameterManager initialize failed!");
    }

    initialized_ = true;
  }

  auto UiParameterManager::uninitialize() -> void {
    if (!isInitialized()) {
      assert(false && "UiParameterManager uninitialize repeatly!");
      return;
    }

    saveProcessCustomizedKeys();

    initialized_ = false;
  }

  auto UiParameterManager::isInitialized() const -> bool {
    return initialized_;
  }

  auto UiParameterManager::getMachineTreeModel() const -> std::shared_ptr<UiParameterTreeModel> {
    return machine_tree_model_;
  }

  auto UiParameterManager::getMachineTreeModelObject() const -> QAbstractItemModel* {
    return machine_tree_model_.get();
  }

  auto UiParameterManager::getProcessTreeModel() const -> std::shared_ptr<UiParameterTreeModel> {
    return process_tree_model_;
  }

  auto UiParameterManager::getProcessTreeModelObject() const -> QAbstractItemModel* {
    return process_tree_model_.get();
  }

  auto UiParameterManager::getMaterialTreeModel() const -> std::shared_ptr<UiParameterTreeModel> {
    return material_tree_model_;
  }

  auto UiParameterManager::getMaterialTreeModelObject() const -> QAbstractItemModel* {
    return material_tree_model_.get();
  }

  auto UiParameterManager::getOverrideTreeModel() const -> std::shared_ptr<UiParameterTreeModel> {
    return override_tree_model_;
  }

  auto UiParameterManager::getOverrideTreeModelObject() const -> QAbstractItemModel* {
    return override_tree_model_.get();
  }

  auto UiParameterManager::getMachineCompareTreeModel() const
      -> std::shared_ptr<UiParameterTreeModel> {
    return machine_compare_tree_model_;
  }

  auto UiParameterManager::getMachineCompareTreeModelObject() const -> QAbstractItemModel* {
    return machine_compare_tree_model_.get();
  }

  auto UiParameterManager::getProcessCompareTreeModel() const
      -> std::shared_ptr<UiParameterTreeModel> {
    return process_compare_tree_model_;
  }

  auto UiParameterManager::getProcessCompareTreeModelObject() const -> QAbstractItemModel* {
    return process_compare_tree_model_.get();
  }

  auto UiParameterManager::getMaterialCompareTreeModel() const
      -> std::shared_ptr<UiParameterTreeModel> {
    return material_compare_tree_model_;
  }

  auto UiParameterManager::getMaterialCompareTreeModelObject() const -> QAbstractItemModel* {
    return material_compare_tree_model_.get();
  }

  auto UiParameterManager::getOverrideCompareTreeModel() const
      -> std::shared_ptr<UiParameterTreeModel> {
    return override_compare_tree_model_;
  }

  auto UiParameterManager::getOverrideCompareTreeModelObject() const -> QAbstractItemModel* {
    return override_compare_tree_model_.get();
  }

  auto UiParameterManager::getFocusedMaterialName() const -> QString {
    return focused_material_name_;
  }

  auto UiParameterManager::setFocusedMaterialName(const QString& name) -> void {
    if (focused_material_name_ != name) {
      focused_material_name_ = name;
      focusedMaterialNameChanged();
    }
  }

  auto UiParameterManager::getFocusedProcessName() const -> QString {
    return focused_process_name_;
  }

  auto UiParameterManager::setFocusedProcessName(const QString& name) -> void {
    if (focused_process_name_ != name) {
      focused_process_name_ = name;
      focusedProcessNameChanged();
    }
  }

  auto UiParameterManager::getProcessOverrideContextObject() const -> QObject* {
    return process_override_context_.get();
  }

  auto UiParameterManager::getProcessLevelListModelObject() const -> QObject* {
    return process_level_list_model_.get();
  }

  auto UiParameterManager::saveProcessCustomizedKeys() -> void {
    std::set<QString> keys{};

    using node_t = std::shared_ptr<UiParameterTreeModel>;

    const std::function<void(const node_t&)> node_handler =
      [this, &keys, &node_handler](const node_t& node) mutable {
        auto key = node->getKey();
        if (!key.isEmpty() && node->isLevelCustomized()) {
          keys.emplace(std::move(key));
        }

        node->forEach(node_handler);
      };

    node_handler(process_tree_model_->sharedFromThis());

    SetLevelCustomizedKeys(keys);
  }

  auto UiParameterManager::emplaceOverrideParameter(const QString& key) -> bool {
    if (!parameter_manager_ || focused_material_name_.isEmpty()) {
      return false;
    }

    auto* const current_machine = parameter_manager_->currentMachine();
    if (!current_machine) {
      return false;
    }

    auto* const focused_material = current_machine->materialWithName(focused_material_name_);
    if (!focused_material) {
      return false;
    }

    const auto iter = key_override_node_map_.find(key);
    if (iter == key_override_node_map_.cend() || iter->second.expired()) {
      return false;
    }

    const auto node = iter->second.lock();

    switch (engine_type_) {
      case SliceFlow::EngineType::CURA: {
        const auto parent_label = node->parentNode()->getLabel();
        if (parent_label == EXTRUDER_LABEL) {
          focused_material->addExtruderOverrideKey(key);
        } else if (parent_label == PROCESS_LABEL && !focused_process_name_.isEmpty()) {
          focused_material->addProcessOverrideKey(focused_process_name_, key);
        } else {
          return false;
        }
        break;
      }
      case SliceFlow::EngineType::OCRA: {
        auto* extruder_data = current_machine->getExtruder1DataModel();
        auto* material_data = focused_material->getDataModel();
        if (!extruder_data || !material_data) {
          return false;
        }

        auto extruder_key = key;
        extruder_key.replace(QStringLiteral("filament_"), QStringLiteral(""));

        auto extruder_item = extruder_data->findOrMakeDataItem(extruder_key);
        auto material_item = material_data->findOrMakeDataItem(key);
        if (extruder_item && material_item) {
          material_item->setValue(extruder_item->getValue());
        }

        break;
      }
      default: {
        break;
      }
    }

    node->setOverrode(true);
    return true;
  }

  auto UiParameterManager::eraseOverrideParameter(const QString& key) -> bool {
    if (!parameter_manager_ || focused_material_name_.isEmpty()) {
      return false;
    }

    const auto* current_machine = parameter_manager_->currentMachine();
    if (!current_machine) {
      return false;
    }

    auto* const focused_material = current_machine->materialWithName(focused_material_name_);
    if (!focused_material) {
      return false;
    }

    const auto iter = key_override_node_map_.find(key);
    if (iter == key_override_node_map_.cend() || iter->second.expired()) {
      return false;
    }

    const auto node = iter->second.lock();

    switch (engine_type_) {
      case SliceFlow::EngineType::CURA: {
        const auto parent_label = node->parentNode()->getLabel();
        if (parent_label == EXTRUDER_LABEL) {
          focused_material->removeExtruderOverrideKey(key);
        } else if (parent_label == PROCESS_LABEL && !focused_process_name_.isEmpty()) {
          focused_material->removeProcessOverrideKey(focused_process_name_, key);
        } else {
          return false;
        }
        break;
      }
      case SliceFlow::EngineType::OCRA: {
        auto data_item = focused_material->getDataModel()->findOrMakeDataItem(key);
        data_item->setValue(QStringLiteral("nil"));
        break;
      }
      default: {
        break;
      }
    }

    node->setOverrode(false);
    return true;
  }

  auto UiParameterManager::clearOverrideParameter() -> bool {
    if (!parameter_manager_ || focused_material_name_.isEmpty()) {
      return false;
    }

    const auto* current_machine = parameter_manager_->currentMachine();
    if (!current_machine) {
      return false;
    }

    auto* const focused_material = current_machine->materialWithName(focused_material_name_);
    if (!focused_material) {
      return false;
    }

    switch (engine_type_) {
      case SliceFlow::EngineType::CURA: {
        override_tree_model_->forEach([this, focused_material](const auto& fork) {
          const auto fork_label = fork->getLabel();
          if (fork_label == EXTRUDER_LABEL) {
            fork->forEach([focused_material](const auto& leaf) {
              if (leaf->isOverrode()) {
                focused_material->removeExtruderOverrideKey(leaf->getKey());
                leaf->setOverrode(false);
              }
            });
          } else if (fork_label == PROCESS_LABEL && !focused_process_name_.isEmpty()) {
            fork->forEach([this, focused_material](const auto& leaf) {
              if (leaf->isOverrode()) {
                focused_material->removeProcessOverrideKey(focused_process_name_, leaf->getKey());
                leaf->setOverrode(false);
              }
            });
          }
        });
        break;
      }
      case SliceFlow::EngineType::OCRA: {
        override_tree_model_->forEach([this, focused_material](const auto& fork) {
          fork->forEach([this, focused_material](const auto& leaf) {
            if (leaf->isOverrode()) {
              auto data_item = focused_material->getDataModel()->findOrMakeDataItem(leaf->getKey());
              data_item->setValue(QStringLiteral("nil"));
              leaf->setOverrode(false);
            }
          });
        });
        break;
      }
      default: {
        break;
      }
    }

    return true;
  }

  auto UiParameterManager::generateSearchContext(
      QObject* tree_model_object, const QString& translate_context) const -> QObject* {
    auto* tree_model_raw = dynamic_cast<UiParameterTreeModel*>(tree_model_object);
    if (!tree_model_raw) {
      assert(false && "argument 'tree_model_object' must be a UiParameterTreeModel");
      return nullptr;
    }

    using node_t = std::shared_ptr<UiParameterTreeModel>;

    const std::function<QString(const QString&)> translate = [context = translate_context.toUtf8()]
                                                             (const QString& source) {
      if (context.isEmpty()) {
        return source;
      }

      return QCoreApplication::translate(context.constData(), source.toUtf8().constData());
    };

    const std::function<QString(const node_t&)> translate_node = [&translate](const node_t& node) {
      std::stack<QString> label_stack;
      node_t current_node = node;
      while (!current_node->isRootNode()) {
        label_stack.push(current_node->getLabel());
        current_node = current_node->parentNode();
      }

      QString translation;
      while (!label_stack.empty()) {
        if (!translation.isEmpty()) {
          translation.push_back(QStringLiteral(" : "));
        }
        translation.push_back(translate(label_stack.top()));
        label_stack.pop();
      };

      return translation;
    };

    UiParameterSearchListModel::DataContainer datas;

    const std::function<void(const node_t&)> node_handler =
      [this, &datas, &node_handler, &translate_node](const node_t& node) mutable {
        if (!node->getKey().isEmpty()) {
          datas.emplace_back(UiParameterSearchListModel::Data{
            node->getUid(),
            node->getKey(),
            node->getLevel(),
            node.get(),
            translate_node(node),
          });
        }

        node->forEach(node_handler);
      };

    node_handler(tree_model_raw->sharedFromThis());

    return new UiParameterSearchListModel{ std::move(datas) };
  }

  auto UiParameterManager::uiImageMap() const -> const std::map<QString, QString>& {
    return ui_image_map_;
  }

  auto UiParameterManager::onThemeChanged(ThemeCategory theme) -> void {
    const auto theme_name = [theme]() {
      switch (theme) {
        case ThemeCategory::tc_light: {
          return QStringLiteral("light_theme");
        }
        case ThemeCategory::tc_dark:
        case ThemeCategory::tc_num:
        default: {
          return QStringLiteral("dark_theme");
          break;
        }
      }
    }();

    machine_tree_model_->setThemeName(theme_name);
    process_tree_model_->setThemeName(theme_name);
    material_tree_model_->setThemeName(theme_name);
    override_tree_model_->setThemeName(theme_name);
    machine_compare_tree_model_->setThemeName(theme_name);
    process_compare_tree_model_->setThemeName(theme_name);
    material_compare_tree_model_->setThemeName(theme_name);
    override_compare_tree_model_->setThemeName(theme_name);
  }

  auto UiParameterManager::onLanguageChanged(MultiLanguage language) -> void {
    std::ignore = language;
  }

  auto UiParameterManager::loadUiImageMap() -> bool {
    ui_image_map_.clear();

    const auto file_path = QStringLiteral("%1/resources/sliceconfig/%2/param/ui/image/map.json")
        .arg(QCoreApplication::applicationDirPath(),
             [] {
               switch (getKernel()->globalConst()->getEngineType()) {
                 case EngineType::ET_CURA: return QStringLiteral("cura");
                 case EngineType::ET_ORCA: return QStringLiteral("orca");
               }
               return QStringLiteral("orca");
             }());

    rapidjson::Document json_document = ReadJsonFile(file_path);
    if (json_document.HasParseError() || !json_document.IsObject()) {
      return false;
    }

    const auto json_root = json_document.GetObject();
    for (auto iter = json_root.begin(); iter != json_root.end(); ++iter) {
      const auto& json_name = iter->name;
      const auto& json_value = iter->value;
      if (!json_name.IsString() || !json_value.IsString()) {
        continue;
      }

      ui_image_map_.emplace(std::make_pair(QString::fromUtf8(json_name.GetString()),
                                           QString::fromUtf8(json_value.GetString())));
    }

    return true;
  }

  auto UiParameterManager::loadKeyLabelMap() -> bool {
    key_label_map_.clear();

    for (const auto& setting : us::SettingDef::instance().getHashItemDef()) {
      key_label_map_.emplace(std::make_pair(setting->name, setting->label));
    }

    return !key_label_map_.empty();
  }

  auto UiParameterManager::loadMachineTreeModel() -> bool {
    const auto file_info = FindReadonlyFile(MACHINE_FILE_NAME);
    if (!file_info.isFile()) {
      return false;
    }

    using Node = decltype(machine_tree_model_);

    const auto json = ReadJsonFile(file_info.absoluteFilePath());
    const auto callback = [this](const Node& node) {
      if (!node->getLabel().isEmpty()) {
        return;
      }

      const auto key = node->getKey();
      const auto iter = key_label_map_.find(key);
      node->setLabel(iter != key_label_map_.cend() ? iter->second : key);
    };

    machine_tree_model_->clearChildNodes();
    machine_compare_tree_model_->clearChildNodes();

    JsonToTreeModel(json, machine_tree_model_, callback);
    JsonToTreeModel(json, machine_compare_tree_model_, callback);

    auto model = std::const_pointer_cast<const UiParameterTreeModel>(machine_tree_model_);
    return !model->childNodes().empty();
  }

  auto UiParameterManager::loadProcessTreeModel() -> bool {
    const auto file_info = FindReadonlyFile(PROCESS_FILE_NAME);
    if (!file_info.isFile()) {
      return false;
    }

    using Node = decltype(process_tree_model_);

    const auto json = ReadJsonFile(file_info.absoluteFilePath());
    const auto level_customized_keys = GetLevelCustomizedKeys();
    const auto callback = [this, &level_customized_keys](const Node& node) {
      const auto key = node->getKey();
      if (key.isEmpty()) {
        return;
      }

      if (node->getLabel().isEmpty()) {
        const auto iter = key_label_map_.find(key);
        node->setLabel(iter != key_label_map_.cend() ? iter->second : key);
      }

      node->setLevelCustomized(level_customized_keys.find(key) != level_customized_keys.cend());
    };

    process_tree_model_->clearChildNodes();
    process_compare_tree_model_->clearChildNodes();

    JsonToTreeModel(json, process_tree_model_, callback);
    JsonToTreeModel(json, process_compare_tree_model_, callback);

    auto model = std::const_pointer_cast<const UiParameterTreeModel>(process_tree_model_);
    return !model->childNodes().empty();
  }

  auto UiParameterManager::loadMaterialTreeModel() -> bool {
    const auto file_info = FindReadonlyFile(MATERIAL_FILE_NAME);
    if (!file_info.isFile()) {
      return false;
    }

    using Node = decltype(material_tree_model_);

    const auto json = ReadJsonFile(file_info.absoluteFilePath());
    const auto callback = [this](const Node& node) {
      if (!node->getLabel().isEmpty()) {
        return;
      }

      const auto key = node->getKey();
      const auto iter = key_label_map_.find(key);
      node->setLabel(iter != key_label_map_.cend() ? iter->second : key);
    };

    material_tree_model_->clearChildNodes();
    material_compare_tree_model_->clearChildNodes();

    JsonToTreeModel(json, material_tree_model_, callback);
    JsonToTreeModel(json, material_compare_tree_model_, callback);

    auto model = std::const_pointer_cast<const UiParameterTreeModel>(material_tree_model_);
    return !model->childNodes().empty();
  }

  auto UiParameterManager::loadOverrideTreeModel() -> bool {
    using Node = decltype(override_tree_model_);

    const auto recursive_children = [](const Node& node,
                                       const std::function<void(const Node&)>& handler) {
      auto impl = [handler](const auto& impl, const auto& node, bool skip = false) -> void {
        if (!skip) {
          handler(node);
        }

        if (node && node->isForkNode()) {
          node->forEach([impl](const auto& child) {
            impl(impl, child);
          });
        }
      };

      return impl(impl, node, true);
    };

    const auto emplace_leaf = [this](const Node& fork, QString&& key) {
      auto leaf = fork->createChild();
      leaf->setKey(key);
      leaf->setLabel([this, key] {
        const auto iter = key_label_map_.find(key);
        return iter != key_label_map_.cend() ? iter->second : key;
      }());

      key_override_node_map_.emplace(std::make_pair(std::move(key), std::move(leaf)));
    };

    override_tree_model_->clearChildNodes();
    override_compare_tree_model_->clearChildNodes();

    key_override_node_map_.clear();

    switch (engine_type_) {
      case SliceFlow::EngineType::CURA: {
        // non-process nodes
        auto override_node = Node{ nullptr };
        material_tree_model_->forEach([&override_node](const Node& node, bool& break_loop) {
          if (node->getLabel() == OVERRIDE_LABEL) {
            override_node = node;
            break_loop = true;
          }
        });

        auto fork_node = Node{ nullptr };
        auto compare_fork_node = Node{ nullptr };
        recursive_children(override_node,
          [this, &fork_node, &compare_fork_node, &emplace_leaf] (const Node& node) {
            if (node->isForkNode()) {
              fork_node = override_tree_model_->createChild();
              fork_node->setLabel(node->getLabel());
              compare_fork_node = override_compare_tree_model_->createChild();
              compare_fork_node->setLabel(node->getLabel());
            } else if (node->isLeafNode() && fork_node) {
              emplace_leaf(fork_node, node->getKey());
            }
          });

        // process nodes
        fork_node = override_tree_model_->createChild();
        fork_node->setLabel(PROCESS_LABEL);
        recursive_children(process_tree_model_,
          [&fork_node, &emplace_leaf](const Node& node) {
            if (node->isLeafNode()) {
              emplace_leaf(fork_node, node->getKey());
            }
          });

        compare_fork_node = override_compare_tree_model_->createChild();
        compare_fork_node->setLabel(PROCESS_LABEL);
        recursive_children(override_compare_tree_model_,
          [&compare_fork_node, &emplace_leaf](const Node& node) {
            if (node->isLeafNode()) {
              emplace_leaf(compare_fork_node, node->getKey());
            }
          });

        break;
      }
      case SliceFlow::EngineType::OCRA: {
        override_tree_model_ = std::static_pointer_cast<Node::element_type>(
          material_tree_model_->findChildNode(QStringLiteral("/setting_overrides")));

        override_compare_tree_model_ = std::static_pointer_cast<Node::element_type>(
          material_compare_tree_model_->findChildNode(QStringLiteral("/setting_overrides")));

        using Node = decltype(override_tree_model_);
        const auto callback = [this](const Node& node) {
          if (node->isLeafNode()) {
            key_override_node_map_.emplace(
              std::make_pair(node->getKey(), std::move(node)));
          }
        };

        recursive_children(override_tree_model_, callback);
        recursive_children(override_compare_tree_model_, callback);
        break;
      }
      default: {
        break;
      }
    }

    return true;
  }

  auto UiParameterManager::syncOverrideTreeModel() -> bool {
    if (!parameter_manager_ || focused_material_name_.isEmpty()) {
      return false;
    }

    const auto* current_machine = parameter_manager_->currentMachine();
    if (!current_machine) {
      return false;
    }

    auto* const focused_material = current_machine->materialWithName(focused_material_name_);
    if (!focused_material) {
      return false;
    }

    // TODO : find orca overrode keys
    const auto overrode_keys = [this, focused_material] {
      auto keys = focused_material->getExtruderOverrideKeys();
      if (!focused_process_name_.isEmpty()) {
        keys << focused_material->getProcessOverrideKeys(focused_process_name_);
      }
      return keys;
    }();

    for (const auto& pair : key_override_node_map_) {
      const auto& key = pair.first;
      const auto node = pair.second.lock();
      node->setOverrode(false); // ensure emit changed signal when node overrode after sync
      node->setOverrode(overrode_keys.contains(key));
    }

    return true;
  }

}  // namespace creative_kernel
