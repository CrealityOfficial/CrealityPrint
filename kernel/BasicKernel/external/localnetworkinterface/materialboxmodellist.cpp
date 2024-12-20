#include <QDebug>
#include <QDateTime>
#include <QUrl>
#include <QColor>
#include "materialboxmodellist.h"
#include "Klipper4408Interface.h"
#include <QtGui/QDesktopServices>
#include <qchar.h>
#include <qglobal.h>
#include <qlist.h>
#include "interface/appsettinginterface.h"
#include "colorDistance.c"
#include <cmath>
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
	m_materialBoxItemList = QList<MaterialBoxModelItem*>();
	m_autoRefillInfoListModel = std::make_unique<AutoRefillInfoListModel>();
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
	case MaterialBoxModelItem::E_Name:
		value = modelItem->name();
		break;
	case MaterialBoxModelItem::E_Vendor:
		value = modelItem->vendor();
		break;
	case MaterialBoxModelItem::E_MinTemp:
		value = modelItem->minTemp();
		break;
	case MaterialBoxModelItem::E_MaxTemp:
		value = modelItem->maxTemp();
		break;
	case MaterialBoxModelItem::E_Pressure:
		value = modelItem->pressure();
		break;
	case MaterialBoxModelItem::E_Selected:
		value = modelItem->selected();
		break;
	case MaterialBoxModelItem::E_State:
		value = modelItem->state();
		break;
	case MaterialBoxModelItem::E_Percent:
		value = modelItem->percent();
		break;
	case MaterialBoxModelItem::E_Used:
		value = modelItem->used();
		break;
	case MaterialBoxModelItem::E_EditStatus:
		value = modelItem->editStatus();
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
	roles[MaterialBoxModelItem::E_Vendor] = "vendor";
	roles[MaterialBoxModelItem::E_Name] = "name";
	roles[MaterialBoxModelItem::E_MinTemp] = "minTemp";
	roles[MaterialBoxModelItem::E_MaxTemp] = "maxTemp";
	roles[MaterialBoxModelItem::E_Pressure] = "pressure";
	roles[MaterialBoxModelItem::E_Selected] = "selected";
	roles[MaterialBoxModelItem::E_State] = "state";
	roles[MaterialBoxModelItem::E_Percent] = "percent";
	roles[MaterialBoxModelItem::E_Used] = "used";
	roles[MaterialBoxModelItem::E_EditStatus] = "editStatus";
	return roles;
}




void MaterialBoxListModel::materialColorMap(const QString& ip, QString path,const QList<QVariantMap>& colorMap)
{
	getRemotePrinterManager()->materialColorMap(ip, path, colorMap);
}


QString MaterialBoxListModel::getMaterialBoxId(const QString& macAddress, int index)
{
	if (m_MaterialBoxItemMap[macAddress].count() < index + 1)
	{
		return QString();
	}
	if (index < 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].size() == 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].at(index) == nullptr) return QString();
	auto type = m_MaterialBoxItemMap[macAddress].at(index)->materialBoxId();
	return type;
}



QString MaterialBoxListModel::getMaterialType(const QString& macAddress,int index)
{
	if (m_MaterialBoxItemMap[macAddress].count() < index + 1)
	{
		return QString();
	}
	if (index < 0)return QString();
	if(m_MaterialBoxItemMap[macAddress].size()==0)return QString();
	if (m_MaterialBoxItemMap[macAddress].at(index) == nullptr) return QString();
	auto type = m_MaterialBoxItemMap[macAddress].at(index)->materialType();
	return type;
}

QString MaterialBoxListModel::getMaterialName(const QString& macAddress, int index)
{
	if (m_MaterialBoxItemMap[macAddress].count() < index + 1)
	{
		return QString();
	}
	if (index < 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].size() == 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].at(index) == nullptr) return QString();
	auto type = m_MaterialBoxItemMap[macAddress].at(index)->name();
	return type;
}

