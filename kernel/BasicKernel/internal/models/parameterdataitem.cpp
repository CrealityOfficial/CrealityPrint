#include "internal/models/parameterdataitem.h"

#include <set>

#include <QtQml/QQmlEngine>

#include <qtusercore/util/translateutil.h>

#include "external/data/modelgroup.h"
#include "external/interface/spaceinterface.h"
#include "external/kernel/kernel.h"
#include "external/kernel/kernelui.h"
#include "external/kernel/reuseablecache.h"

#include "internal/data/ObjectsDataManager.h"
#include "internal/models/parameterdatamodel.h"
#include "internal/models/parameterdatatool.h"
#include "internal/multi_printer/printermanager.h"
#include "internal/parameter/parameterbase.h"

namespace creative_kernel {

  namespace {

    template <typename _Type>
    auto Translate(_Type source) -> auto {
      return cx::translate(u8"ParameterComponent", source);
    }

  }  // namespace





  auto ParameterDataItem::Create(QPointer<us::USetting> default_setting,
                                 QPointer<us::USetting> modifyed_setting,
                                 QPointer<DataModel> data_model)
      -> std::unique_ptr<ParameterDataItem> {
    std::unique_ptr<ParameterDataItem> item{ nullptr };

    do {
      if (!default_setting) {
        break;
      }

      const auto key = default_setting->key();
      if (key.isEmpty()) {
        break;
      }

      if (key == QStringLiteral("sparse_infill_density")) {
        item.reset(
            new SparseInfillDensityDataItem{ default_setting, modifyed_setting, data_model });
        break;
      }

      if (key == QStringLiteral("flush_into_objects")) {
        item.reset(new FlushIntoObjectsDataItem{ default_setting, modifyed_setting, data_model });
        break;
      }

      if (key == QStringLiteral("ironing_spacing")) {
        item.reset(new IroningSpacingDataItem{ default_setting, modifyed_setting, data_model });
        break;
      }

      if (key == QStringLiteral("enable_prime_tower")) {
        item.reset(new EnablePrimeTowerDataItem{ default_setting, modifyed_setting, data_model });
        break;
      }

      if (key == QStringLiteral("tree_support_branch_diameter")) {
        item.reset(
            new TreeSupportBranchDiameterDataItem{ default_setting, modifyed_setting, data_model });
        break;
      }

      if (key == QStringLiteral("tree_support_branch_diameter_organic")) {
        item.reset(
            new TreeSupportBranchDiameterDataItem{ default_setting, modifyed_setting, data_model });
        break;
      }

      if (key == QStringLiteral("support_style")) {
        item.reset(new SupportStyleDataItem{ default_setting, modifyed_setting, data_model });
        break;
      }

      item.reset(new ParameterDataItem{ default_setting, modifyed_setting, data_model });
    } while (false);

    if (item) {
      QQmlEngine::setObjectOwnership(item.get(), QQmlEngine::ObjectOwnership::CppOwnership);
    }

    return item;
  }

  ParameterDataItem::ParameterDataItem(QPointer<us::USetting> default_setting,
                                       QPointer<us::USetting> modifyed_setting,
                                       QPointer<DataModel> data_model)
      : QObject{ nullptr }
      , default_setting_{ default_setting }
      , modifyed_setting_{ modifyed_setting }
      , data_model_{ data_model }
      , enum_list_model_{ nullptr }
      , ui_model_{ nullptr } {
    assert(default_setting_ != nullptr && u8"default_setting_ must exist");
    connect(default_setting_.data(), &us::USetting::strChanged, this, [this] {
      setModifyed(getDefaultValue() != getValue());
      updateExceededState(false);
      defaultValueChanged();
    });

    using This = ParameterDataItem;
    connect(this, &This::modifyedChanged, this, &This::partiallyModifyedChanged);
    connect(this, &This::valueChanged, this, &This::partiallyModifyedChanged);
    connect(this, &This::uiModelChanged, this, &This::onUiModelChanged);
    connect(this, &This::defaultValueChanged, this, &This::defaultIndexChanged);
    connect(this, &This::enabledChanged, this, std::bind(&This::updateExceededState, this, false));

    QMetaObject::invokeMethod(this, [this] {
      updateExceededState(data_model_ && data_model_->getDataType() == DataModel::DataType::PROFILE);
    }, Qt::ConnectionType::QueuedConnection);
  }

  ParameterDataItem::~ParameterDataItem() {
    EraseParameterMessage(this);
  }

  auto ParameterDataItem::getKey() const -> QString {
    if (!default_setting_) {
      return {};
    }

    return default_setting_->key();
  }

  auto ParameterDataItem::getLabel() const -> QString {
    if (!default_setting_) {
      return {};
    }

    return default_setting_->label();
  }

  auto ParameterDataItem::getDescription() const -> QString {
    if (!default_setting_) {
      return {};
    }

    return default_setting_->description();
  }

  auto ParameterDataItem::getUnit() const -> QString {
    if (!default_setting_) {
      return {};
    }

    return default_setting_->unit();
  }

  auto ParameterDataItem::getType() const -> QString {
    if (!default_setting_) {
      return {};
    }

    return default_setting_->type();
  }

  auto ParameterDataItem::isGlobalSettable() const -> bool {
    // if (!default_setting_) {
    //   return false;
    // }

    // return default_setting_->globalSettable();
    return true;
  }

  auto ParameterDataItem::isExtruderSettable() const -> bool {
    if (!default_setting_) {
      return false;
    }

    return default_setting_->extruderSettable();
  }

