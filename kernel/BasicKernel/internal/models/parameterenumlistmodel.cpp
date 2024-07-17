#include "internal/models/parameterenumlistmodel.h"

#include <QtQml/QQmlEngine>

#include "internal/models/parameterdatamodel.h"
#include "internal/parameter/parametermanager.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/printmaterial.h"
#include "internal/parameter/printextruder.h"

#include "external/interface/commandinterface.h"
#include "external/kernel/kernel.h"

namespace creative_kernel {

  auto ParameterEnumListModel::Create(QPointer<const DataItem> data_item)
      -> std::unique_ptr<ParameterEnumListModel> {
    std::unique_ptr<ParameterEnumListModel> model{ nullptr };

    do {
      if (!data_item) {
        break;
      }

      const auto key = data_item->getKey();
      if (key.isEmpty()) {
        break;
      }

      if (key == QStringLiteral("support_filament") ||
          key == QStringLiteral("support_interface_filament")) {
        model.reset(new SupportFilamentEnumListModel{ data_item });
        break;
      }

      model.reset(new ParameterEnumListModel{ data_item });
    } while (false);

    if (model) {
      QQmlEngine::setObjectOwnership(model.get(), QQmlEngine::ObjectOwnership::CppOwnership);
    }

    return model;
  }

  ParameterEnumListModel::ParameterEnumListModel(QPointer<const DataItem> data_item)
      : DataListModel<ParameterEnumItem>{ nullptr }
      , data_item_{ data_item } {
    if (data_item_) {
      DataContainer&& enums{};
      for (const auto& enum_data : data_item_->getDefaultSetting()->enums()) {
        const auto pair = enum_data.toString().split(QStringLiteral("="));
        if (pair.size() == 2) {
          enums.emplace_back(Data{
            pair.at(0),  // uid
            pair.at(1),  // name
          });
        }
      }
      pushBack(std::move(enums));
    }

    connect(this, &QAbstractItemModel::rowsInserted, this, &ParameterEnumListModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &ParameterEnumListModel::countChanged);
    addUIVisualTracer(this,this);
  }

  ParameterEnumListModel::ParameterEnumListModel(const DataContainer& enums,
                                                 QPointer<const DataItem> data_item)
      : DataListModel<ParameterEnumItem>{ enums, nullptr }
      , data_item_{ data_item } {
    connect(this, &QAbstractItemModel::rowsInserted, this, &ParameterEnumListModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &ParameterEnumListModel::countChanged);
    addUIVisualTracer(this,this);
  }

  ParameterEnumListModel::ParameterEnumListModel(DataContainer&& enums,
                                                 QPointer<const DataItem> data_item)
      : DataListModel<ParameterEnumItem>{ std::move(enums), nullptr }
      , data_item_{ data_item } {
    connect(this, &QAbstractItemModel::rowsInserted, this, &ParameterEnumListModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &ParameterEnumListModel::countChanged);
    addUIVisualTracer(this,this);
  }

  auto ParameterEnumListModel::getDataItem() const -> QPointer<const DataItem> {
    return data_item_;
  }

  auto ParameterEnumListModel::getFilterModel() const -> QPointer<FilterModel> {
    if (!filter_model_initialized_) {
      auto* self = const_cast<ParameterEnumListModel*>(this);
      filter_model_ = ParameterEnumFilterListModel::Create(self);
      filter_model_initialized_ = true;
    }

    return filter_model_.get();
  }

  auto ParameterEnumListModel::getFilterModelObject() const -> QObject* {
    return getFilterModel();
  }

  auto ParameterEnumListModel::getCount() const -> int {
    return getSize();
  }

  auto ParameterEnumListModel::get(int row) const -> QVariantMap {
    QVariantMap name_value_map = {};

    const auto index = createIndex(std::max(0, std::min(row, rowCount())), 0);
    const auto role_name_map = roleNames();
    for (auto iter = role_name_map.cbegin(); iter != role_name_map.cend(); ++iter) {
      const auto& role = iter.key();
      const auto& name = iter.value();
      name_value_map.insert(name, data(index, role));
    }

    return name_value_map;
  }