void MaterialBoxListModel::getMaterialSelected(int boxId, int materialId)
{
	if (m_materialBoxItemList.size() == 0)return;
	for (MaterialBoxModelItem* item : m_materialBoxItemList)
	{
		if (item->materialBoxId() == QString::number(boxId) && item->materialId() == QString::number(materialId))
		{
			item->setSelected(true);
		}
		else
		{
			item->setSelected(false);
		}
	}
	emit dataChanged(createIndex(0, 0), createIndex(rowCount()-1, 0), { MaterialBoxModelItem::E_Selected });
	emit materialSelected();
}
int MaterialBoxListModel::getCurrentMaterialState()
{
	if (m_materialBoxItemList.size() == 0)return 1; //只使用外挂耗材，永远使能
	for (MaterialBoxModelItem* item : m_materialBoxItemList)
	{
		if(item->selected())
		{
			return item->state().toInt();
		}
	}
	return 0;
}
QString MaterialBoxListModel::getMaterialId(const QString& macAddress, int index)
{
	if (m_MaterialBoxItemMap[macAddress].count() < index + 1)
	{
		return QString();
	}
	if (index < 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].size() == 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].at(index) == nullptr) return QString();
	auto type = m_MaterialBoxItemMap[macAddress].at(index)->materialId();
	return type;
}
QString MaterialBoxListModel::getMaterialState(const QString& macAddress, int index)
{
	if (m_MaterialBoxItemMap[macAddress].count() < index + 1)
	{
		return QString();
	}
	if (index < 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].size() == 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].at(index) == nullptr) return QString();
	auto type = m_MaterialBoxItemMap[macAddress].at(index)->state();
	return type;
}



QColor MaterialBoxListModel::getMaterialColor(const QString& macAddress, int index)
{
	if (m_MaterialBoxItemMap[macAddress].count() < index + 1)
	{
		return QColor();
	}
	if (index < 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].size() == 0)return QString();
	if (m_MaterialBoxItemMap[macAddress].at(index) == nullptr) return QString();
	auto type = m_MaterialBoxItemMap[macAddress].at(index)->materialColor();
	return type;
}

QList<QColor> MaterialBoxListModel::getMaterialColorList(const QString& macAddress)
{

	if (m_MaterialBoxItemMap[macAddress].size() == 0)return QList<QColor>();
	QList<QColor> list;
	for (auto item: m_MaterialBoxItemMap[macAddress])
	{
		QColor color = item->materialColor();
		list.append(color);

	}
	return list;
}

void MaterialBoxListModel::toMultiColorGuide()
{
	QString urlStr = multiColorGuide();
	QDesktopServices::openUrl(QUrl(urlStr));

}

void MaterialBoxListModel::feedInOrOutMaterial(const QString& ip, const QString& boxId, const QString& materialId, const QString& isFeed)
{
	getRemotePrinterManager()->feedInOrOutMaterial(ip.toStdString(), boxId.toStdString(), materialId.toStdString(), isFeed.toStdString());
}

void MaterialBoxListModel::refreshBox(const QString& ip, const QString& boxId, const QString& materialId)
{
	getRemotePrinterManager()->refreshBox(ip.toStdString(), boxId.toStdString(), materialId.toStdString());
}

void MaterialBoxListModel::boxConfig(const QString& ip, const QString& autoRefill, const QString& cAutoFeed, const QString& cSelfTest)
{
	getRemotePrinterManager()->boxConfig(ip.toStdString(), autoRefill.toStdString(), cAutoFeed.toStdString(), cSelfTest.toStdString());
}

void MaterialBoxListModel::getColorMap(const QString& ip, const QString& filePath)
{
	getRemotePrinterManager()->getColorMap(ip.toStdString(), filePath.toStdString());
}

