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
      COLOR,
      COLORINDEX,
      DEFAULTORSYNC,//0: default 1:user 2:sync
      SYNCPBOXINDEX,
      OBJECT
    };

   public:
    explicit PrintMaterialListModel(PrintMachine* machine, QObject* parent = nullptr);
    virtual ~PrintMaterialListModel() override = default;

   public:
    Q_INVOKABLE QVariantMap get(int row) const;

    auto insertMaterial(PrintMaterial* material, size_t index) -> void;
    auto removeMaterial(PrintMaterial* material, size_t index) -> void;
    Q_INVOKABLE void refresh();

   protected:
    auto roleNames() const -> QHash<int, QByteArray> override;
    auto rowCount(const QModelIndex& parent = QModelIndex{}) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;

   private:
     void generateData(bool isSynced, int row, QVariant& name, QVariant& isModify, QVariant& isDefault, 
         QVariant& defaultOrSync, QVariant& color, QVariant& colorIndex, QVariant& obj, QVariant& reserve) const;

    Q_SLOT void onSettingsModifyedChanged();

   protected:
    QPointer<PrintMachine> machine_{ nullptr };
  };

}  // namespace creative_kernel
