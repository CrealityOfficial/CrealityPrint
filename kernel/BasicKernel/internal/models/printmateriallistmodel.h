#pragma once

#include <QtCore/QAbstractListModel>
#include <QtCore/QPointer>

#include "internal/parameter/printmaterial.h"

namespace creative_kernel {

  class PrintMachine;

  class PrintMaterialListModel : public QAbstractListModel {
    Q_OBJECT;

   public:
    enum class Role : int {
      NAME = Qt::ItemDataRole::UserRole,
      DEFAULT,
      MODIFYED,
      OBJECT
    };

   public:
    explicit PrintMaterialListModel(PrintMachine* machine, QObject* parent = nullptr);
    virtual ~PrintMaterialListModel() override = default;

   public:
    Q_INVOKABLE QVariantMap get(int row) const;

    auto insertMaterial(PrintMaterial* material, size_t index) -> void;
    auto removeMaterial(PrintMaterial* material, size_t index) -> void;

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
