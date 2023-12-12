#include "fliterproxymodel.h"
FliterProxyModel::FliterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    m_state = 0;
    m_group = 1;
	m_remoteType = 0;
    m_deviceName = "";
}

void FliterProxyModel::multiConditionFilter(int state, int group, int remoteType)
{
    m_state = state;
    m_group = group;
	m_remoteType = remoteType;
    invalidateFilter();
}

QAbstractItemModel * FliterProxyModel::cSourceModel() const
{
    return m_cSourceModel;
}

void FliterProxyModel::setCurrentDevice(QString deviceName) {

    m_deviceName = deviceName;
    setDeviceMapCheck(true);
}

bool FliterProxyModel::isDeviceMapCheck() const
{
    return m_deviceMapCheck;
}

void FliterProxyModel::setDeviceMapCheck(bool value)
{
    if (m_deviceMapCheck != value) {
        m_deviceMapCheck = value;
        invalidateFilter();
    }
}

void FliterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    m_cSourceModel = sourceModel;
	QSortFilterProxyModel::setSourceModel(sourceModel);
}

bool FliterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    
    QModelIndex stateIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    QModelIndex groupIndex= sourceModel()->index(sourceRow, 0, sourceParent);
    QModelIndex nameIndex = sourceModel()->index(sourceRow, 0, sourceParent);

    int state = sourceModel()->data(stateIndex, ListModelItem::E_PrinterState).toInt();
    int group = sourceModel()->data(groupIndex, ListModelItem::E_GroupNo).toInt();
	int remoteType = sourceModel()->data(groupIndex, ListModelItem::E_PrinterType).toInt();
    QString name = sourceModel()->data(nameIndex, ListModelItem::E_PrinterModelName).toString();
	bool stateFliter = true;
	if (m_state) stateFliter = m_state - 1 ? (state == 1 || state == 5) : (state != 1 && state != 5);//1:´òÓ¡ÖÐ(°üÀ¨ÔÝÍ£) 0:¿ÕÏÐ(°üÀ¨´òÓ¡Íê³É/Ê§°Ü/Í£Ö¹)

	if (remoteType == 3) remoteType = 0;//Klipper4408 -> Lan Printer
	bool remoteFilter = (m_remoteType != 1) ? (remoteType == m_remoteType) : true;//0:Lan Printer(°üÀ¨Klipper4408) 1:All 2:Sonic Pad
    bool nameFilter = (m_deviceName == "") ||(name == m_deviceName);

    bool filterCondition;
    if(m_deviceMapCheck)
        filterCondition = remoteFilter && stateFliter && group == m_group && nameFilter;
    else
        filterCondition = remoteFilter && stateFliter && group == m_group;
    return (filterCondition);
}
