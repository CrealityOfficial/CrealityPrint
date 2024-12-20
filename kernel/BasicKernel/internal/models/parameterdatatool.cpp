#include "internal/models/parameterdatatool.h"

#include <map>
#include <set>

#include <QtCore/QRegularExpression>

#include "external/data/modelgroup.h"
#include "external/data/modeln.h"
#include "external/kernel/kernel.h"
#include "external/kernel/kernelui.h"
#include "external/kernel/reuseablecache.h"

#include "internal/models/parameterdatamodel.h"
#include "internal/multi_printer/printer.h"
#include "internal/multi_printer/printermanager.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printextruder.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printmaterial.h"
#include "internal/parameter/printprofile.h"
#include "internal/parameter/uiparametermanager.h"

namespace creative_kernel {

  auto CheckBooleanType(const QString& type, const QString& value) -> bool {
    // filter arrays
    if (type == QStringLiteral("coBools")) {
      return !value.contains(QStringLiteral(","));
    }

    static const std::set<QString> TYPES{
      QStringLiteral("bool"),
      QStringLiteral("coBool"),
    };

    return TYPES.find(type) != TYPES.cend();
  }

  auto CheckEnumType(const QString& type, const QString& value) -> bool {
    // filter arrays
    if (type == QStringLiteral("coEnums")) {
      return !value.contains(QStringLiteral(","));
    }

    static const std::set<QString> TYPES{
      QStringLiteral("enum"),
      QStringLiteral("coEnum"),
    };

    return TYPES.find(type) != TYPES.cend();
  }

  auto CheckIntegerType(const QString& type, const QString& value) -> bool {
    // filter arrays
    if (type == QStringLiteral("coInts")) {
      return !value.contains(QStringLiteral(","));
    }

    static const std::set<QString> TYPES{
      QStringLiteral("int"),
      QStringLiteral("coInt"),
    };

    return TYPES.find(type) != TYPES.cend();
  }

  auto CheckFloatType(const QString& type, const QString& value) -> bool {
    // filter arrays
    if (type == QStringLiteral("coFloats")) {
      return !value.contains(QStringLiteral(","));
    }

    // filter percentages
    if (type == QStringLiteral("coFloatOrPercent")) {
      return !value.endsWith(QStringLiteral("%"));
    }

    static const std::set<QString> TYPES{
      QStringLiteral("float"),
      QStringLiteral("coFloat"),
    };

    return TYPES.find(type) != TYPES.cend();
  }

  auto CheckPercentType(const QString& type, const QString& value) -> bool {
    // filter arrays
    if (type == QStringLiteral("coPercents")) {
      return !value.contains(QStringLiteral(","));
    }

    static const std::set<QString> TYPES{
      QStringLiteral("coPercent"),
      QStringLiteral("coPercents"),
    };

    return TYPES.find(type) != TYPES.cend();
  }

  auto CalculateFuzzyValue(const QString& type,
                           const QString& key,
                           const QString& value) -> QString {
    if (value == QStringLiteral("nil")) {
      return value;
    }

    if (CheckPercentType(type, value) && value.endsWith(QStringLiteral("%"))) {
      return value.left(value.length() - 1);
    }

    if (!CheckFloatType(type, value)) {
      return value;
    }

    auto number_valid{ false };
    const auto number = value.toFloat(&number_valid);
    if (!number_valid) {
      return value;
    }

    constexpr auto FORMAT{ 'f' };
    constexpr auto PRECISION{ 3 };
    static const QRegularExpression REGEXPER{ QStringLiteral("0+$") };

    auto string = QString::number(number, FORMAT, PRECISION).trimmed().remove(REGEXPER);
    if (string.endsWith(QStringLiteral("."))) {
      string.chop(1);
    }

    return string;
  }

  auto CheckJsScriptValid(const QString& script) -> bool {
    static const std::set<QString> FEATURES{
      QStringLiteral("machine.value"),
      QStringLiteral("extruder.value"),
      QStringLiteral("material.value"),
      QStringLiteral("process.value"),
      QStringLiteral("context.value"),
      QStringLiteral("contex.value"),  // old
    };

    for (const auto& feature : FEATURES) {
      if (script.contains(feature)) {
        return true;
      }
    }

    return false;
  }

