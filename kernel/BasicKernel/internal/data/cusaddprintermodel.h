#ifndef CUSADDPRINTERMODEL_H
#define CUSADDPRINTERMODEL_H

#include <set>
#include <map>
#include <vector>

#include <QtCore/QAbstractListModel>

namespace creative_kernel
{
    class /*BASIC_KERNEL_API*/ CusAddPrinterModel : public QAbstractListModel {
        Q_OBJECT;

    public:
        explicit CusAddPrinterModel(QObject* parent = nullptr);
        ~CusAddPrinterModel();
    public:
        void initialize();
        void reinitialize();

    public:
        Q_PROPERTY(QString currentPrinterName READ getCurrentPrinterName NOTIFY sigCurrentPrinterNameChanged);
        Q_SIGNAL void sigCurrentPrinterNameChanged(const QString& name);
        Q_INVOKABLE QString getCurrentPrinterName();

    public:
        Q_PROPERTY(QString currentTypeName READ getCurrentTypeName NOTIFY sigCurrentTypeNameChanged);
        Q_SIGNAL void sigCurrentTypeNameChanged(const QString& name);
        Q_INVOKABLE QString getCurrentTypeName();

    public:
        Q_PROPERTY(QStringList typeNameList READ getTypeNameList NOTIFY sigTypeNameListChanged);
        Q_SIGNAL void sigTypeNameListChanged(const QStringList& name);
        Q_INVOKABLE QStringList getTypeNameList();
        Q_INVOKABLE QStringList getPrinterNameList(const QString& type);
        Q_INVOKABLE QStringList getAllPrinterNameList();
        Q_INVOKABLE QString getModelNameByCodeName(const QString& codeName);
        Q_INVOKABLE QString getThumbByCodeName(const QString& printerModel);
        Q_INVOKABLE void selectPrinter(const QString& type, const QString& printer = {});

    protected:
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override final;
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override final;
        virtual QHash<int, QByteArray> roleNames() const override final;

    private:
        struct PrinterInfo {
            QString  image_url{ "" };
            QString  name{ "" };
            QString  code_name{ "" };
            uint32_t depth{ 0U };
            uint32_t width{ 0U };
            uint32_t height{ 0U };
            uint32_t nozzle_count{ 1U };
            bool     granular{ false };
        };

        enum class DataRole : int {
            IMAGE_URL = Qt::UserRole + 1,
            NAME,
            CODENAME,
            DEPTH,
            WIDTH,
            HEIGHT,
            NOZZLE_COUNT,
            GRANULAR,
            ADDED
        };

    private:
        std::map<QString, std::vector<PrinterInfo>> type_printer_map_;

        QStringList                                 type_name_list_;
        QStringList                                 printer_name_list_;
        std::map<QString, QStringList>              type_printer_name_map_;

        QString                                     current_type_name_;
        QString                                     current_printer_name_;
        std::vector<PrinterInfo>                    current_printer_vector_;
    };
}
#endif // CUSADDPRINTERMODEL_H