  auto ParameterDataItem::isModelGroupSettable() const -> bool {
    if (!default_setting_) {
      return false;
    }

    return default_setting_->meshGroupSettable();
  }

  auto ParameterDataItem::isModelSettable() const -> bool {
    if (!default_setting_) {
      return false;
    }

    return default_setting_->meshSettable();
  }

  auto ParameterDataItem::isEnabled() const -> bool {
    if (!default_setting_ || !data_model_) {
      return false;
    }

    const auto* define = default_setting_->def();
    if (!define) {
      return false;
    }

    const auto js_value = InvokeJsScript(define->enableStatus, data_model_);
    return !js_value.isNull() ? js_value.toBool() : true;
  }

  auto ParameterDataItem::isModifyed() const -> bool {
    return modifyed_setting_;
  }

  auto ParameterDataItem::setModifyed(bool modifyed) -> void {
    setModifyedValue(modifyed, getDefaultValue());
  }

  auto ParameterDataItem::getDefaultSetting() const -> QPointer<us::USetting> {
    return default_setting_;
  }

  auto ParameterDataItem::getModifyedSetting() const -> QPointer<us::USetting> {
    return modifyed_setting_;
  }

  auto ParameterDataItem::getDataModel() const -> QPointer<DataModel> {
    return data_model_;
  }

  auto ParameterDataItem::setModifyedValue(bool modifyed, const QString& value) -> bool {
    if (isModifyed() == modifyed || !data_model_) {
      return false;
    }

    const auto key = getKey();

    if (modifyed) {
      modifyed_setting_ = us::SettingDef::instance().create(key, value);
      data_model_->emplaceModifyedSetting(key, modifyed_setting_);
      updateExceededState(false);
    } else {
      EraseParameterMessage(modifyed_setting_.data());
      modifyed_setting_ = nullptr;
      data_model_->eraseModifyedSetting(key);
    }

    modifyedChanged();
    return true;
  }

  auto ParameterDataItem::isPartiallyModifyed() const -> bool {
    return modifyed_setting_ && modifyed_setting_->str().isEmpty();
  }

  auto ParameterDataItem::isExceeded() const -> bool {
    return isMinimumExceeded() || isMaximumExceeded();
  }

  auto ParameterDataItem::isMinimumExceeded() const -> bool {
    return minimum_exceeded_;
  }

  auto ParameterDataItem::isMaximumExceeded() const -> bool {
    return maximum_exceeded_;
  }

  auto ParameterDataItem::getValue() const -> QString {
    if (isModifyed()) {
      return CalculateFuzzyValue(getType(), getKey(), modifyed_setting_->str());
    }

    return getDefaultValue();
  }

  auto ParameterDataItem::getDefaultValue() const -> QString {
    if (!default_setting_) {
      return {};
    }

    return CalculateFuzzyValue(getType(), getKey(), default_setting_->str());
  }

  auto ParameterDataItem::getMinimumValue() const -> QString {
    if (!default_setting_) {
      return QStringLiteral("0");
    }

    const auto value_script = default_setting_->minStr();

    if (CheckJsScriptValid(value_script)) {
      const auto js_value = InvokeJsScript(value_script, data_model_);
      return !js_value.isNull() ? js_value.toString() : QStringLiteral("0");
    }

    return CalculateFuzzyValue(getType(), getKey(), value_script);
  }

  auto ParameterDataItem::getMaximumValue() const -> QString {
    if (!default_setting_) {
      return QStringLiteral("99999");
    }

    const auto value_script = default_setting_->maxStr();

    if (CheckJsScriptValid(value_script)) {
      const auto js_value = InvokeJsScript(value_script, data_model_);
      return !js_value.isNull() ? js_value.toString() : QStringLiteral("99999");
    }

    return CalculateFuzzyValue(getType(), getKey(), value_script);
  }

  auto ParameterDataItem::setValue(const QString& value) -> void {
    const auto key = getKey();
    const auto type = getType();
    const auto default_value = getDefaultValue();
    const auto current_value = getValue();
    const auto new_value = CalculateFuzzyValue(getType(), key, value);

    // 1. update 'modifyed' state
    const auto modifyed_changed = setModifyedValue(default_value != new_value, new_value);

    if (current_value == new_value) {
      return;
    }

    // 2. update value
    if (isModifyed() && !modifyed_changed) {
      modifyed_setting_->setStr(new_value);
    }

    // 3. emit signal for each type of value
    const auto is_boolean = CheckBooleanType(type, new_value);
    const auto is_enum = CheckEnumType(type, value) || enum_list_model_;
    if (is_boolean) {
      checkedChanged();
    } else if (is_enum) {
      currentIndexChanged();
    }
    valueChanged();

    // 4. update exceeded state
    updateExceededState(false);

    // 5. handle related objects
    if (data_model_) {
      // 5-1. update related parameter (to other ParameterDataItem)
      data_model_->updateAffectedSettings(key);
      // 5-2. notify related objects (to ParameterBase)
      if (is_boolean) {
        data_model_->enableChanged(key, isChecked());
      } else if (is_enum) {
        // data_model_->indexChanged(key, currentIndex());
      }
      data_model_->strChanged(key, new_value);
      // 5-3. save data (to ParameterBase)
      data_model_->save();
    }
  }

