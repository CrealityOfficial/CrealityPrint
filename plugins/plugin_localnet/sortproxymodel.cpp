#include "sortproxymodel.h"
#include <QDateTime>

std::function<bool(const QModelIndex&, const QModelIndex&)> sortOrderProxy;

SortProxyModel::SortProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
{
	setDynamicSortFilter(true);
	sortOrderProxy = std::bind(&SortProxyModel::sortByTime, this, std::placeholders::_1, std::placeholders::_2);
	sort(0, Qt::AscendingOrder);
}

QAbstractItemModel* SortProxyModel::cSourceModel() const
{
	return m_cSourceModel;
}

void SortProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
	m_cSourceModel = sourceModel;
	QSortFilterProxyModel::setSourceModel(sourceModel);
}

void SortProxyModel::setSort(int index, int role)
{
	switch (role)
	{
	case GcodeModelItem::E_GCodeFileTime:
		sortOrderProxy = std::bind(&SortProxyModel::sortByTime, this, std::placeholders::_1, std::placeholders::_2);
		break;
	case GcodeModelItem::E_GCodeFileSize:
		sortOrderProxy = std::bind(&SortProxyModel::sortByFileSize, this, std::placeholders::_1, std::placeholders::_2);
		break;
	case GcodeModelItem::E_GCodeFileName:
		sortOrderProxy = std::bind(&SortProxyModel::sortByFileName, this, std::placeholders::_1, std::placeholders::_2);
		break;
	default:
		break;
	}

	if (sortOrder() == Qt::AscendingOrder)
	{
		sort(index, Qt::DescendingOrder);
	}
	else {
		sort(index, Qt::AscendingOrder);
	}
}

bool SortProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
{
	return sortOrderProxy(source_left, source_right);
}

bool SortProxyModel::sortByTime(const QModelIndex& source_left, const QModelIndex& source_right)
{
	QString creationTime0 = sourceModel()->data(source_left, GcodeModelItem::E_GCodeFileTime).toString();
	QString creationTime1 = sourceModel()->data(source_right, GcodeModelItem::E_GCodeFileTime).toString();

	QDateTime dateTime0 = QDateTime::fromString(creationTime0, "yyyy-MM-dd hh:mm:ss");
	QDateTime dateTime1 = QDateTime::fromString(creationTime1, "yyyy-MM-dd hh:mm:ss");
	return dateTime0 > dateTime1;
}

bool SortProxyModel::sortByFileSize(const QModelIndex& source_left, const QModelIndex& source_right)
{
	float fileSize0 = sourceModel()->data(source_left, GcodeModelItem::E_GCodeFileSize).toFloat();
	float fileSize1 = sourceModel()->data(source_right, GcodeModelItem::E_GCodeFileSize).toFloat();


	return fileSize0 > fileSize1;
}

bool SortProxyModel::sortByFileName(const QModelIndex& source_left, const QModelIndex& source_right)
{
	QString fileSize0 = sourceModel()->data(source_left, GcodeModelItem::E_GCodeFileName).toString();
	QString fileSize1 = sourceModel()->data(source_right, GcodeModelItem::E_GCodeFileName).toString();


	return fileSize0 > fileSize1;
}

