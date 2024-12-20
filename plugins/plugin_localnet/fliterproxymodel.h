#ifndef FLITERPROXYMODEL_H
#define FLITERPROXYMODEL_H

#include "cuslistmodel.h"
#include "localnetworkinterface/RemotePrinterManager.h"

#include <QObject>
#include <QSortFilterProxyModel>

class FliterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(bool deviceMapCheck READ isDeviceMapCheck WRITE setDeviceMapCheck NOTIFY deviceMapCheckChanged)

public:
    Q_PROPERTY(QAbstractItemModel* cSourceModel READ cSourceModel CONSTANT)
    
    explicit FliterProxyModel(QObject *parent = nullptr);

    Q_INVOKABLE bool isDeviceMapCheck() const;
    Q_INVOKABLE void setDeviceMapCheck(bool value);

    //Filter
    Q_INVOKABLE void multiConditionFilter(int state, int group, int remoteType);

    Q_INVOKABLE void setCurrentDevice(QString deviceName = "");

     Q_INVOKABLE void searchDevice(QString label = "");

     Q_INVOKABLE QObject* getDeviceObject(int index);

    //Q_PROPERTY
    QAbstractItemModel *cSourceModel() const;
    void setSourceModel(QAbstractItemModel *sourceModel) override;

signals:
    void deviceMapCheckChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    int m_state;
    int m_group;
	int m_remoteType;
    QString m_deviceName;
    QString m_searchLabel;
    bool m_deviceMapCheck;
    QAbstractItemModel *m_cSourceModel;
};

#endif // CUSPROXYMODEL_H