void MaterialBoxListModel::modifyMaterial(const QString& ip, const QVariantMap& materialParams)
{
	getRemotePrinterManager()->modifyMaterial(ip.toStdString(), materialParams);
}

static bool almostEqual(double a, double b, double epsilon = 1e-9) {
	return std::fabs(a - b) < epsilon;
}

bool MaterialBoxListModel::isRemoteModifyMaterial(const QString& ip, const QVariantMap& params)
{
	int id = params.value("id").toInt();
	int boxId = params.value("boxId").toInt();
	std::string rfid = params.value("rfid").toString().toStdString();
	std::string type = params.value("type").toString().toStdString();
	std::string vendor = params.value("vendor").toString().toStdString();
	std::string name = params.value("name").toString().toStdString();
	std::string color = params.value("color").toString().toStdString();
	double minTemp = params.value("minTemp").toDouble();
	double maxTemp = params.value("maxTemp").toDouble();
	double pressure = params.value("pressure").toDouble();

	for(int i=0; i< m_materialBoxItemList.size(); i++)
	{
		MaterialBoxModelItem *item = m_materialBoxItemList[i];
		int mat_id = item->materialId().toInt();
		int mat_box_id = item->materialBoxId().toInt();

		if (boxId == 0)//shelf
		{
			if (vendor != this->m_vendor.toStdString()
				|| type != this->m_type.toStdString()
				|| name != this->m_name.toStdString()
				|| QColor(color.c_str()) != this->m_color
				|| !almostEqual(minTemp, this->m_minTemp.toDouble())
				|| !almostEqual(maxTemp, this->m_maxTemp.toDouble())
				// 					|| !almostEqual(pressure, this->m_pressure.toDouble())
				// 					|| rfid != this->rfid.toStdString()
				)
			{
				return true;
			}
		}
		else if (mat_id == id &&  mat_box_id == boxId)
		{
			if (vendor != item->vendor().toStdString()
				|| type != item->materialType().toStdString()
				|| name != item->name().toStdString()
				|| QColor(color.c_str()) != item->materialColor()
				|| !almostEqual(minTemp, item->minTemp().toDouble())
				|| !almostEqual(maxTemp, item->maxTemp().toDouble())
				|| !almostEqual(pressure, item->pressure().toDouble())
				|| rfid != item->rfid().toStdString()
				)
			{
				return true;
			}
		}
	}

	return false;
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
		//getRemotePrinterManager()->setGetMaterialBoxCb(std::bind(&MaterialBoxListModel::onGetMaterialBoxList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	}
}