  auto InvokeJsScript(const QString& script, ParameterDataModel* context) -> QJSValue {
    auto* kernel_ui = getKernelUI();
    if (!kernel_ui) {
      return QJSValue{ QJSValue::SpecialValue::NullValue };
    }

    auto* engine = kernel_ui->getQmlEngine();
    if (!engine) {
      return QJSValue{ QJSValue::SpecialValue::NullValue };
    }

    auto root = engine->globalObject();
    if (root.errorType() != QJSValue::ErrorType::NoError) {
      return QJSValue{ QJSValue::SpecialValue::NullValue };
    }

    std::map<QString, ParameterDataModel*> name_context_map{};

    // "this" data model
    if (context) {
      name_context_map.emplace(QStringLiteral("context"), context);
      name_context_map.emplace(QStringLiteral("contex"), context);  // old
    }

    do {
      // current machine data model
      auto* machine = getKernel()->parameterManager()->currentMachine();
      if (!machine) {
        break;
      }

      auto* machine_data = machine->getDataModel();
      if (machine_data) {
        name_context_map.emplace(QStringLiteral("machine"), machine_data);
      }

      // current extruder data model
      auto* extruder_data = machine->getExtruder1DataModel();
      if (extruder_data) {
        name_context_map.emplace(QStringLiteral("extruder"), extruder_data);
      }

      // current process data model
      auto* process = machine->currentProfile();
      if (process) {
        auto* process_data = process->getDataModel();
        if (process_data) {
          name_context_map.emplace(QStringLiteral("process"), process_data);
        }
      }

      // current material data model
      const auto& extruders = machine->extruders();
      if (extruders.empty()) {
        break;
      }

      auto* extruder = extruders.front();
      if (!extruder) {
        break;
      }

      auto* material = machine->materialWithName(extruder->materialName());
      if (!material) {
        break;
      }

      auto* material_data = material->getDataModel();
      if (material_data) {
        name_context_map.emplace(QStringLiteral("material"), material_data);
      }

    } while (false);

    // filter unvalid value (percentages etc.)
    auto has_unvalid_value{ false };

    for (const auto& pair : name_context_map) {
      const auto pattern = QStringLiteral("%1\\.value\\('(.*?)'\\)").arg(pair.first);
      const QRegularExpression expression{ pattern };
      auto iter = expression.globalMatch(script);

      while (iter.hasNext()) {
        auto match = iter.next();
        const auto key = match.captured(1);
        const auto value = pair.second->value(key);
        if (value.endsWith(QStringLiteral("%"))) {
          has_unvalid_value = true;
          break;
        }
      }
    }

    if (has_unvalid_value) {
      return QJSValue{ QJSValue::SpecialValue::NullValue };
    }

    // inject data context object into js script context
    for (const auto& pair : name_context_map) {
      if (pair.second) {
        root.setProperty(pair.first, engine->newQObject(pair.second));
      }
    }

    return engine->evaluate(script);
  }

  auto CheckEmptySettings(us::USettings* settings) -> bool {
    if (!settings) {
      return true;
    }

    if (settings->isEmpty()) {
      return true;
    }

    return false;
  }

  auto CheckEmptySettings(us::USettings* settings, const std::set<QString>& ignore_keys) -> bool {
    if (CheckEmptySettings(settings)) {
      return true;
    }

    if (ignore_keys.empty()) {
      return false;
    }

    for (const auto& key : settings->hashSettings().keys()) {
      if (ignore_keys.find(key) == ignore_keys.cend()) {
        return false;
      }
    }

    return true;
  }





  struct ParameterMessageContext::Internal {
    QPointer<Printer> platform{ nullptr };
    QPointer<PrinterMangager> platform_manager{ nullptr };
    QPointer<KernelUI> kernel_ui{ nullptr };
    std::map<QObject*, std::unique_ptr<Message>> message_map{};
  };

  ParameterMessageContext::ParameterMessageContext()
      : QObject{ nullptr }
      , internal_{ std::make_unique<Internal>() } {}

  ParameterMessageContext::~ParameterMessageContext() {
    uninitialize();
  }

  auto ParameterMessageContext::initialize() -> void {
    if (isInitialized()) {
      return;
    }

    QPointer<Kernel> kernel = getKernel();
    if (!kernel) {
      return;
    }

    internal_->kernel_ui = kernel->kernelUI();
    if (!internal_->kernel_ui) {
      return;
    }

    QPointer<ReuseableCache> reuseable = kernel->reuseableCache();
    internal_->platform_manager = reuseable ? reuseable->getPrinterMangager() : nullptr;
    if (!internal_->platform_manager) {
      return;
    }

    connect(internal_->platform_manager, &PrinterMangager::signalDidSelectPrinter, this,
      [this](Printer* printer) {
        if (internal_->platform == printer) {
          return;
        }

        if (internal_->platform) {
          internal_->platform->disconnect(this);
        }

        internal_->platform = printer;

        if (internal_->platform) {
          connect(internal_->platform, &Printer::signalModelsInsideChange,
                  this, &ParameterMessageContext::refreshMessage);
        }

        refreshMessage();
      });

    internal_->platform = internal_->platform_manager->getSelectedPrinter();

    if (internal_->platform) {
      connect(internal_->platform, &Printer::signalModelsInsideChange,
              this, &ParameterMessageContext::refreshMessage);
    }

    initialized_ = true;

    refreshMessage();
  }

