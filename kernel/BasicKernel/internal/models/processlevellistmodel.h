#pragma once

#include <QtCore/QObject>

#include <qtusercore/plugin/datalistmodel.h>

namespace creative_kernel {

  struct ProcessLevelItem {
    QString uid{};
    QString name{};
    int level{ 0 };
  };

  class ProcessLevelListModel : public qtuser_qml::DataListModel<ProcessLevelItem> {
    Q_OBJECT;

    auto operator=(const ProcessLevelListModel&) -> ProcessLevelListModel = delete;
    auto operator=(ProcessLevelListModel&&) -> ProcessLevelListModel = delete;
    ProcessLevelListModel(const ProcessLevelListModel&) = delete;
    ProcessLevelListModel(ProcessLevelListModel&&) = delete;

   public:
    explicit ProcessLevelListModel(QObject* parent = nullptr);
    virtual ~ProcessLevelListModel() = default;

   public:
    Q_INVOKABLE QVariantMap get(int index) const override;

   public:
    auto getCurrentIndex() const -> int;
    auto setCurrentIndex(int index) -> void;
    Q_SIGNAL void currentIndexChanged() const;
    Q_PROPERTY(int currentIndex
               READ getCurrentIndex
               WRITE setCurrentIndex
               NOTIFY currentIndexChanged);

    auto getDefaultIndex() const -> int;
    Q_PROPERTY(int defaultIndex READ getDefaultIndex CONSTANT);

    auto getCustomIndex() const -> int;
    Q_PROPERTY(int customIndex READ getCustomIndex CONSTANT);

   protected:
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

   private:
    using SuperType = qtuser_qml::DataListModel<ProcessLevelItem>;

    enum class DataRole : int {
      UID  = Qt::ItemDataRole::UserRole,
      NAME ,
      LEVEL,
    };
  };

}  // namespace creative_kernel
