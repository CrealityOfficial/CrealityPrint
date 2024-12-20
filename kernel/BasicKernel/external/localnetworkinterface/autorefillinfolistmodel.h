#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QObject>
#include <QtCore/QSortFilterProxyModel>
#include <QtCore/QString>
#include <QtGui/QColor>

#include <qtusercore/plugin/datalistmodel.h>

#include "basickernelexport.h"

namespace creative_kernel {

  struct AutoRefillInfo{
    QString uid{};  // unused
    QString material_name{};
    QColor material_color{};
    QStringList box_name_list{};
  };

  class BASIC_KERNEL_API AutoRefillInfoListModel : public qtuser_qml::DataListModel<AutoRefillInfo> {
    Q_OBJECT;

   public:
    enum class DataRole : int {
      UID = Qt::ItemDataRole::UserRole,  // unused
      MATERIAL_NAME,
      MATERIAL_COLOR,
      BOX_NAME_LIST,
    };

   public:
    using SuperType::SuperType;
    virtual ~AutoRefillInfoListModel() = default;

   public:
    auto pressJsonData(const QString& json_string) -> void;
    Q_INVOKABLE QObject* generlatePageFilterListModel(int page_size);

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;
  };





  class BASIC_KERNEL_API PageFilterListModel : public QSortFilterProxyModel {
    Q_OBJECT;

   public:
    explicit PageFilterListModel(QObject* parent = nullptr);
    explicit PageFilterListModel(int page_size, QObject* parent = nullptr);
    virtual ~PageFilterListModel() = default;

   public:
    Q_PROPERTY(int pageSize READ getPageSize CONSTANT);
    auto getPageSize() const -> int;

    Q_PROPERTY(int currentPage READ getCurrentPage WRITE setCurrentPage NOTIFY currentPageChanged);
    Q_SIGNAL void currentPageChanged() const;
    auto getCurrentPage() const -> int;
    auto setCurrentPage(int current_page) -> void;

    Q_PROPERTY(int pageCount READ getPageCount NOTIFY pageCountChanged);
    Q_SIGNAL void pageCountChanged() const;
    auto getPageCount() const -> int;

   protected:
    auto filterAcceptsRow(int source_row, const QModelIndex& source_parent) const -> bool override;

   private:
    auto onSourceModelChanged() -> void;

   private:
    int page_size_{ 3 };
    int current_page_{ 0 };
  };

}  // namespace creative_kernel