double calculateColorDistance(const RGB& color1, const RGB& color2) {
	
	long r1 = color1.r, g1 = color1.g, b1 = color1.b; // White
    long r2 = color2.r, g2 = color2.g, b2 = color2.b; // Gray

    double xyz1[3], xyz2[3];
    double lab1[3], lab2[3];

    // Convert from RGB to XYZ
    rgb_to_xyz(r1 / 255.0, g1 / 255.0, b1 / 255.0, xyz1);
    rgb_to_xyz(r2 / 255.0, g2 / 255.0, b2 / 255.0, xyz2);

    // Convert from XYZ to Lab
    xyz_to_lab(xyz1[0], xyz1[1], xyz1[2], lab1);
    xyz_to_lab(xyz2[0], xyz2[1], xyz2[2], lab2);


    return  DeltaE2000(lab1, lab2, KL_DEFULT, KC_DEFULT, KH_DEFULT);

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
void MaterialBoxListModel::GenCloseGCodeColors2(const QString colors, const QString types)
{
	QStringList usedColors;
	QStringList usedTypes;
	QStringList usedIndexs;
	QStringList colorsList = colors.split(";");
	QStringList typesList = types.split(";");
	int usedIndex = 0;
	for(int i=0;i<colorsList.length();++i)
	{	
		if(colorsList[i]!="")
		{
			usedColors.append(colorsList[i]);
			usedTypes.append(typesList[i]);
			usedIndexs.append(QString::number(usedIndex));
			usedIndex++;
		}
	}
	GenCloseGCodeColors(usedColors,usedTypes,usedIndexs);
}
void MaterialBoxListModel::GenCloseGCodeColors(const QStringList usedColors, const QStringList usedTypes,QStringList usedIndexs)
{
	if(usedColors.size() == 0 || usedTypes.size() == 0)
	{
		return;
	}
	//QMap<int,int> closestIndexs; // key: material index, value: color index
	m_closestIndexs.clear();
	QMap<int,double> matchedIndexs;
	QSet<int> matchedAgianIndexs;
	int currentIndex = 0;
	for (auto item : m_materialBoxItemList) {
			QColor color = item->materialColor();
			QString mapType = item->materialType();
			QString mapState = item->state();

			int bestIndex = -1;
			double closestDistance = 1000000;
			for(int i=0;i<usedColors.length();++i)
			{
				if(matchedIndexs.contains(i))
				{ //颜色已经匹配跳过
					continue;
				}
				QString usedColor = usedColors[i];
				QString usedType = usedTypes[i];
				if (color == Qt::transparent || mapState == "0" || mapType != usedType) {

					continue;
				}
				if(!usedIndexs.contains(QString::number(i)))
				{
					continue;
				}
				QString itemColor = color.name();
				RGB targetColor = parseHexColor(usedColor);
				RGB currentColor = parseHexColor(itemColor);
				double currentDistance = calculateColorDistance(targetColor, currentColor);
				if (currentDistance < closestDistance) {
						closestDistance = currentDistance;
						bestIndex = i;
					}
			}
			
			if(bestIndex != -1&&!matchedIndexs.contains(bestIndex)) //找到最匹配的颜色且这个颜色没有被匹配过
			{
				m_closestIndexs[currentIndex] = bestIndex;
				matchedIndexs.insert(bestIndex,closestDistance);
			}
			//}else{
				//在已匹配的列表钟查询是否有更匹配的颜色
				for (auto it = matchedIndexs.cbegin(); it != matchedIndexs.cend(); ++it) {
        			//不和自己比较
					if(it.key() == bestIndex)
					{
						continue;
					}

					QString usedColor = usedColors[it.key()];
					QString usedType = usedTypes[it.key()];
					//耗材类型不同跳过比较
					if(mapType != usedType)
					{
						continue;
					}
					RGB targetColor = parseHexColor(usedColor);
					RGB currentColor = parseHexColor(color.name());
					double currentDistance = calculateColorDistance(targetColor, currentColor);
					if (currentDistance < it.value()) {
						//查找到更匹配的，删除原来的匹配
						int agianIndex = -1;
						auto cit = m_closestIndexs.begin();
						while (cit != m_closestIndexs.end()) {
							if (cit.value() == it.key()) {
								agianIndex = cit.key();
								cit = m_closestIndexs.erase(cit); 
								
							} else {
								++cit;
        				}
						}
						//交换位置
						m_closestIndexs.insert(agianIndex,bestIndex);
						m_closestIndexs.insert(currentIndex,it.key());
						matchedIndexs.insert(it.key(),currentDistance);
						break;
					}
    			}
				//matchedAgianIndexs.insert(currentIndex);

			//}
			currentIndex++;
		}
	
	  for (auto it = m_closestIndexs.cbegin(); it != m_closestIndexs.cend(); ++it) {
        qDebug() << it.key() << ": " << it.value();
    }

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
int MaterialBoxListModel::getCloseColorByIndex(int colorIndex)
{
	for (auto it = m_closestIndexs.cbegin(); it != m_closestIndexs.cend(); ++it) 
	{
		if (it.value() == colorIndex)
			return it.key();
	}
	return -1;
}
int MaterialBoxListModel::getCloseColorFromPrinter(const QString& hexColor, const QString& type)
{
	if (m_materialBoxItemList.size() <= 0) return 0;
	int initIndex = 0;
	for (size_t i = 0; i < m_materialBoxItemList.size(); ++i) {
		if (m_materialBoxItemList.at(i)->materialColor() != Qt::transparent&& m_materialBoxItemList.at(i)->materialType()== type) {
			initIndex = i;
		}
	}
	int closestIndex = initIndex;
	QColor colorItem = m_materialBoxItemList.at(initIndex)->materialColor();
	QString firstColor = colorItem.name();
	RGB targetColor = parseHexColor(hexColor);
	RGB closestColor = parseHexColor(firstColor);
	double closestDistance = calculateColorDistance(targetColor, closestColor);

	int currentIndex = 0;
	bool isMap = false;
	for (auto item : m_materialBoxItemList) {
		QColor color = item->materialColor();
		QString mapType = item->materialType();
		QString mapState = item->state();

		if (color == Qt::transparent || mapState == "0" || mapType != type) {

			currentIndex++;
			continue;
		}
		isMap = true;
		QString itemColor = color.name();
		RGB currentColor = parseHexColor(itemColor);
		double currentDistance = calculateColorDistance(targetColor, currentColor);
		if (currentDistance < closestDistance) {
			closestColor = currentColor;
			closestDistance = currentDistance;
			closestIndex = currentIndex;
		}
		currentIndex++;
	}
	if (!isMap) return -1;
	return closestIndex;
}

void MaterialBoxListModel::switchDataSource(const QString& macAddress, RemotePrinerType remoteType, bool init)//@true: init @false: update
{
	static QString curMacAddress = QString();
	if (init) curMacAddress = macAddress;
	if (curMacAddress.compare(macAddress) == 0 && m_MaterialBoxItemMap.contains(curMacAddress))
	{
		m_printerType = int(remoteType);
		beginResetModel();
		m_materialBoxItemList = m_MaterialBoxItemMap[curMacAddress];
		endResetModel();
		//dataChanged(createIndex(0, 0), createIndex(rowCount(), 0), { MaterialBoxModelItem::E_MaterialColor,MaterialBoxModelItem::E_MaterialType, MaterialBoxModelItem::E_State, MaterialBoxModelItem::E_Percent });

		emit printerTypeChanged();
		//emit layoutChanged();
	}
}
 bool MaterialBoxListModel::isOnlyRackMaterial(const QString& ip)
 {
	if(m_MaterialBoxItemMap.contains(ip))
	{
		auto& list = m_MaterialBoxItemMap[ip];
		if(list.size()>0)
		{
			return false;
		}

	}
	return true;
 }
void MaterialBoxListModel::onGetMaterialState(const QString& macAddr, MaterialState state, RemotePrinerType remoteType)
{
	if (macAddr.isEmpty()|| !m_MaterialBoxItemMap.contains(macAddr))
		return;
	if(m_boxMaterialUpdateTime==state.updateTime)
	{
		return;
	}
	m_boxMaterialUpdateTime = state.updateTime;
	if(state.boxId == 0)
	{
		setUsed(state.selected == 1);
	}
	auto& list = m_MaterialBoxItemMap[macAddr];
	for(int i=0;i< list.size();++i)
	{
		auto item = list[i];
		item->setUsed(false);
		if(item->materialBoxId()== QString::number(state.boxId) && item->materialId()== QString::number(state.materialId))
		{
			item->setPercent(QString::number(state.percent));
			item->setState(QString::number(state.state));
			item->setUsed(state.selected == 1);
			item->setEditStatus(QString::number(state.editStatus));
		}
	}
	dataChanged(createIndex(0, 0), createIndex(rowCount(), 0), { MaterialBoxModelItem::E_MaterialColor,MaterialBoxModelItem::E_MaterialType, MaterialBoxModelItem::E_State, MaterialBoxModelItem::E_Percent ,
		MaterialBoxModelItem::E_Used,MaterialBoxModelItem::E_EditStatus});
}

void MaterialBoxListModel::onGetBoxConfig(const std::string& std_macAddr, BoxConfig state, RemotePrinerType remoteType)
{
	if(m_boxConfigUpdateTime==state.updateTime)
	{
		return;
	}
	m_boxConfigUpdateTime = state.updateTime;
	if (m_materialBoxItemList.size() == 0)return;
	 setAutoRefill(QString::number(state.autoRefill));
	 setCAutoFeed(QString::number(state.cAutoFeed));
	 setCSelfTest(QString::number(state.cSelfTest));
}

void MaterialBoxListModel::onGetColorMap(const std::string& std_macAddr, ColorMapInfo state, RemotePrinerType remoteType)
{
	if (m_materialBoxItemList.size() == 0)return;
	for (MaterialBoxModelItem* item : m_materialBoxItemList)
	{
		for (auto it = state.ColorMapList.begin(); it != state.ColorMapList.end(); ++it) {
			if (it->boxId == item->materialBoxId() && it->materialId == item->materialId())
			{
				QColor color(QString::fromStdString(it->color));
				item->setMapColor(color);
				item->setMapType(QString::fromStdString(it->type));
				break;
			}
		}
	}

}

void MaterialBoxListModel::onGetGCodeColors(const std::string& std_macAddr, ColorMapInfo state, RemotePrinerType remoteType)
{
	m_materialBoxItemList.clear();
	for (auto it = state.ColorMapList.begin(); it != state.ColorMapList.end(); ++it) {
			MaterialBoxModelItem* item = new MaterialBoxModelItem();
			QColor color(QString::fromStdString(it->color));
			item->setMapColor(color);
			item->setMapType(QString::fromStdString(it->type));
			m_materialBoxItemList.push_back(item);
	}
}


void MaterialBoxListModel::onGetMaterialBoxList(const std::string& std_macAddr, const BoxsInfo& std_list, RemotePrinerType remoteType)
{
	if (std_macAddr.empty())
		return;
	if(m_boxUpdateTime == std_list.updateTime)
	{
		return;
	}
	m_boxUpdateTime = std_list.updateTime;
	QString from_std_macAddr = QString::fromStdString(std_macAddr);

	m_sameMaterial = QString::fromStdString(std_list.sameMaterial);
	m_autoRefillInfoListModel->pressJsonData(m_sameMaterial);

	QList<MaterialBoxModelItem*> itemList;
	for (auto & box1:std_list.materialBoxs)
	{
		if (box1.materials.size() > 0) {
            if (box1.id == 0) {//rack
                setVendor(QString::fromStdString(box1.materials[0].vendor));
                setType(QString::fromStdString(box1.materials[0].type));
                QColor materialColor;
                if (box1.materials[0].color.empty()) {
                    materialColor = Qt::white;
                    setRack(false);
                }
                else {
                    materialColor = "#" + QString::fromStdString(box1.materials[0].color.substr(2));
                    setRack(true);
                }
                setColor(materialColor);
                setName(QString::fromStdString(box1.materials[0].name));
                setMinTemp(QString::number(box1.materials[0].minTemp));
                setMaxTemp(QString::number(box1.materials[0].maxTemp));
                setEditStatus(QString::number(box1.materials[0].editStatus));
				setRfid(QString::fromStdString(box1.materials[0].rfid));
				setUsed(box1.materials[0].selected == 1);
            }
			else
			{
				for (Material item : box1.materials) {
					MaterialBoxModelItem* modelItem = new MaterialBoxModelItem(this);
					modelItem->setMaterialBoxId(QString::number(box1.id));
					modelItem->setMaterialBoxState(QString::number(box1.state));
					if (box1.humidity > 0)
						setHumidity(QString::number(box1.humidity));
					modelItem->setMaterialId(QString::number(item.id));
					modelItem->setMaterialType(QString::fromStdString(item.type));
					modelItem->setRfid(QString::fromStdString(item.rfid));
					QColor materialColor;
					if (item.color.empty())
						materialColor = Qt::transparent; // 使用白色作为默认值
					else
						materialColor = "#" + QString::fromStdString(item.color.substr(2));
					modelItem->setMaterialColor(materialColor);
					modelItem->setName(QString::fromStdString(item.name));
					modelItem->setVendor(QString::fromStdString(item.vendor));
					modelItem->setMinTemp(QString::number(item.minTemp));
					modelItem->setMaxTemp(QString::number(item.maxTemp));
					modelItem->setPressure(QString::number(item.pressure));
					modelItem->setState(QString::number(item.state));
					modelItem->setPercent(QString::number(item.percent));
					modelItem->setUsed(item.selected == 1);
					modelItem->setEditStatus(QString::number(item.editStatus));
					itemList.append(modelItem);
				}
			}
		}
	}

	m_MaterialBoxItemMap[from_std_macAddr] = itemList;
	switchDataSource(from_std_macAddr, remoteType, true);
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

QString MaterialBoxListModel::autoRefill() const
{
	return m_autoRefill;
}

void MaterialBoxListModel::setAutoRefill(QString autoRefill)
{
	m_autoRefill = autoRefill;
	emit autoRefillChanged();
}

QString MaterialBoxListModel::cAutoFeed() const
{
	return m_cAutoFeed;
}

void MaterialBoxListModel::setCAutoFeed(QString cAutoFeed)
{
	m_cAutoFeed = cAutoFeed;
	emit cAutoFeedChanged();
}

QString MaterialBoxListModel::cSelfTest() const
{
	return m_cSelfTest;
}

void MaterialBoxListModel::setCSelfTest(QString cSelfTest)
{
	m_cSelfTest = cSelfTest;
	emit cSelfTestChanged();
}

QString MaterialBoxListModel::humidity() const
{
	return m_humidity;
}

void MaterialBoxListModel::setHumidity(QString humidity)
{
	m_humidity = humidity;
	emit humidityChanged();
}

QString MaterialBoxListModel::editStatus() const
{
	return m_editStatus;
}
bool MaterialBoxListModel::used() const
{
	return m_used;
}
void MaterialBoxListModel::setUsed(bool used)
{
	m_used = used;
	emit usedChanged();
}
void MaterialBoxListModel::setEditStatus(QString editStatus)
{
	m_editStatus = editStatus;
	emit editStatusChanged();
}

bool MaterialBoxListModel::rack() const
{
	return m_rack;
}

void MaterialBoxListModel::setRack(bool rack)
{
	m_rack = rack;
	emit rackChanged();
}
QString MaterialBoxListModel::rfid() const
{
	return m_rfid;
}
void MaterialBoxListModel::setRfid(QString rfid)
{
	m_rfid = rfid;
	emit rfidChanged();
}

QString MaterialBoxListModel::vendor() const
{
	return m_vendor;
}

void MaterialBoxListModel::setVendor(QString vendor)
{
	m_vendor = vendor;
	emit vendorChanged();
}

QString MaterialBoxListModel::type() const
{
	return m_type;
}

void MaterialBoxListModel::setType(QString type)
{
	m_type = type;
	emit typeChanged();
}

QColor MaterialBoxListModel::color() const
{
	return m_color;
}

void MaterialBoxListModel::setColor(QColor color)
{
	m_color = color;
	emit colorChanged();
}

QString MaterialBoxListModel::name() const
{
	return m_name;
}

void MaterialBoxListModel::setName(QString name)
{
	m_name = name;
	emit nameChanged();
}

QString MaterialBoxListModel::minTemp() const
{
	return m_minTemp;
}

void MaterialBoxListModel::setMinTemp(QString minTemp)
{
	m_minTemp = minTemp;
	emit minTempChanged();
}

QString MaterialBoxListModel::maxTemp() const
{
	return m_maxTemp;
}

void MaterialBoxListModel::setMaxTemp(QString maxTemp)
{
	m_maxTemp = maxTemp;
	emit maxTempChanged();
}

void MaterialBoxListModel::initRack() {
	setVendor("");
	setType("");
	setColor(Qt::transparent);
	setRack(false);
}

QObject* MaterialBoxListModel::getAutoRefillListModel() {
	return m_autoRefillInfoListModel.get();
}


//QString MaterialBoxListModel::cAutoUpdateFilament() const
//{
//	return m_cAutoUpdateFilament;
//}
//
//void MaterialBoxListModel::setCAutoUpdateFilament(QString cAutoUpdateFilament)
//{
//	m_cAutoUpdateFilament = cAutoUpdateFilament;
//	emit cAutoUpdateFilamentChanged();
//}



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

 QColor MaterialBoxModelItem::materialColor() const
{
	return m_materialColor;
}

void MaterialBoxModelItem::setMaterialColor(const QColor& color)
{
	m_materialColor = color;
	emit materialColorChanged();
}

 QString MaterialBoxModelItem::materialType() const
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

const QString MaterialBoxModelItem::name() const
{
	return m_name;
}

void MaterialBoxModelItem::setName(const QString& name)
{
	m_name = name;
	emit nameChanged();
}
const QString MaterialBoxModelItem::vendor() const
{
	return m_vendor;
}

void MaterialBoxModelItem::setVendor(const QString& vendor)
{
	m_vendor = vendor;
	emit vendorChanged();
}

const QString MaterialBoxModelItem::minTemp() const
{
	return m_minTemp;
}

void MaterialBoxModelItem::setMinTemp(const QString& minTemp)
{
	m_minTemp = minTemp;
	emit minTempChanged();
}

const QString MaterialBoxModelItem::maxTemp() const
{
	return m_maxTemp;
}

void MaterialBoxModelItem::setMaxTemp(const QString& maxTemp)
{
	m_maxTemp = maxTemp;
	emit maxTempChanged();
}
const QString MaterialBoxModelItem::pressure() const
{
	return m_pressure;
}

void MaterialBoxModelItem::setPressure(const QString& pressure)
{
	m_pressure = pressure;
	emit pressureChanged();
}

 bool MaterialBoxModelItem::selected() const
{
	return m_selected;
}

void MaterialBoxModelItem::setSelected(bool select)
{
	if (m_selected == select) return;
	m_selected = select;
	emit selectedChanged();
}

bool MaterialBoxModelItem::used() const
{
	return m_used;
}

void MaterialBoxModelItem::setUsed(bool used)
{
	if (m_used == used) return;
	m_used = used;
	emit usedChanged();
}

const QString MaterialBoxModelItem::state() const
{
	return m_state;
}

void MaterialBoxModelItem::setState(const QString& state)
{
	m_state = state;
	emit stateChanged();
}

const QString MaterialBoxModelItem::percent() const
{
	return m_percent;
}

void MaterialBoxModelItem::setPercent(const QString& percent)
{
	m_percent = percent;
	emit percentChanged();
}

const QColor MaterialBoxModelItem::mapColor() const
{
	return m_mapColor;
}

void MaterialBoxModelItem::setMapColor(const QColor& color)
{
	m_mapColor = color;
	emit mapColorChanged();
}

const QString MaterialBoxModelItem::mapType() const
{
	return m_mapType;
}

void MaterialBoxModelItem::setMapType(const QString& type)
{
	m_mapType = type;
	emit mapTypeChanged();
}

const QString MaterialBoxModelItem::editStatus() const
{
	return m_editStatus;
}

void MaterialBoxModelItem::setEditStatus(const QString& status)
{
	m_editStatus = status;
	emit editStatusChanged();
}
