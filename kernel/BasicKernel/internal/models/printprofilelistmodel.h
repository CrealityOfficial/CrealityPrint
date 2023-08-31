#ifndef CREATIVE_KERNEL_PROFILELISTMODEL_1675918310093_H
#define CREATIVE_KERNEL_PROFILELISTMODEL_1675918310093_H
#include <QtCore/QObject>
#include <QtCore/QAbstractListModel>

namespace creative_kernel
{
    class PrintMachine;
    class PrintProfile;
    class PrintProfileListModel : public QAbstractListModel
    {
        Q_OBJECT
    public:
        explicit PrintProfileListModel(PrintMachine* machine, QObject* parent = nullptr);
        ~PrintProfileListModel();

        Q_INVOKABLE void notifyReset();
    protected:
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role) override;
        virtual QHash<int, QByteArray> roleNames() const override;
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    private:
        QVariant getProfileValue(PrintProfile* profile, const QString& key, const QVariant& defaultValue) const;

    protected:
        PrintMachine* m_machine;
    };
}

#endif // CREATIVE_KERNEL_PROFILELISTMODEL_1675918310093_H