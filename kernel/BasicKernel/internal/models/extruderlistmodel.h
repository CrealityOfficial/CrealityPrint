#ifndef CREATIVE_KERNEL_EXTRUDERLISTMODEL_H
#define CREATIVE_KERNEL_EXTRUDERLISTMODEL_H
#include <QtCore/QObject>
#include <QtCore/QAbstractListModel>

namespace creative_kernel
{
    class PrintMachine;
    class ExtruderListModel : public QAbstractListModel
    {
        Q_OBJECT
    public:
        explicit ExtruderListModel(PrintMachine* machine, QObject* parent = nullptr);
        ~ExtruderListModel();

        void refresh();
        void refreshItem(int index);
        Q_INVOKABLE void elmInsertRow(int row = -1);
        Q_INVOKABLE void elmRemoveRow(int row = -1);
        Q_INVOKABLE QVariantList colorList();
        Q_INVOKABLE QVariantList typeList();
        Q_INVOKABLE QVariantMap get(int index) const;

        int getCount() const;
        Q_SIGNAL void countChanged();
        Q_PROPERTY(int count READ getCount NOTIFY countChanged);

        Q_INVOKABLE void setExtruder(int index, const QString& uid, const QString& type, const QString& color);

    protected:
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        virtual QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    protected:
        PrintMachine* m_machine;
    };
}

#endif // CREATIVE_KERNEL_PROFILELISTMODEL_1675918310093_H
