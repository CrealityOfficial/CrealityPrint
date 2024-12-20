#include "localnetworkinterface/autorefillinfolistmodel.h"

#include <QtCore/QUuid>

#include <rapidjson/document.h>
#include <rapidjson/stream.h>

namespace creative_kernel {

  PageFilterListModel::PageFilterListModel(QObject* parent) : PageFilterListModel{ 3, parent } {}

  PageFilterListModel::PageFilterListModel(int page_size, QObject* parent)
      : QSortFilterProxyModel{ parent }, page_size_{ page_size } {
    connect(this, &QSortFilterProxyModel::sourceModelChanged,
            this, &PageFilterListModel::onSourceModelChanged);

    connect(this, &PageFilterListModel::currentPageChanged,
            this, [this]() { invalidateFilter(); });
  }

  auto PageFilterListModel::getPageSize() const -> int {
    return page_size_;
  }

  auto PageFilterListModel::getCurrentPage() const -> int {
    return current_page_;
  }

  auto PageFilterListModel::setCurrentPage(int current_page) -> void {
    current_page = std::max(0, std::min(current_page, getPageCount() - 1));
    if (current_page_ != current_page) {
      current_page_ = current_page;
      currentPageChanged();
    }
  }

  auto PageFilterListModel::getPageCount() const -> int {
    auto* source_model = sourceModel();
    auto total_count = source_model->rowCount();
    return total_count / page_size_ + (total_count % page_size_ == 0 ? 0 : 1);
  }

  auto PageFilterListModel::filterAcceptsRow(int                source_row,
                                            const QModelIndex& source_parent) const -> bool {
    auto* source_model = sourceModel();
    if (!source_model) {
      return false;
    }

    auto max_row = std::min((current_page_ + 1) * page_size_, source_model->rowCount());
    auto min_row = std::max(0, current_page_ * page_size_);
    return source_row >= min_row && source_row < max_row;
  }

  auto PageFilterListModel::onSourceModelChanged() -> void {
    auto* source_model = sourceModel();
    if (!source_model) {
      return;
    }

    connect(source_model, &QAbstractItemModel::rowsInserted,
            this, &PageFilterListModel::pageCountChanged);

    connect(source_model, &QAbstractItemModel::rowsRemoved,
            this, &PageFilterListModel::pageCountChanged);

    connect(source_model, &QAbstractItemModel::modelReset,
            this, &PageFilterListModel::pageCountChanged);
  }





  auto AutoRefillInfoListModel::pressJsonData(const QString& json_string) -> void {
    auto bytes = json_string.toUtf8();
    rapidjson::StringStream stream{ bytes.constData() };
    rapidjson::Document document{};
    if (document.ParseStream(stream).HasParseError()) {
      return;
    }

    auto& root = document;
    if (!root.IsArray()) {
      return;
    }

    DataContainer datas{};
    for (const auto& item : root.GetArray()) {
      if (!item.IsArray() || item.Size() != 4) {
        continue;
      }

      if (!item[0].IsString() || !item[1].IsString() || !item[2].IsArray() || !item[3].IsString()) {
        continue;
      }

      decltype(Data::box_name_list) box_name_list{};
      for (const auto& box : item[2].GetArray()) {
        if (!box.IsObject()) {
          continue;
        }

        if (!box.HasMember("boxId") || !box["boxId"].IsInt() ||
            !box.HasMember("materialId") || !box["materialId"].IsInt()) {
          continue;
        }

        auto box_index = box["boxId"].GetInt();
        auto material_index = box["materialId"].GetInt();

        static const QString CHARS{ "ABCDEFGHIJKLMNOPQRSTUVWXYZ" };
        box_name_list.push_back(QStringLiteral("%1%2").arg(QString::number(box_index),
                                                           QString{ CHARS[material_index] }));
      }

      if (box_name_list.size() < 2) {
        continue;
      }

      auto color_string = QString::fromUtf8(item[1].GetString()).replace(0, 1, QStringLiteral("#"));
      auto material_name = QString::fromUtf8(item[3].GetString());

      datas.push_back(Data{
        QUuid::createUuid().toString(), // uid
        std::move(material_name),       // material_name
        QColor{ color_string },         // material_color
        std::move(box_name_list),       // box_name_list
      });
    }

    clear();
    emplaceBack(std::move(datas));
  }

  auto AutoRefillInfoListModel::generlatePageFilterListModel(int page_size) -> QObject* {
    auto* filter_model = new PageFilterListModel{ page_size, this };
    filter_model->setSourceModel(this);
    return filter_model;
  }

  auto AutoRefillInfoListModel::data(const QModelIndex& index, int role) const -> QVariant {
    const auto& datas = rawData();
    const auto data_index = static_cast<size_t>(index.row());
    if (index.row() < 0 || data_index >= rawData().size()) {
      return { QVariant::Type::Invalid };
    }

    const auto data = datas.at(data_index);
    switch (static_cast<DataRole>(role)) {
      case DataRole::UID: {
        return QVariant::fromValue(data.uid);
      }
      case DataRole::MATERIAL_NAME: {
        return QVariant::fromValue(data.material_name);
      }
      case DataRole::MATERIAL_COLOR: {
        return QVariant::fromValue(data.material_color);
      }
      case DataRole::BOX_NAME_LIST: {
        return QVariant::fromValue(data.box_name_list);
      }
      default: {
        return { QVariant::Type::Invalid };
      }
    }
  }

  auto AutoRefillInfoListModel::roleNames() const -> QHash<int, QByteArray> {
    static const auto ROLE_NAMES = QHash<int, QByteArray>{
      { static_cast<int>(DataRole::UID           ), QByteArrayLiteral("uid"          ) },
      { static_cast<int>(DataRole::MATERIAL_NAME ), QByteArrayLiteral("materialName" ) },
      { static_cast<int>(DataRole::MATERIAL_COLOR), QByteArrayLiteral("materialColor") },
      { static_cast<int>(DataRole::BOX_NAME_LIST ), QByteArrayLiteral("boxNameList"  ) },
    };

    return ROLE_NAMES;
  }

}  // namespace creative_kernel