  auto ParameterEnumListModel::data(const QModelIndex& index, int role) const -> QVariant {
    if (index.row() < 0 || index.row() >= rawData().size()) {
      return { QVariant::Type::Invalid };
    }

    const auto data = rawData().at(index.row());
    switch (static_cast<Role>(role)) {
      case Role::UID: {
        return QVariant::fromValue(data.uid);
      }
      case Role::NAME: {
        return QVariant::fromValue(data.name);
      }
      case Role::COLOR: {
        return QVariant::fromValue(data.color);
      }
      case Role::IMAGE: {
        if (!data.image.isEmpty()) {
          return QVariant::fromValue(data.image);
        }

        auto key = data_item_ ? data_item_->getKey() : QString{};
        return QVariant::fromValue(
          QStringLiteral("qrc:/UI/photo/parameter/%1-%2-%3.svg").arg(key, data.uid, theme_));
      }
      case Role::HOVERED_IMAGE: {
        if (!data.hovered_image.isEmpty()) {
          return QVariant::fromValue(data.hovered_image);
        }

        auto key = data_item_ ? data_item_->getKey() : QString{};
        return QVariant::fromValue(
          QStringLiteral("qrc:/UI/photo/parameter/%1-%2-dark.svg").arg(key, data.uid));
      }
      default: {
        break;
      }
    }

    return { QVariant::Type::Invalid };
  }

  auto ParameterEnumListModel::roleNames() const -> QHash<int, QByteArray> {
    static const QHash<int, QByteArray> ROLE_NAME{
      { static_cast<int>(Role::UID), QByteArrayLiteral("uid") },
      { static_cast<int>(Role::NAME), QByteArrayLiteral("name") },
      { static_cast<int>(Role::COLOR), QByteArrayLiteral("color") },
      { static_cast<int>(Role::IMAGE), QByteArrayLiteral("image") },
      { static_cast<int>(Role::HOVERED_IMAGE), QByteArrayLiteral("hoveredImage") },
    };

    return ROLE_NAME;
  }

  auto ParameterEnumListModel::onThemeChanged(ThemeCategory theme) -> void {
    theme_ = theme == ThemeCategory::tc_light ? QStringLiteral("light") : QStringLiteral("dark");
    dataChanged(createIndex(0, 0),
                createIndex(rowCount() - 1, 0),
                { static_cast<int>(Role::IMAGE) });
  }

  auto ParameterEnumListModel::onLanguageChanged(MultiLanguage language) -> void {
    std::ignore = language;
  }





  struct SupportFilamentEnumListModel::Internal {
    QPointer<PrintMachine> machine;
  };

  SupportFilamentEnumListModel::~SupportFilamentEnumListModel() {
    // do nothing
    // Make sure the destructor is defined after the Internal struct.
  }

  SupportFilamentEnumListModel::SupportFilamentEnumListModel(QPointer<const DataItem> data_item)
      : ParameterEnumListModel{ DataContainer{},  data_item }
      , internal_{ std::make_unique<Internal>() } {
    auto* kernel = getKernel();
    if (!kernel) {
      assert(false && u8"Ensure Kernel init before SupportFilamentEnumListModel.");
      return;
    }

    auto* parameter_manager = kernel->parameterManager();
    if (!parameter_manager) {
      assert(false && u8"Ensure ParameterManager init before SupportFilamentEnumListModel.");
      return;
    }

    connect(parameter_manager, &ParameterManager::curMachineIndexChanged,
            this, &SupportFilamentEnumListModel::onCurrentMachineChanged,
            Qt::ConnectionType::UniqueConnection);

    onCurrentMachineChanged();
  }

