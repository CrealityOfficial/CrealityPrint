#ifndef SORTPROXYMODEL_H
#define SORTPROXYMODEL_H

#include "gcodelistmodel.h"
#include "historylistmodel.h"

#include <QObject>
#include <QSortFilterProxyModel>

class SortProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	Q_PROPERTY(QAbstractItemModel* cSourceModel READ cSourceModel CONSTANT)
	explicit SortProxyModel(QObject* parent = nullptr);

	//Q_PROPERTY
	QAbstractItemModel* cSourceModel() const;
	void setSourceModel(QAbstractItemModel* sourceModel) override;

	Q_INVOKABLE void setSort(int index, int role);

	
protected:
	bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

private:
	bool sortByTime(const QModelIndex&, const QModelIndex&);
	bool sortByFileSize(const QModelIndex&, const QModelIndex&);
	bool sortByFileName(const QModelIndex&, const QModelIndex&);
	QAbstractItemModel* m_cSourceModel;
};

#endif // SORTPROXYMODEL_H