  auto ParameterDataItem::setAffectedValue(const QString& value) -> void {
    if (!default_setting_) {
      return;
    }

    // 1. update 'modifyed' status
    setModifyed(false);

    // 2. update value
    default_setting_->setStr(value);

    // 3. emit signal for each type of value
    const auto type = getType();
    if (CheckBooleanType(type, value)) {
      checkedChanged();
    } else if (CheckEnumType(type, value) || enum_list_model_) {
      currentIndexChanged();
    }
    valueChanged();
  }

  auto ParameterDataItem::isChecked() const -> bool {
    const auto checked_string = getValue();

    if (checked_string.isEmpty()) {
      return false;
    }

    return checked_string == QStringLiteral("true") || checked_string == QStringLiteral("1");
  }

  auto ParameterDataItem::setChecked(bool checked) -> void {
    const auto current_checked_string = getValue();

    auto value_as_int = false;
    auto current_checked = current_checked_string.toInt(&value_as_int) == 1;
    if (!value_as_int) {
      current_checked = current_checked_string == QStringLiteral("true");
    }

    if (current_checked == checked) {
      return;
    }

    setValue(value_as_int ? checked ? QStringLiteral("1") : QStringLiteral("0")
                          : checked ? QStringLiteral("true") : QStringLiteral("false"));
  }

  auto ParameterDataItem::getEnumListModel() const -> QPointer<EnumListModel> {
    if (!enum_list_model_) {
      enum_list_model_ = EnumListModel::Create(this);
      auto filter_model = enum_list_model_->getFilterModel();
      if (filter_model) {
        connect(filter_model.data(), &ParameterEnumFilterListModel::filterInvalidated,
                this, &ParameterDataItem::onEnumFilterInvalidated);
      }
    }

    return enum_list_model_.get();
  }

  auto ParameterDataItem::getEnumListModelObject() const -> QAbstractListModel* {
    return getEnumListModel().data();
  }

  auto ParameterDataItem::getDefaultIndex() const -> int {
    const auto default_value = getDefaultValue();

    const auto& datas = enum_list_model_->rawData();
    for (size_t index = 0; index < datas.size(); ++index) {
      const auto& data = datas.at(index);
      if (data.uid == default_value) {
        return static_cast<int>(index);
      }
    }

    return 0;
  }

  auto ParameterDataItem::getCurrentIndex() const -> int {
    if (modifyed_setting_) {
      return modifyed_setting_->enumIndex();
    }

    if (default_setting_) {
      return default_setting_->enumIndex();
    }

    return 0;
  }

  auto ParameterDataItem::setCurrentIndex(int index) -> void {
    if (!enum_list_model_) {
      return;
    }

    const auto& enums = enum_list_model_->rawData();
    if (index < 0 || static_cast<size_t>(index) > enums.size()) {
      return;
    }

    setValue(enums.at(index).uid);
  }

  auto ParameterDataItem::resetValue() -> void {
    setValue(getDefaultValue());
  }

  auto ParameterDataItem::updateExceededState(bool block_signal) -> void {
    const auto old_minimum_exceeded = minimum_exceeded_;
    const auto old_maximum_exceeded = maximum_exceeded_;
    const auto old_exceeded         = old_minimum_exceeded || old_maximum_exceeded;

    minimum_exceeded_ = false;
    maximum_exceeded_ = false;

    const auto type                 = getType();
    const auto value_string         = getValue();
    const auto minimum_value_string = getMinimumValue();
    const auto maximum_value_string = getMaximumValue();

    if (!isEnabled() || value_string.isEmpty()) {
      // do nothing
    } else if (CheckIntegerType(type, value_string)) {
      const auto value = value_string.toInt();

      if (!minimum_value_string.isEmpty()) {
        const auto minimum_value = minimum_value_string.toInt();
        minimum_exceeded_        = value < minimum_value;
      }

      if (!minimum_value_string.isEmpty()) {
        const auto maximum_value = maximum_value_string.toInt();
        maximum_exceeded_        = value > maximum_value;
      }

    } else if (CheckFloatType(type, value_string)) {
      const auto value = value_string.toFloat();

      if (!minimum_value_string.isEmpty()) {
        const auto minimum_value = minimum_value_string.toFloat();
        minimum_exceeded_        = value < minimum_value;
      }

      if (!minimum_value_string.isEmpty()) {
        const auto maximum_value = maximum_value_string.toFloat();
        maximum_exceeded_        = value > maximum_value;
      }
    } else if (CheckPercentType(type, value_string)) {
      static const auto percent_to_float = [](QString percent) {
        return percent.replace(QStringLiteral("%"), QStringLiteral("")).toFloat();
      };

      const auto value = percent_to_float(value_string);

      if (!minimum_value_string.isEmpty()) {
        const auto minimum_value = percent_to_float(minimum_value_string);
        minimum_exceeded_        = value < minimum_value;
      }

      if (!minimum_value_string.isEmpty()) {
        const auto maximum_value = percent_to_float(maximum_value_string);
        maximum_exceeded_        = value > maximum_value;
      }
    }

    const auto new_minimum_exceeded = minimum_exceeded_;
    const auto new_maximum_exceeded = maximum_exceeded_;
    const auto new_exceeded         = new_minimum_exceeded || new_maximum_exceeded;

    if (block_signal) {
      return;
    }

    if (old_minimum_exceeded != new_minimum_exceeded) {
      minimumExceededChanged();
    }

    if (old_maximum_exceeded != new_maximum_exceeded) {
      maximumExceededChanged();
    }

    if (old_exceeded != new_exceeded) {
      exceededChanged();
    }

    if (!data_model_ || !data_model_->modifyed_settings_) {
      return;
    }

    const auto  modifyed_settings       = data_model_->modifyed_settings_;
    const auto  message_owner_key_bytes = getMessagePropertyKey().toUtf8();
    const auto* message_owner_key       = message_owner_key_bytes.constData();
    auto        message_owner_variant   = modifyed_settings->property(message_owner_key);
    auto*       message_owner =
        message_owner_variant.isValid() ? message_owner_variant.value<QObject*>() : nullptr;

    // update exceeded message
    if (isExceeded() && ui_model_) {
      auto message = createMessage();
      if (message) {
        message->setTreeNode(ui_model_);
        message->setMinimumExceeded(minimum_exceeded_);

        if (!message_owner) {
          message_owner = new QObject{ modifyed_settings.data() };
          modifyed_settings->setProperty(message_owner_key, QVariant::fromValue(message_owner));
        }
        message->setOwner(message_owner);

        auto* settings_owner = modifyed_settings->parent();
        message->setModel(dynamic_cast<ModelN*>(settings_owner));
        message->setModelGroup(dynamic_cast<ModelGroup*>(settings_owner));

        EmplaceParameterMessage(message->getOwner(), std::move(message));
      }
    } else if (message_owner) {
      EraseParameterMessage(message_owner);
    }
  }