  auto SupportFilamentEnumListModel::onCurrentMachineChanged() -> void {
    for (const auto& connection : connections_) {
      disconnect(connection);
    }

    connections_.clear();

    auto* kernel = getKernel();
    if (!kernel) {
      return;
    }

    auto* parameter_manager = kernel->parameterManager();
    if (!parameter_manager) {
      return;
    }

    internal_->machine = parameter_manager->currentMachine();
    if (!internal_->machine) {
      return;
    }

    connections_.emplace_back(connect(internal_->machine.data(), &PrintMachine::extruderAdded,
                                      this, &SupportFilamentEnumListModel::onExtruderAdded));
    connections_.emplace_back(connect(internal_->machine.data(), &PrintMachine::extruderRemoved,
                                      this, &SupportFilamentEnumListModel::onExtruderRemoved));

    DataContainer&& datas{
      {
        QStringLiteral("0"),                     // uid
        QStringLiteral("Default"),               // name
        QColor{ Qt::GlobalColor::transparent },  // color
      }
    };

    const auto extruders = internal_->machine->extruders();
    for (auto index = 0; index < extruders.size(); ++index) {
      auto* extruder = extruders.at(index);
      if (!extruder) {
        continue;
      }

      auto* material = internal_->machine->material(extruder->materialIndex());
      if (!material) {
        continue;
      }

      datas.emplace_back(Data{
        QString::number(index + 1),  // uid
        material->materialType(),    // name
        extruder->color(),           // color
      });

      connections_.emplace_back(connect(
          extruder, &PrintExtruder::colorChanged,
          this, &SupportFilamentEnumListModel::onExtruderColorChanged));

      connections_.emplace_back(connect(
          extruder, &PrintExtruder::materialIndexChanged,
          this, &SupportFilamentEnumListModel::onExtruderMaterialChanged));
    }

    beginResetModel();
    rawData() = std::move(datas);
    endResetModel();
  }

  auto SupportFilamentEnumListModel::onExtruderAdded(QObject* extruder_object) -> void {
    if (!internal_->machine) {
      return;
    }

    auto* extruder = dynamic_cast<PrintExtruder*>(extruder_object);
    if (!extruder) {
      return;
    }

    auto* material = internal_->machine->material(extruder->materialIndex());
    if (!material) {
      return;
    }

    connections_.emplace_back(connect(
        extruder, &PrintExtruder::colorChanged,
        this, &SupportFilamentEnumListModel::onExtruderColorChanged));

    connections_.emplace_back(connect(
        extruder, &PrintExtruder::materialIndexChanged,
        this, &SupportFilamentEnumListModel::onExtruderMaterialChanged));

    pushBack(Data{
      QString::number(getCount()),
      material->materialType(),
      extruder->color(),
    });
  }

  auto SupportFilamentEnumListModel::onExtruderRemoved(QObject* extruder_object) -> void {
    const auto index = getCount() - 1;
    if (index <= 0) {
      return;
    }

    if (extruder_object) {
      extruder_object->disconnect(this);
    }

    beginRemoveRows(QModelIndex{}, index, index);
    auto& data = rawData();
    data.erase(--data.cend());
    endRemoveRows();
  }

  auto SupportFilamentEnumListModel::onExtruderColorChanged() -> void {
    if (!internal_->machine) {
      return;
    }

    auto* extruder = dynamic_cast<PrintExtruder*>(sender());
    if (!extruder) {
      return;
    }

    auto row = -1;
    const auto extruders = internal_->machine->extruders();
    for (auto index = 0; index < extruders.size(); ++index) {
      if (extruders.at(index) == extruder) {
        row = index + 1;
        rawData().at(row).color = extruder->color();
        break;
      }
    }

    if (row == -1) {
      return;
    }

    dataChanged(createIndex(row, 0),
                createIndex(row, 0),
                { static_cast<int>(Role::COLOR) });
  }

  auto SupportFilamentEnumListModel::onExtruderMaterialChanged() -> void {
    if (!internal_->machine) {
      return;
    }

    auto* extruder = dynamic_cast<PrintExtruder*>(sender());
    if (!extruder) {
      return;
    }

    auto* material = internal_->machine->material(extruder->materialIndex());
    if (!material) {
      return;
    }

    auto row = -1;
    const auto extruders = internal_->machine->extruders();
    for (auto index = 0; index < extruders.size(); ++index) {
      if (extruders.at(index) == extruder) {
        row = index + 1;
        rawData().at(row).name = material->materialType();
        break;
      }
    }

    if (row == -1) {
      return;
    }

    dataChanged(createIndex(row, 0),
                createIndex(row, 0),
                { static_cast<int>(Role::NAME) });
  }

}  // namespace creative_kernel
