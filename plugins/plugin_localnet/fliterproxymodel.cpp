#include "fliterproxymodel.h"
#include <variant>
FliterProxyModel::FliterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    m_state = 0;
    m_group = 1;
	m_remoteType = 0;
    m_deviceName = "";
    m_searchLabel = "";
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

QObject* FliterProxyModel::getDeviceObject(int index) {
    QObject* object = nullptr;
    QMetaObject::invokeMethod(m_cSourceModel, "getDeviceObject", Q_RETURN_ARG(QObject*, object), Q_ARG(int, this->index(index, 0).row()));
    return object;
}

bool FliterProxyModel::isDeviceMapCheck() const
{
    return m_deviceMapCheck;
}

void FliterProxyModel::setDeviceMapCheck(bool value)
{
    if (m_deviceMapCheck != value) {
        m_deviceMapCheck = value;
    }
    invalidateFilter();
}


void FliterProxyModel::searchDevice(QString label)
{
    if(m_searchLabel!= label)
        m_searchLabel = label; 
    invalidateFilter();
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
    QModelIndex ipIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    QModelIndex deviceNameIndex = sourceModel()->index(sourceRow, 0, sourceParent);

    int state = sourceModel()->data(stateIndex, ListModelItem::E_PrinterState).toInt();
    int group = sourceModel()->data(groupIndex, ListModelItem::E_GroupNo).toInt();
	int remoteType = sourceModel()->data(groupIndex, ListModelItem::E_PrinterType).toInt();
    QString name = sourceModel()->data(nameIndex, ListModelItem::E_PrinterModel).toString();
    QString ip = sourceModel()->data(ipIndex, ListModelItem::E_IpAddr).toString();
    QVariant modelItem = sourceModel()->data(deviceNameIndex, ListModelItem::E_DeviceName);
    ListModelItem* listItem = modelItem.value<ListModelItem*>();
    QString deviceName = listItem->deviceName();



	bool stateFliter = true;
	if (m_state) stateFliter = m_state - 1 ? (state == 1 || state == 5) : (state != 1 && state != 5);//1:��ӡ��(������ͣ) 0:����(������ӡ���/ʧ��/ֹͣ)

	if (remoteType == 3) remoteType = 0;//Klipper4408 -> Lan Machine
	bool remoteFilter = (m_remoteType != 1) ? (remoteType == m_remoteType) : true;//0:Lan Machine(����Klipper4408) 1:All 2:Sonic Pad
    bool nameFilter = (m_deviceName == "") ||(name == m_deviceName);


    QString lowerDeviceName = deviceName.toLower();
    QString lowerSearchLabel = m_searchLabel.toLower();
   
    bool searchFilter = (lowerDeviceName.contains(lowerSearchLabel) || ip.contains(m_searchLabel));


    bool filterCondition;
    if(m_deviceMapCheck)
        filterCondition = remoteFilter && stateFliter && group == m_group && nameFilter && searchFilter;
    else
        filterCondition = remoteFilter && stateFliter && group == m_group && searchFilter;
    return (filterCondition);
}