  auto ParameterDataItem::createMessage() const -> std::unique_ptr<ExceededMessage> {
    auto* message = CreateParameterExceededMessage(getKey()).data();
    return std::unique_ptr<ExceededMessage>{ message };
  }

  auto ParameterDataItem::getMessagePropertyKey() const -> QString {
    return QStringLiteral("_parameter_message_owner_%1").arg(getKey());
  }

  auto ParameterDataItem::getUiModel() const -> QObject* {
    return ui_model_;
  }

  auto ParameterDataItem::setUiModel(QObject* ui_model) -> void {
    if (ui_model_ != ui_model) {
      ui_model_ = ui_model;
      uiModelChanged();
    }
  }

  auto ParameterDataItem::onUiModelChanged() -> void {
    if (!data_model_ || !data_model_->modifyed_settings_) {
      return;
    }

    auto* modifyed_settings_parent = data_model_->modifyed_settings_->parent();

    // from profile
    if (dynamic_cast<ParameterBase*>(modifyed_settings_parent)) {
      updateExceededState(false);
      return;
    }

    // from object
    if (dynamic_cast<ModelN*>(modifyed_settings_parent)) {
      // do nothing
      return;
    }
  }

  auto ParameterDataItem::onEnumFilterInvalidated() -> void {
    if (!enum_list_model_) {
      return;
    }

    const auto filter_model = enum_list_model_->getFilterModel();

    auto default_index = getDefaultIndex();
    if (!filter_model->filterAcceptsRow(default_index, filter_model->index(0, 0))) {
      default_index = filter_model->mapIndexToSource(0);
    }

    setCurrentIndex(default_index);
  }





  SparseInfillDensityDataItem::SparseInfillDensityDataItem(QPointer<us::USetting> default_setting,
                                                           QPointer<us::USetting> modifyed_setting,
                                                           QPointer<DataModel> data_model)
      : ParameterDataItem{ default_setting, modifyed_setting, data_model } {
    connect(this, &ParameterDataItem::valueChanged,
            this, &SparseInfillDensityDataItem::updateVaildState);

    if (!data_model_) {
      return;
    }

    sparse_infill_pattern_ =
        data_model_->findOrMakeDataItem(QStringLiteral("sparse_infill_pattern"));
    if (!sparse_infill_pattern_) {
      return;
    }

    connect(sparse_infill_pattern_.data(), &ParameterDataItem::valueChanged,
            this, &SparseInfillDensityDataItem::updateVaildState);
  }

  auto SparseInfillDensityDataItem::getMessageText() const -> QString {
    constexpr auto SOURCE = "%1 infill pattern doesn't support 100% density.\n"
                            "Switch to rectilinear pattern?\n"
                            "Yes - switch to rectilinear pattern automaticlly\n"
                            "No - reset density to default non 100% value automaticlly";

    if (!sparse_infill_pattern_) {
      return Translate(SOURCE).arg(QStringLiteral(""));
    }

    const auto list = sparse_infill_pattern_->getEnumListModel();
    const auto value = list->load(sparse_infill_pattern_->getValue());
    return Translate(SOURCE).arg(Translate(value.name));
  }

  auto SparseInfillDensityDataItem::isAcceptable() const -> bool {
    return true;
  }

  auto SparseInfillDensityDataItem::isCancelable() const -> bool {
    return true;
  }

  auto SparseInfillDensityDataItem::isIgnoreable() const -> bool {
    return false;
  }

  auto SparseInfillDensityDataItem::onAccepted() -> void {
    if (sparse_infill_pattern_) {
      sparse_infill_pattern_->setValue("zig-zag");
    }
  }

  auto SparseInfillDensityDataItem::onCanceled() -> void {
    setValue(getDefaultValue());
  }