  auto ParameterMessageContext::uninitialize() -> void {
    if (!isInitialized()) {
      return;
    }

    if (internal_->platform_manager) {
      internal_->platform_manager->disconnect(this);
    }

    internal_->platform_manager = nullptr;
    internal_->platform = nullptr;

    initialized_ = false;
  }

  auto ParameterMessageContext::isInitialized() const -> bool {
    return initialized_;
  }

  auto ParameterMessageContext::emplaceMessage(QObject* owner,
                                               std::unique_ptr<Message>&& message) -> void {
    if (!owner || !message) {
      return;
    }

    connect(owner, &QObject::destroyed,
            this, &ParameterMessageContext::onOwnerDestroyed,
            Qt::ConnectionType::UniqueConnection);

    auto iter = internal_->message_map.find(owner);
    if (iter != internal_->message_map.cend()) {
      if (!message->getTreeNode() && iter->second) {
        message->setTreeNode(iter->second->getTreeNode());
      }

      hideMessage(iter->second.get());
      internal_->message_map.erase(iter);
    }

    internal_->message_map.emplace(owner, std::move(message));
    refreshMessage();
  }

  auto ParameterMessageContext::eraseMessage(QObject* owner) -> void {
    auto iter = internal_->message_map.find(owner);
    if (iter == internal_->message_map.cend()) {
      return;
    }

    hideMessage(iter->second.get());

    // ensure message delete after SpaceTracer's callback loop finished
    iter->second.release()->deleteLater();

    internal_->message_map.erase(iter);

    refreshMessage();
  }

  auto ParameterMessageContext::refreshMessage() -> void {
    if (!isInitialized() || !internal_->platform) {
      for (const auto& pair : internal_->message_map) {
        auto& message = pair.second;
        if (message) {
          showMessage(message.get());
        }
      }

      return;
    }

    QList<ModelN*> platform_models{};
    QList<ModelGroup*> platform_model_groups{};
    if (internal_->platform) {
      platform_models = internal_->platform->modelsInside();
      platform_model_groups = internal_->platform->modelGroupsInside();
    }

    std::list<Message*> global_messages{};
    std::list<Message*> object_messages{};

    for (const auto& pair : internal_->message_map) {
      auto& message = pair.second;
      if (!message) {
        continue;
      }

      const auto model = message->getModel();
      const auto model_group = message->getModelGroup();
      if (!model && !model_group) {
        global_messages.emplace_back(message.get());
        continue;
      }

      if (platform_models.contains(model) || platform_model_groups.contains(model_group)) {
        object_messages.emplace_back(message.get());
        continue;
      }

      hideMessage(message.get());
    }

    for (const auto& message : global_messages) {
      showMessage(message);
    }

    for (const auto& message : object_messages) {
      showMessage(message);
    }
  }

  auto ParameterMessageContext::showMessage(Message* message) -> void {
    if (!message || !internal_->kernel_ui) {
      return;
    }

    internal_->kernel_ui->requestMessage(message);
    QQmlEngine::setObjectOwnership(message, QQmlEngine::ObjectOwnership::CppOwnership);
  }

  auto ParameterMessageContext::hideMessage(Message* message) -> void {
    if (!message || !internal_->kernel_ui) {
      return;
    }

    internal_->kernel_ui->destroyMessage(message->code());
  }

  auto ParameterMessageContext::onOwnerDestroyed() -> void {
    eraseMessage(sender());
  }





  namespace {

    auto GetMessageContext() -> ParameterMessageContext* {
      static std::pair<bool, QPointer<Kernel>>                  kernel{ false, nullptr };
      static std::pair<bool, QPointer<UiParameterManager>>      manager{ false, nullptr };
      static std::pair<bool, QPointer<ParameterMessageContext>> context{ false, nullptr };

      if (!kernel.first && !kernel.second) {
        kernel.second = getKernelSafely();
        kernel.first = kernel.second != nullptr;
      }

      if (kernel.second && !manager.first && !manager.second) {
        manager.second = kernel.second->uiParameterManager();
        manager.first = manager.second != nullptr;
      }

      if (manager.second && !context.first && !context.second) {
        context.second = manager.second->getMessageContext();
        context.first = context.second != nullptr;
      }

      return context.second.data();
    }

  }  // namespace

  auto EmplaceParameterMessage(
      QObject* owner, std::unique_ptr<ParameterMessageContext::Message>&& message) -> void {
    auto* context = GetMessageContext();
    if (context) {
      context->emplaceMessage(owner, std::move(message));
    }
  }

  auto EraseParameterMessage(QObject* owner) -> void {
    auto* context = GetMessageContext();
    if (context) {
      context->eraseMessage(owner);
    }
  }

}  // namespace creative_kernel
