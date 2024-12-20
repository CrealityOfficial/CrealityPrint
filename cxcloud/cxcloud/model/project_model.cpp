#include "cxcloud/model/project_model.h"

namespace cxcloud {

  ProjectListModel::ProjectListModel(const QStringList& vaild_suffix_list, QObject* parent)
      : SuperType{ parent }, vaild_suffix_list_{ vaild_suffix_list } {}

  ProjectListModel::ProjectListModel(QStringList&& vaild_suffix_list, QObject* parent)
      : SuperType{ parent }, vaild_suffix_list_{ std::move(vaild_suffix_list) } {}

  auto ProjectListModel::getVaildSuffixList() const -> QStringList {
    return vaild_suffix_list_;
  }

  auto ProjectListModel::setVaildSuffixList(const QStringList& vaild_suffix_list) -> void {
    vaild_suffix_list_ = vaild_suffix_list;
  }

  auto ProjectListModel::get(int index) const -> QVariantMap {
    auto result = QVariantMap{};

    const auto role_names = roleNames();
    const auto model_index = createIndex(index, 0);
    for (auto iter = role_names.cbegin(); iter != role_names.cend(); ++iter) {
      result.insert(iter.value(), data(model_index, iter.key()));
    }

    return result;
  }

  auto ProjectListModel::pushBack(const Data& data) -> bool {
    if (vaild_suffix_list_.empty() || vaild_suffix_list_.contains(data.suffix)) {
      return SuperType::pushBack(data);
    }

    return false;
  }

  auto ProjectListModel::pushBack(Data&& data) -> bool {
    if (vaild_suffix_list_.empty() || vaild_suffix_list_.contains(data.suffix)) {
      return SuperType::pushBack(std::move(data));
    }

    return false;
  }

  auto ProjectListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = static_cast<size_t>(index.row());
    if (data_index < 0 || data_index >= datas.size()) {
      return { QVariant::Invalid };
    }

    switch (const auto& data = datas[data_index]; static_cast<DataRole>(role)) {
      case DataRole::UID: {
        return QVariant::fromValue(data.uid);
      }
      case DataRole::NAME: {
        return QVariant::fromValue(data.name);
      }
      case DataRole::IMAGE: {
        return QVariant::fromValue(data.image);
      }
      case DataRole::MODEL_GROUP_UID: {
        return QVariant::fromValue(data.model_group_uid);
      }
      case DataRole::PRINTER_NAME: {
        return QVariant::fromValue(data.printer_name);
      }
      case DataRole::LAYER_HEIGHT: {
        return QVariant::fromValue(data.layer_height);
      }
      case DataRole::SPARSE_INFILL_DENSITY: {
        return QVariant::fromValue(data.sparse_infill_density);
      }
      case DataRole::PLATE_COUNT: {
        return QVariant::fromValue(data.plate_count);
      }
      case DataRole::PRINT_DURACTION: {
        return QVariant::fromValue(data.print_duraction);
      }
      case DataRole::MATERIAL_WEIGHT: {
        return QVariant::fromValue(data.material_weight);
      }
      case DataRole::AUTHOR_ID: {
        return QVariant::fromValue(data.author_id);
      }
      case DataRole::AUTHOR_NAME: {
        return QVariant::fromValue(data.author_name);
      }
      case DataRole::AUTHOR_IMAGE: {
        return QVariant::fromValue(data.author_image);
      }
      case DataRole::CREATION_TIMEPOINT: {
        return QVariant::fromValue(data.creation_timepoint);
      }
      case DataRole::SUFFIX: {
        return QVariant::fromValue(data.suffix);
      }
      case DataRole::URL: {
        return QVariant::fromValue(data.url);
      }
      default: {
        return { QVariant::Invalid };
      }
    }
  }

  auto ProjectListModel::roleNames() const -> QHash<int, QByteArray> {
    static const auto ROLE_NAMES = QHash<int, QByteArray>{
      { static_cast<int>(DataRole::UID            ), QByteArrayLiteral("model_uid"            ) },
      { static_cast<int>(DataRole::NAME           ), QByteArrayLiteral("model_name"           ) },
      { static_cast<int>(DataRole::IMAGE          ), QByteArrayLiteral("model_image"          ) },
      { static_cast<int>(DataRole::MODEL_GROUP_UID), QByteArrayLiteral("model_model_group_uid") },
      { static_cast<int>(DataRole::PRINTER_NAME   ), QByteArrayLiteral("model_printer_name"   ) },
      { static_cast<int>(DataRole::LAYER_HEIGHT   ), QByteArrayLiteral("model_layer_height"   ) },
      { static_cast<int>(DataRole::SPARSE_INFILL_DENSITY),
          QByteArrayLiteral("model_sparse_infill_density") },
      { static_cast<int>(DataRole::PLATE_COUNT    ), QByteArrayLiteral("model_plate_count"    ) },
      { static_cast<int>(DataRole::PRINT_DURACTION), QByteArrayLiteral("model_print_duraction") },
      { static_cast<int>(DataRole::MATERIAL_WEIGHT), QByteArrayLiteral("model_material_weight") },
      { static_cast<int>(DataRole::AUTHOR_ID      ), QByteArrayLiteral("model_author_id"      ) },
      { static_cast<int>(DataRole::AUTHOR_NAME    ), QByteArrayLiteral("model_author_name"    ) },
      { static_cast<int>(DataRole::AUTHOR_IMAGE   ), QByteArrayLiteral("model_author_image"   ) },
      { static_cast<int>(DataRole::CREATION_TIMEPOINT),
          QByteArrayLiteral("model_creation_timepoint") },
      { static_cast<int>(DataRole::SUFFIX         ), QByteArrayLiteral("model_suffix"         ) },
      { static_cast<int>(DataRole::URL            ), QByteArrayLiteral("model_url"            ) },
    };

    return ROLE_NAMES;
  }

}  // namespace cxcloud
