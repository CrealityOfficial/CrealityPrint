#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QPointer>

#include "internal/parameter/printmachine.h"

namespace creative_kernel {

  class ParameterManager;

  class PrintMachineListModel : public QAbstractListModel {
    Q_OBJECT;

   public:
    enum class Role : int {
      NAME = Qt::ItemDataRole::UserRole,
      BASENAME,
      DEFAULT,
      MODIFYED,
      OBJECT,
    };

   public:
    explicit PrintMachineListModel(ParameterManager* manager, QObject* parent = nullptr);
    virtual ~PrintMachineListModel() override = default;

   public:
    Q_INVOKABLE QVariantMap get(int row) const;

    auto getCount() const -> int;
    Q_SIGNAL void countChanged() const;
    Q_PROPERTY(int count READ getCount NOTIFY countChanged);

    auto insertMachine(PrintMachine* machine, size_t index) -> void;
    auto removeMachine(PrintMachine* machine, size_t index) -> void;

   protected:
    auto roleNames() const -> QHash<int, QByteArray> override;
    auto rowCount(const QModelIndex& parent = QModelIndex{}) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;

   private:
    Q_SLOT void onSettingsModifyedChanged();

   protected:
    QPointer<ParameterManager> manager_{ nullptr };
  };

}  // namespace creative_kernel
