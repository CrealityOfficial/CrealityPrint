#ifndef FLITERPROXYMODEL_H
#define FLITERPROXYMODEL_H

#include "cuslistmodel.h"
#include "localnetworkinterface/RemotePrinterManager.h"

#include <QObject>
#include <QSortFilterProxyModel>

class FliterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    Q_PROPERTY(QAbstractItemModel* cSourceModel READ cSourceModel CONSTANT)
    explicit FliterProxyModel(QObject *parent = nullptr);

    //Filter
    Q_INVOKABLE void multiConditionFilter(int state, int group, int remoteType);

    //Q_PROPERTY
    QAbstractItemModel *cSourceModel() const;
    void setSourceModel(QAbstractItemModel *sourceModel) override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    int m_state;
    int m_group;
	int m_remoteType;
    QAbstractItemModel *m_cSourceModel;
};

#endif // CUSPROXYMODEL_H
