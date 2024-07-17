#include "internal/models/parameterdataitem.h"

#include <set>

#include <QtCore/QCoreApplication>
#include <QtQml/QQmlEngine>

#include "external/kernel/kernelui.h"

#include "internal/models/parameterdatamodel.h"
#include "internal/models/parameterdatatool.h"
#include "internal/parameter/parameterbase.h"

namespace creative_kernel {

  namespace {

    auto Translate(const char* source) -> QString {
      return QCoreApplication::translate("ParameterComponent", source);
    };

    auto Translate(const QString source) -> QString {
      return Translate(source.toUtf8().constData());
    };

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

    updateExceededState(true);
  }

  ParameterDataItem::~ParameterDataItem() {
    MessageContext::Instance().eraseMessage(this);
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

  auto ParameterDataItem::isProcessSettable() const -> bool {
    return true;
  }

  auto ParameterDataItem::isMeshSettable() const -> bool {
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
      MessageContext::Instance().eraseMessage(modifyed_setting_.data());
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

  auto ParameterDataItem::updateExceededState(bool initialize) -> void {
    const auto old_minimum_exceeded = minimum_exceeded_;
    const auto old_maximum_exceeded = maximum_exceeded_;
    const auto old_exceeded = old_minimum_exceeded || old_maximum_exceeded;

    minimum_exceeded_ = false;
    maximum_exceeded_ = false;

    const auto type = getType();
    const auto value_string = getValue();
    const auto minimum_value_string = getMinimumValue();
    const auto maximum_value_string = getMaximumValue();

    if (CheckIntegerType(type, value_string)) {
      const auto value = value_string.toInt();

      if (!minimum_value_string.isEmpty()) {
        const auto minimum_value = minimum_value_string.toInt();
        minimum_exceeded_ = value < minimum_value;
      }

      if (!minimum_value_string.isEmpty()) {
        const auto maximum_value = maximum_value_string.toInt();
        maximum_exceeded_ = value > maximum_value;
      }

    } else if (CheckFloatType(type, value_string)) {
      const auto value = value_string.toFloat();

      if (!minimum_value_string.isEmpty()) {
        const auto minimum_value = minimum_value_string.toFloat();
        minimum_exceeded_ = value < minimum_value;
      }

      if (!minimum_value_string.isEmpty()) {
        const auto maximum_value = maximum_value_string.toFloat();
        maximum_exceeded_ = value > maximum_value;
      }
    } else if (CheckPercentType(type, value_string)) {
      static const auto percent_to_float = [](QString percent) {
        return percent.replace(QStringLiteral("%"), QStringLiteral("")).toFloat();
      };

      const auto value = percent_to_float(value_string);

      if (!minimum_value_string.isEmpty()) {
        const auto minimum_value = percent_to_float(minimum_value_string);
        minimum_exceeded_ = value < minimum_value;
      }

      if (!minimum_value_string.isEmpty()) {
        const auto maximum_value = percent_to_float(maximum_value_string);
        maximum_exceeded_ = value > maximum_value;
      }
    }

    const auto new_minimum_exceeded = minimum_exceeded_;
    const auto new_maximum_exceeded = maximum_exceeded_;
    const auto new_exceeded = new_minimum_exceeded || new_maximum_exceeded;

    if (initialize) {
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

    // update exceeded message
    if (isExceeded() && ui_model_) {
      std::unique_ptr<MessageContext::Message> message{ nullptr };
      message.reset(CreateParameterExceededMessage(getKey()).data());
      if (message) {
        message->setTreeNode(ui_model_);
        message->setMinimumExceeded(minimum_exceeded_);
        if (data_model_ && data_model_->modifyed_settings_) {
          message->setModel(dynamic_cast<ModelN*>(data_model_->modifyed_settings_->parent()));
        }
        auto* owner = isModifyed() ? modifyed_setting_.data() : static_cast<QObject*>(this);
        MessageContext::Instance().emplaceMessage(owner, std::move(message));
      }
    } else {
      MessageContext::Instance().eraseMessage(modifyed_setting_.data());
      MessageContext::Instance().eraseMessage(this);
    }
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
      default_index = filter_model->mapToSource(0);
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

  auto SparseInfillDensityDataItem::accept() -> void {
    if (sparse_infill_pattern_) {
      sparse_infill_pattern_->setValue("zig-zag");
    }
  }

  auto SparseInfillDensityDataItem::cancel() -> void {
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





  auto FlushIntoObjectsDataItem::isProcessSettable() const -> bool {
    return false;
  }

}  // namespace creative_kernel