  auto SparseInfillDensityDataItem::updateVaildState() -> void {
    auto successed = false;
    auto value = getValue().remove(QStringLiteral("%")).toFloat(&successed);
    if (!successed || value < 100.f) {
      return;
    }

    if (!sparse_infill_pattern_) {
      return;
    }

    static const std::set<QString> TARGET_OPTIONS{
      QStringLiteral("line"),
      QStringLiteral("cubic"),
      QStringLiteral("triangles"),
      QStringLiteral("tri-hexagon"),
      QStringLiteral("gyroid"),
      QStringLiteral("honeycomb"),
      QStringLiteral("adaptivecubic"),
      QStringLiteral("3dhoneycomb"),
      QStringLiteral("lightning"),
      QStringLiteral("supportcubic"),
    };

    const auto option = sparse_infill_pattern_->getValue();
    if (TARGET_OPTIONS.find(option) == TARGET_OPTIONS.cend()) {
      return;
    }

    getKernelUI()->requestQmlDialog(this, QStringLiteral("messageDlg"));
  }





  auto FlushIntoObjectsDataItem::isGlobalSettable() const -> bool {
    return false;
  }





  EnablePrimeTowerDataItem::EnablePrimeTowerDataItem(QPointer<us::USetting> default_setting,
                                                     QPointer<us::USetting> modifyed_setting,
                                                     QPointer<DataModel> data_model)
      : ParameterDataItem{ default_setting, modifyed_setting, data_model } {
    if (!data_model || data_model->getDataType() != DataModel::DataType::PROFILE) {
      return;
    }

    enable_support_ = data_model->findOrMakeDataItem(QStringLiteral("enable_support"));
    support_filament_ = data_model->findOrMakeDataItem(QStringLiteral("support_filament"));
    support_interface_filament_ =
        data_model->findOrMakeDataItem(QStringLiteral("support_interface_filament"));

    for (auto& setting : { enable_support_, support_filament_, support_interface_filament_ }) {
      connect(setting.data(), &ParameterDataItem::valueChanged,
              this, std::bind(&EnablePrimeTowerDataItem::updateExceededState, this, false));
    }

    auto* kernel = getKernelSafely();
    if (!kernel) {
      return;
    }

    auto* reuseable_cache = kernel->reuseableCache();
    if (!reuseable_cache) {
      return;
    }

    auto* panel_manager = reuseable_cache->getPrinterMangager();
    if (!panel_manager) {
      return;
    }

    auto* model_space = kernel->modelSpace();
    if (!model_space) {
      return;
    }

    traceSpace(this);
    model_space_ = model_space;

    connect(panel_manager, &PrinterMangager::signalDidSelectPrinter,
            this, &EnablePrimeTowerDataItem::onCurrentPanelChanged);
    onCurrentPanelChanged(panel_manager->getSelectedPrinter());
  }

  EnablePrimeTowerDataItem::~EnablePrimeTowerDataItem() {
    if (model_space_) {
      unTraceSpace(this);
    }
  }

  auto EnablePrimeTowerDataItem::updateExceededState(bool block_signal) -> void {
    auto exceeded = false;

    do {
      if (!isChecked()) {
        break;
      }

      const auto model_group_pair_list = generlateModelGroupPairList();
      if (model_group_pair_list.empty()) {
        break;
      }

      const auto premise = generlateExceededPremise(model_group_pair_list);
      if (!premise.multi_color) {
        break;
      }

      // get global layer height value from current process data model
      auto data_model = getDataModel();
      if (!data_model) {
        continue;
      }

      static const auto LAYER_HEIGHT_KEY{ QStringLiteral("layer_height") };
      auto data_item = data_model->findOrMakeDataItem(LAYER_HEIGHT_KEY);
      if (!data_item) {
        continue;
      }

      const auto global_value = data_item->getValue();

      QString first_value{};
      ModelGroup* different_group{ nullptr };
      for (const auto& pair : model_group_pair_list) {
        // check if the model's adaptive layer height option is opened
        auto* model = pair.first;
        if (!model) {
          continue;
        }

        if (premise.multi_group && !model->layerHeightProfile().empty()) {
          message_state_ = Message::State::ADAPTIVE_LAYER_HEIGHT_OPENED;
          layer_height_model_group_ = nullptr;
          layer_height_ui_model_ = nullptr;
          exceeded = true;
          break;
        }

        // check if the group's layer height value is different with others
        auto* group = pair.second;
        if (!group) {
          continue;
        }

        auto* settings = group->setting();
        if (!settings) {
          continue;
        }

        auto* setting = settings->findSetting(LAYER_HEIGHT_KEY);
        const auto value = setting ? setting->str() : global_value;
        if (first_value.isEmpty()) {
          first_value = value;
        }

        if (!different_group && value != global_value) {
          different_group = group;
        }

        if (value != first_value) {
          message_state_ = Message::State::DIFFERENT_LAYER_HEIGHT;
          layer_height_model_group_ = different_group;
          layer_height_ui_model_ = data_item->getUiModel();
          exceeded = true;
          break;
        }
      }

    } while (false);

    if (isExceeded() != exceeded) {
      minimum_exceeded_ = exceeded;
      maximum_exceeded_ = exceeded;
      if (!block_signal) {
        exceededChanged();
      }
    }

    if (!block_signal) {
      if (isExceeded()) {
        EmplaceParameterMessage(this, createMessage());
      } else {
        EraseParameterMessage(this);
      }
    }
  }

  auto EnablePrimeTowerDataItem::createMessage() const -> std::unique_ptr<ExceededMessage> {
    if (!isExceeded()) {
      return nullptr;
    }

    return std::make_unique<Message>(
        getKey(), message_state_, layer_height_model_group_, layer_height_ui_model_);
  }

