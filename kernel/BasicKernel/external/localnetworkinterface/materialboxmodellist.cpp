#include <QDebug>
#include <QDateTime>
#include <QUrl>

#include "materialboxmodellist.h"
#include "Klipper4408Interface.h"

using namespace creative_kernel;

struct Color {
	int red;
	int green;
	int blue;
};

struct RGB {
	unsigned char r, g, b;
};

MaterialBoxListModel::MaterialBoxListModel(QObject* parent) : QAbstractListModel(parent)
{
	m_printerType = 0;
	m_bFirstConnect = false;
	m_webInfoRegExp = QRegExp("(^(?::[^:]*:?)+$)");
}

MaterialBoxListModel::~MaterialBoxListModel()
{
	getRemotePrinterManager()->setGetMaterialBoxCb(nullptr);
	this->disconnect();
}

int MaterialBoxListModel::rowCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
		return m_materialBoxItemList.count();
}

QVariant MaterialBoxListModel::data(const QModelIndex& index, int role) const
{
	if (index.row() < 0 || index.row() >= rowCount()) return QVariant();

	QVariant value;
	MaterialBoxModelItem* modelItem = m_materialBoxItemList.at(index.row());

	switch (role)
	{
	case MaterialBoxModelItem::E_MaterialBoxId:
		value = modelItem->materialBoxId();
		break;
	case MaterialBoxModelItem::E_MaterialBoxState:
		value = modelItem->materialBoxState();
		break;
	case MaterialBoxModelItem::E_MaterialBoxFilament:
		value = modelItem->materialBoxFilament();
		break;
	case MaterialBoxModelItem::E_MaterialId:
		value = modelItem->materialId();
		break;
	case MaterialBoxModelItem::E_MaterialColor:
		value = modelItem->materialColor();
		break;
	case MaterialBoxModelItem::E_MaterialType:
		value = modelItem->materialType();
		break;
	case MaterialBoxModelItem::E_Rfid:
		value = modelItem->rfid();
		break;
	default:
		break;
	}
	return value;
}
QHash<int, QByteArray> MaterialBoxListModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[MaterialBoxModelItem::E_MaterialBoxId] = "materialBoxId";
	roles[MaterialBoxModelItem::E_MaterialBoxState] = "materialBoxState";
	roles[MaterialBoxModelItem::E_MaterialBoxFilament] = "materialBoxFilament";
	roles[MaterialBoxModelItem::E_MaterialId] = "materialId";
	roles[MaterialBoxModelItem::E_MaterialColor] = "materialColor";
	roles[MaterialBoxModelItem::E_MaterialType] = "materialType";
	roles[MaterialBoxModelItem::E_Rfid] = "rfid";
	return roles;
}




void MaterialBoxListModel::materialColorMap(const QString& ip, QString path,const QList<QVariantMap>& colorMap)
{
	getRemotePrinterManager()->materialColorMap(ip, path, colorMap);
}


QString MaterialBoxListModel::getMaterialBoxId(const QString& macAddress, int index)
{
	if (m_MaterialBoxItemMap[macAddress].size() == 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].at(index) == nullptr) return QString();
	auto type = m_MaterialBoxItemMap[macAddress].at(index)->materialBoxId();
	return type;
}


QString MaterialBoxListModel::getMaterialType(const QString& macAddress,int index)
{
	if(m_MaterialBoxItemMap[macAddress].size()==0)return QString();
	if (m_MaterialBoxItemMap[macAddress].at(index) == nullptr) return QString();
	auto type = m_MaterialBoxItemMap[macAddress].at(index)->materialType();
	return type;
}

QString MaterialBoxListModel::getMaterialId(const QString& macAddress, int index)
{
	if (m_MaterialBoxItemMap[macAddress].size() == 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].at(index) == nullptr) return QString();
	auto type = m_MaterialBoxItemMap[macAddress].at(index)->materialId();
	return type;
}

QString MaterialBoxListModel::getMaterialColor(const QString& macAddress, int index)
{
	if (m_MaterialBoxItemMap[macAddress].size() == 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].at(index) == nullptr) return QString();
	auto type = m_MaterialBoxItemMap[macAddress].at(index)->materialColor();
	return type;
}

QStringList MaterialBoxListModel::getMaterialColorList(const QString& macAddress)
{
	if (m_MaterialBoxItemMap[macAddress].size() == 0)return QStringList();
	QStringList list;
	for (auto item: m_MaterialBoxItemMap[macAddress])
	{
		QString color = item->materialColor();
		list.push_back(color);

	}
	return list;
}

QString MaterialBoxListModel::getRfid(const QString& macAddress, int index)
{
	if (m_MaterialBoxItemMap[macAddress].at(index) == nullptr) return QString();
	auto type = m_MaterialBoxItemMap[macAddress].at(index)->rfid();
	return type;
}

