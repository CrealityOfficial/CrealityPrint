#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QPointer>

#include "internal/parameter/printprofile.h"

namespace creative_kernel {

  class PrintMachine;

  class PrintProfileListModel : public QAbstractListModel {
    Q_OBJECT;

   public:
    enum class Role : int {
      NAME = Qt::ItemDataRole::UserRole,
      DEFAULT,
      MODIFYED,
      OBJECT,
    };

   public:
    explicit PrintProfileListModel(PrintMachine* machine, QObject* parent = nullptr);
    virtual ~PrintProfileListModel() override = default;

   public:
    Q_INVOKABLE QVariantMap get(int row) const;

    auto insertProfile(PrintProfile* profile, size_t index) -> void;
    auto removeProfile(PrintProfile* profile, size_t index) -> void;

    Q_SIGNAL void profileInserted(int index);
    Q_SIGNAL void profileRemoved(int index);

   protected:
    auto roleNames() const -> QHash<int, QByteArray> override;
    auto rowCount(const QModelIndex& parent = QModelIndex{}) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;

   private:
    Q_SLOT void onSettingsModifyedChanged();

   protected:
    QPointer<PrintMachine> machine_{ nullptr };
  };

}  // namespace creative_kernel