  auto EnablePrimeTowerDataItem::onModelGroupAdded(ModelGroup* group) -> void {
    if (group) {
      auto slot = std::bind(&EnablePrimeTowerDataItem::updateExceededState, this, false);

      connect(group, &ModelGroup::colorsDataChanged,
              this, slot, Qt::ConnectionType::UniqueConnection);

      connect(group, &ModelGroup::adaptiveLayerDataChanged,
              this, slot, Qt::ConnectionType::UniqueConnection);

      auto* settings = group->setting();
      if (settings) {
        connect(settings, &us::USettings::settingsChanged,
                this, slot, Qt::ConnectionType::UniqueConnection);

        connect(settings, &us::USettings::settingValueChanged,
                this, slot, Qt::ConnectionType::UniqueConnection);
      }

      onModelGroupModified(group, {}, group->models());
    }
  }

  auto EnablePrimeTowerDataItem::onModelGroupRemoved(ModelGroup* group) -> void {
    if (group) {
      group->disconnect(this);

      auto* settings = group->setting();
      if (settings) {
        settings->disconnect(this);
      }

      onModelGroupModified(group,  group->models(), {});
    }
  }

  auto EnablePrimeTowerDataItem::onModelGroupModified(ModelGroup*           group,
                                                      const QList<ModelN*>& removed_models,
                                                      const QList<ModelN*>& added_models) -> void {
    for (auto* model : removed_models) {
      if (model) {
        model->disconnect(this);

        auto* settings = model->setting();
        if (settings) {
          settings->disconnect(this);
        }
      }
    }

    auto slot = std::bind(&EnablePrimeTowerDataItem::updateExceededState, this, false);
    for (auto* model : added_models) {
      if (model) {
        connect(model, &ModelN::defaultColorIndexChanged,
                this, slot, Qt::ConnectionType::UniqueConnection);

        auto* settings = model->setting();
        if (settings) {
          connect(settings, &us::USettings::settingsChanged,
                  this, slot, Qt::ConnectionType::UniqueConnection);

          connect(settings, &us::USettings::settingValueChanged,
                  this, slot, Qt::ConnectionType::UniqueConnection);
        }
      }
    }

    updateExceededState(false);
  }

  auto EnablePrimeTowerDataItem::onCurrentPanelChanged(Printer* panel) -> void {
    if (current_panel_ == panel) {
      return;
    }

    if (current_panel_) {
      current_panel_->disconnect(this);
    }

    current_panel_ = panel;

    if (current_panel_) {
      connect(current_panel_.data(), &Printer::signalModelsInsideChange,
              this, std::bind(&EnablePrimeTowerDataItem::updateExceededState, this, false),
              Qt::ConnectionType::QueuedConnection);
    }

    QMetaObject::invokeMethod(this, [this] {
      updateExceededState(false);
    }, Qt::ConnectionType::QueuedConnection);
  }

  auto EnablePrimeTowerDataItem::generlateModelGroupPairList() const -> ModelGroupPairList {
    if (!current_panel_) {
      return {};
    }

    auto* kernel = getKernel();
    if (!kernel) {
      return {};
    }

    auto* objects_manager = kernel->objectsDataManager();
    if (!objects_manager) {
      return {};
    }

    std::list<std::pair<ModelN*, ModelGroup*>> model_group_pair_list{};

    const auto group_list = objects_manager->generlatePrinterModelGroupList(current_panel_.data());
    for (const auto& group : group_list) {
      if (!group) {
        continue;
      }

      for (auto* model : group->models()) {
        if (!model) {
          continue;
        }

        model_group_pair_list.emplace_back(model, group);
      }
    }

    return model_group_pair_list;
  }

  auto EnablePrimeTowerDataItem::generlateExceededPremise(const ModelGroupPairList& pair_list) const
      -> ExceededPremise {
    if (pair_list.empty()) {
      return {};
    }

    ExceededPremise premise{};

    {
      auto support_enabled = isSupportEnabled();
      auto support_color_index = getSupportFilament();
      auto support_interface_color_index = getSupportInterfaceFilament();
      if (support_enabled && support_color_index != support_interface_color_index) {
        premise.multi_color = true;
      }
    }

    int first_model_color_index = -1;
    ModelGroup* first_group = nullptr;
    for (const auto& pair : pair_list) {
      auto* model = pair.first;
      auto* group = pair.second;
      if (!model || !group) {
        continue;
      }

      // check if any objects use multiple colors or have a different color index than others
      if (!premise.multi_color) {
        do {
          if (model->hasColors()) {
            premise.multi_color = true;
            break;
          }

          auto* settings = group->setting();
          auto support_enabled = checkSupportEnabled(settings);
          auto support_color_index = checkSupportFilament(settings);
          auto support_interface_color_index = checkSupportInterfaceFilament(settings);
          if (support_enabled && support_color_index != support_interface_color_index) {
            premise.multi_color = true;
            break;
          }

          if (first_model_color_index == -1) {
            first_model_color_index = model->defaultColorIndex();
            if (support_enabled && first_model_color_index != support_color_index) {
              premise.multi_color = true;
            }
            break;
          }

          if (first_model_color_index != model->defaultColorIndex()) {
            premise.multi_color = true;
            break;
          }
        } while (false);
      }

      // check if any model come from the different group than others
      if (!premise.multi_group) {
        do {
          if (!first_group) {
            first_group = group;
            break;
          }

          if (first_group != group) {
            premise.multi_group = true;
            break;
          }
        } while (false);
      }

      if (premise.multi_color && premise.multi_group) {
        break;
      }
    }

    return premise;
  }