void MaterialBoxListModel::findMaterialBoxList()
{
	if (!m_bFirstConnect)
	{
		m_bFirstConnect = true;
		connect(this, &MaterialBoxListModel::sigGetMaterialBoxList, this, &MaterialBoxListModel::slotGetMaterialBoxList, Qt::ConnectionType::QueuedConnection);
		getRemotePrinterManager()->setGetMaterialBoxCb(std::bind(&MaterialBoxListModel::onGetMaterialBoxList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}

double calculateColorDistance(const RGB& color1, const RGB& color2) {
	long rmean = ((long)color1.r + (long)color2.r) / 2;
	long r = (long)color1.r - (long)color2.r;
	long g = (long)color1.g - (long)color2.g;
	long b = (long)color1.b - (long)color2.b;
	return std::sqrt((((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8));
}

RGB parseHexColor(const QString& hexColor) {
    QString colorStr = hexColor.mid(1); // 去掉 "#" 符号
    bool ok;
    int red = colorStr.mid(0, 2).toInt(&ok, 16);
    int green = colorStr.mid(2, 2).toInt(&ok, 16);
    int blue = colorStr.mid(4, 2).toInt(&ok, 16);
    return RGB{static_cast<unsigned char>(red), static_cast<unsigned char>(green), static_cast<unsigned char>(blue)};
}

QString convertToHexColor(const RGB& color) {
	QString hexColor = "#";
	hexColor += QString::number(color.r, 16).rightJustified(2, '0');
	hexColor += QString::number(color.g, 16).rightJustified(2, '0');
	hexColor += QString::number(color.b, 16).rightJustified(2, '0');
	return hexColor;
}

QString MaterialBoxListModel::getCloseColorFromPrinter(const QString& hexColor, const QStringList hexColorList) {
	if (hexColorList.isEmpty()) {
		return "#000000";
	}

	RGB targetColor = parseHexColor(hexColor);
	RGB closestColor = parseHexColor(hexColorList.first());
	double closestDistance = calculateColorDistance(targetColor, closestColor);

	for (const QString& currentHexColor : hexColorList) {
		RGB currentColor = parseHexColor(currentHexColor);
		double currentDistance = calculateColorDistance(targetColor, currentColor);
		if (currentDistance < closestDistance) {
			closestColor = currentColor;
			closestDistance = currentDistance;
		}
	}

	return convertToHexColor(closestColor);
}

int MaterialBoxListModel::getCloseColorFromPrinter(const QString& hexColor)
{
	int closestIndex = 0;
	QString firstColor = m_materialBoxItemList.at(0)->materialColor();
	RGB targetColor = parseHexColor(hexColor);
	RGB closestColor = parseHexColor(firstColor);
	double closestDistance = calculateColorDistance(targetColor, closestColor);

	for (auto item : m_materialBoxItemList) {
		RGB currentColor = parseHexColor(item-> materialColor());
		double currentDistance = calculateColorDistance(targetColor, currentColor);
		if (currentDistance < closestDistance) {
			closestColor = currentColor;
			closestIndex++;
			closestDistance = currentDistance;
		}
	}
	return closestIndex;
}

void MaterialBoxListModel::switchDataSource(const QString& macAddress, RemotePrinerType remoteType, bool init)//@true: init @false: update
{
	static QString curMacAddress = QString();
	if (init) curMacAddress = macAddress;

	if (curMacAddress.compare(macAddress) == 0 && m_MaterialBoxItemMap.contains(curMacAddress))
	{
		m_printerType = int(remoteType);
		m_materialBoxItemList = m_MaterialBoxItemMap[curMacAddress];

		emit printerTypeChanged();
		emit layoutChanged();
	}
}

void MaterialBoxListModel::onGetMaterialBoxList(const std::string& std_macAddr, const std::string& std_list, RemotePrinerType remoteType)
{
	if (std_macAddr.empty()) return;
	if (!m_materialBoxMap.contains(std_macAddr) || m_materialBoxMap[std_macAddr].compare(std_list) != 0)
	{
		m_materialBoxMap[std_macAddr] = std_list;
		QString from_std_list = QString::fromStdString(std_list);
		QString from_std_macAddr = QString::fromStdString(std_macAddr);
		QList<MaterialBoxModelItem*> itemList;

		switch (remoteType)
		{
		case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
		{
			QStringList splitList = from_std_list.split(";", QString::SkipEmptyParts);
			foreach(QString fullInfo, splitList)
			{
				/*	if (m_webInfoRegExp.exactMatch(fullInfo))
					{*/
				QStringList splitItem = fullInfo.split(":");
				if (splitItem.size() >= 6) {
					if (splitItem[4] != "") {
						MaterialBoxModelItem* modelItem = new MaterialBoxModelItem(this);
						QString materialColor = "#" + splitItem[4].mid(2);
						QString materialBoxId = splitItem[0];
						QString materialBoxState = splitItem[1];
						QString materialId = splitItem[2];
						QString rfid = splitItem[3].mid(1);
						QString materialType = splitItem[5];


						modelItem->setMaterialBoxId(materialBoxId);
						modelItem->setMaterialBoxState(materialBoxState);
						//modelItem->setMaterialBoxFilament(materialBoxFilament);
						modelItem->setMaterialId(materialId);
						modelItem->setMaterialType(materialType);
						modelItem->setRfid(rfid);
						modelItem->setMaterialColor(materialColor);

						itemList.append(modelItem);

					}

				}
				//}
			}
		}
		break;
		default:
			break;
		}

		m_MaterialBoxItemMap[from_std_macAddr] = itemList;
		switchDataSource(from_std_macAddr, remoteType, true);
		//emit sigGetMaterialBoxList(from_std_macAddr, from_std_list, remoteType);
	}
}

void MaterialBoxListModel::slotGetMaterialBoxList(const QString& from_std_macAddr, const QString& from_std_list, RemotePrinerType remoteType)
{
	QList<MaterialBoxModelItem*> itemList;

	switch (remoteType)
	{
	case RemotePrinerType::REMOTE_PRINTER_TYPE_KLIPPER4408:
	{
		QStringList splitList = from_std_list.split(";", QString::SkipEmptyParts);
		foreach(QString fullInfo, splitList)
		{
			/*	if (m_webInfoRegExp.exactMatch(fullInfo))
				{*/
			QStringList splitItem = fullInfo.split(":");
			if (splitList.size() >= 5) {			
				if (splitItem[4]!="") {
					MaterialBoxModelItem* modelItem = new MaterialBoxModelItem(this);
					QString materialColor = "#" + splitItem[4].mid(1);
					QString materialBoxId = splitItem[0];
					QString materialBoxState = splitItem[1];
					//QString materialBoxFilament = m_webInfoRegExp.cap(3);
					QString materialId = splitItem[2];
					QString materialType = splitItem[3];
					

					modelItem->setMaterialBoxId(materialBoxId);
					modelItem->setMaterialBoxState(materialBoxState);
					//modelItem->setMaterialBoxFilament(materialBoxFilament);
					modelItem->setMaterialId(materialId);
					modelItem->setMaterialType(materialType);
					modelItem->setMaterialColor(materialColor);

					itemList.append(modelItem);

				}
			
		}
			//}
		}
	}
	break;
	default:
		break;
	}

	m_MaterialBoxItemMap[from_std_macAddr] = itemList;
	switchDataSource(from_std_macAddr, remoteType, false);
}

int MaterialBoxListModel::printerType() const
{
	return m_printerType;
}

MaterialBoxModelItem::MaterialBoxModelItem(QObject* parent) : QObject(parent)
{
	m_materialId = QString();
	m_materialColor = QString();
	m_materialType = QString();
}

MaterialBoxModelItem::MaterialBoxModelItem(const MaterialBoxModelItem& MaterialBoxModelItem) : QObject(nullptr)
{
	Q_UNUSED(MaterialBoxModelItem)
}

MaterialBoxModelItem::~MaterialBoxModelItem()
{

}
const QString MaterialBoxModelItem::materialBoxId() const
{
	return m_materialBoxId;
}

void MaterialBoxModelItem::setMaterialBoxId(const QString& materialBoxId)
{
	m_materialBoxId = materialBoxId;
	emit materialBoxIdChanged();
}
const QString MaterialBoxModelItem::materialBoxState() const
{
	return m_materialBoxState;
}

void MaterialBoxModelItem::setMaterialBoxState(const QString& materialBoxState)
{
	m_materialBoxState = materialBoxState;
	emit materialBoxStateChanged();
}
const QString MaterialBoxModelItem::materialBoxFilament() const
{
	return m_materialBoxFilament;
}

void MaterialBoxModelItem::setMaterialBoxFilament(const QString& materialBoxFilament)
{
	m_materialBoxFilament = materialBoxFilament;
	emit materialBoxFilamentChanged();
}

const QString MaterialBoxModelItem::materialId() const
{
	return m_materialId;
}

void MaterialBoxModelItem::setMaterialId(const QString& rfid)
{
	m_materialId = rfid;
	emit materialIdChanged();
}

const QString MaterialBoxModelItem::materialColor() const
{
	return m_materialColor;
}

void MaterialBoxModelItem::setMaterialColor(const QString& color)
{
	m_materialColor = color;
	emit materialColorChanged();
}

const QString MaterialBoxModelItem::materialType() const
{
	return m_materialType;
}

void MaterialBoxModelItem::setMaterialType(const QString& type)
{
	m_materialType = type;
	emit materialTypeChanged();
}

const QString MaterialBoxModelItem::rfid() const
{
	return m_rfid;
}

void MaterialBoxModelItem::setRfid(const QString& id)
{
	m_rfid = id;
	emit rfidChanged();
}