  auto EnablePrimeTowerDataItem::isSupportEnabled() const -> bool {
    return enable_support_ && enable_support_->isChecked();
  }

  auto EnablePrimeTowerDataItem::checkSupportEnabled(us::USettings* settings) const -> bool {
    if (!settings) {
      return isSupportEnabled();
    }

    auto* setting = settings->findSetting(QStringLiteral("enable_support"));
    if (!setting) {
      return isSupportEnabled();
    }

    const auto value = setting->str();
    return value == QStringLiteral("true") || value == QStringLiteral("1");
  }

  auto EnablePrimeTowerDataItem::getSupportFilament() const -> int {
    return support_filament_ ? std::max(0, support_filament_->getValue().toInt() - 1) : 0;
  }

  auto EnablePrimeTowerDataItem::checkSupportFilament(us::USettings* settings) const -> int {
    if (!settings) {
      return getSupportFilament();
    }

    auto* setting = settings->findSetting(QStringLiteral("support_filament"));
    if (!setting) {
      return getSupportFilament();
    }

    return std::max(0, setting->str().toInt() - 1);
  }

  auto EnablePrimeTowerDataItem::getSupportInterfaceFilament() const -> int {
    return support_interface_filament_
               ? std::max(0, support_interface_filament_->getValue().toInt() - 1)
               : 0;
  }

  auto EnablePrimeTowerDataItem::checkSupportInterfaceFilament(us::USettings* settings) const
      -> int {
    if (!settings) {
      return getSupportInterfaceFilament();
    }

    auto* setting = settings->findSetting(QStringLiteral("support_interface_filament"));
    if (!setting) {
      return getSupportInterfaceFilament();
    }

    return std::max(0, setting->str().toInt() - 1);
  }





  IroningSpacingDataItem::IroningSpacingDataItem(QPointer<us::USetting> default_setting,
                                                 QPointer<us::USetting> modifyed_setting,
                                                 QPointer<DataModel> data_model)
      : ParameterDataItem{ default_setting, modifyed_setting, data_model } {
    connect(this, &ParameterDataItem::minimumExceededChanged,
            this, &IroningSpacingDataItem::onMinimumExceededChanged);

    ironing_type_ = data_model_->findOrMakeDataItem(QStringLiteral("ironing_type"));
    if (!ironing_type_) {
      return;
    }
  }

  auto IroningSpacingDataItem::getMessageText() const -> QString {
    constexpr auto SOURCE = "%1 too small. It has been reset to %2.";
    return Translate(SOURCE).arg(Translate(getLabel()),  getMinimumValue());
  }

  auto IroningSpacingDataItem::isAcceptable() const -> bool {
    return true;
  }

  auto IroningSpacingDataItem::isCancelable() const -> bool {
    return false;
  }

  auto IroningSpacingDataItem::isIgnoreable() const -> bool {
    return false;
  }

  auto IroningSpacingDataItem::onAccepted() -> void {
    setValue(getMinimumValue());
  }

  auto IroningSpacingDataItem::onMinimumExceededChanged() -> void {
    if (!isMinimumExceeded()) {
      return;
    }

    if (!ironing_type_) {
      return;
    }

    if (ironing_type_->getValue() != QStringLiteral("solid")) {
      return;
    }

    getKernelUI()->requestQmlDialog(this, QStringLiteral("messageDlg"));
  }





  TreeSupportBranchDiameterDataItem::TreeSupportBranchDiameterDataItem(
      QPointer<us::USetting> default_setting,
      QPointer<us::USetting> modifyed_setting,
      QPointer<DataModel>    data_model)
      : ParameterDataItem{ default_setting, modifyed_setting, data_model } {
    if (data_model_) {
      tip_diameter_ = data_model_->findOrMakeDataItem(QStringLiteral("tree_support_tip_diameter"));
    }
  }

  auto TreeSupportBranchDiameterDataItem::createMessage() const
      -> std::unique_ptr<ExceededMessage> {
    if (!isEnabled()) {
      return nullptr;
    }

    auto this_value = getValue().toFloat();
    auto tip_diameter_value = tip_diameter_ ? tip_diameter_->getValue().toFloat() : 0.f;
    auto small_than_tip_diameter = this_value < tip_diameter_value;
    auto small_than_minimum_value = this_value < 1.f;
    return std::make_unique<TreeSupportBranchDiameterExceededMessage>(
        getKey(), small_than_tip_diameter, small_than_minimum_value);
  }





  SupportStyleDataItem::SupportStyleDataItem(QPointer<us::USetting> default_setting,
                                             QPointer<us::USetting> modifyed_setting,
                                             QPointer<DataModel> data_model)
      : ParameterDataItem{ default_setting, modifyed_setting, data_model } {
    if (!data_model || data_model->getDataType() != DataModel::DataType::PROFILE) {
      return;
    }

    enable_support_ = data_model_->findOrMakeDataItem(QStringLiteral("enable_support"));
    support_type_ = data_model_->findOrMakeDataItem(QStringLiteral("support_type"));

    auto* kernel = getKernelSafely();
    if (!kernel) {
      return;
    }

    auto* reuseable_cache = kernel->reuseableCache();
    if (!reuseable_cache) {
      return;
    }

    auto* panel_manager = reuseable_cache->getPrinterMangager();
    if (!panel_manager) {
      return;
    }

    auto* model_space = kernel->modelSpace();
    if (!model_space) {
      return;
    }

    traceSpace(this);
    model_space_ = model_space;

    connect(panel_manager, &PrinterMangager::signalDidSelectPrinter,
            this, &SupportStyleDataItem::onCurrentPanelChanged);
    onCurrentPanelChanged(panel_manager->getSelectedPrinter());
  }

  SupportStyleDataItem::~SupportStyleDataItem() {
    if (model_space_) {
      unTraceSpace(this);
    }
  }

  auto SupportStyleDataItem::updateExceededState(bool block_signal) -> void {
    auto exceeded = false;

    do {
      if (!current_panel_) {
        break;
      }

      const auto check_group_exceeded = [this](ModelGroup* group, us::USettings* settings) -> bool {
        return checkSupportEnabled(settings) &&
               checkTreeSupportTypeSelected(settings) &&
               checkOrganicTreeStyleSelected(settings) &&
               !group->layerHeightProfile().empty();
      };

      for (auto* model_group : current_panel_->modelGroupsInside()) {
        if (!model_group) {
          continue;
        }

        if (check_group_exceeded(model_group, model_group->setting())) {
          const auto global_exceeded =
              isSupportEnabled() && isTreeSupportTypeSelected() && isOrganicTreeStyleSelected();
          exceeded_model_group_ = global_exceeded ? nullptr : model_group;
          exceeded = true;
          break;
        }
      }
    } while (false);

    if (isExceeded() != exceeded) {
      minimum_exceeded_ = exceeded;
      maximum_exceeded_ = exceeded;
      if (!block_signal) {
        exceededChanged();
      }
    }

    if (!block_signal) {
      if (isExceeded()) {
        EmplaceParameterMessage(this, createMessage());
      } else {
        EraseParameterMessage(this);
      }
    }
  }

  auto SupportStyleDataItem::createMessage() const -> std::unique_ptr<ExceededMessage> {
    if (!isExceeded()) {
      return nullptr;
    }

    auto message = std::make_unique<Message>(getKey());
    message->setModelGroup(exceeded_model_group_);
    message->setTreeNode(getUiModel());
    return message;
  }

  auto SupportStyleDataItem::onCurrentPanelChanged(Printer* panel) -> void {
    if (current_panel_ == panel) {
      return;
    }

    if (current_panel_) {
      current_panel_->disconnect(this);
    }

    current_panel_ = panel;

    if (current_panel_) {
      connect(current_panel_.data(), &Printer::signalModelsInsideChange,
              this, std::bind(&SupportStyleDataItem::updateExceededState, this, false),
              Qt::ConnectionType::QueuedConnection);
    }

    QMetaObject::invokeMethod(this, [this] {
      updateExceededState(false);
    }, Qt::ConnectionType::QueuedConnection);
  }

  auto SupportStyleDataItem::onModelGroupAdded(ModelGroup* group) -> void {
    if (group) {
      auto slot = std::bind(&SupportStyleDataItem::updateExceededState, this, false);

      connect(group, &ModelGroup::colorsDataChanged,
              this, slot, Qt::ConnectionType::UniqueConnection);

      connect(group, &ModelGroup::adaptiveLayerDataChanged,
              this, slot, Qt::ConnectionType::UniqueConnection);

      auto* settings = group->setting();
      if (settings) {
        connect(settings, &us::USettings::settingsChanged,
                this, slot, Qt::ConnectionType::UniqueConnection);

        connect(settings, &us::USettings::settingValueChanged,
                this, slot, Qt::ConnectionType::UniqueConnection);
      }
    }
  }

  auto SupportStyleDataItem::onModelGroupRemoved(ModelGroup* group) -> void {
    if (group) {
      group->disconnect(this);

      auto* settings = group->setting();
      if (settings) {
        settings->disconnect(this);
      }
    }
  }

  auto SupportStyleDataItem::isSupportEnabled() const -> bool {
    if (!enable_support_) {
      return false;
    }

    return enable_support_->isChecked();
  }

  auto SupportStyleDataItem::checkSupportEnabled(us::USettings* settings) const -> bool {
    if (settings) {
      auto setting = settings->findSetting(QStringLiteral("enable_support"));
      if (setting) {
        const auto value = setting->str();
        return value == QStringLiteral("true") || value == QStringLiteral("1");
      }
    }

    return isSupportEnabled();
  }

  auto SupportStyleDataItem::isTreeSupportTypeSelected() const -> bool {
    if (!support_type_) {
      return false;
    }

    const auto value = support_type_->getValue();
    return value == QStringLiteral("tree(auto)") || value == QStringLiteral("tree(manual)");
  }

  auto SupportStyleDataItem::checkTreeSupportTypeSelected(us::USettings* settings) const -> bool {
    if (settings) {
      auto setting = settings->findSetting(QStringLiteral("support_type"));
      if (setting) {
        const auto value = setting->str();
        return value == QStringLiteral("tree(auto)") || value == QStringLiteral("tree(manual)");
      }
    }

    return isTreeSupportTypeSelected();
  }

  auto SupportStyleDataItem::isOrganicTreeStyleSelected() const -> bool {
    const auto value = getValue();
    return value == QStringLiteral("default") || value == QStringLiteral("organic");
  }

  auto SupportStyleDataItem::checkOrganicTreeStyleSelected(us::USettings* settings) const -> bool {
    if (settings) {
      auto setting = settings->findSetting(getKey());
      if (setting) {
        const auto value = setting->str();
        return value == QStringLiteral("default") || value == QStringLiteral("organic");
      }
    }

    return isOrganicTreeStyleSelected();
  }

}  // namespace creative_kernel
