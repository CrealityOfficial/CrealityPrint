#include "platesDataManager.h"
#include "cusModelListModel.h"
#include "internal/multi_printer/printermanager.h"
#include "data/modeln.h"

namespace creative_kernel
{

    PlateInfo::PlateInfo(CusModelListModelProxy* modelData, QObject* parent, Printer* printer)
        :QObject(parent),
        m_ModelData(modelData),
        m_Printer(printer)
    {

    }

    PlateInfo::~PlateInfo()
    {
        if (m_ModelData)
        {
            delete m_ModelData;
            m_ModelData = nullptr;
        }
    }

    int PlateInfo::plateIndex()
    {
        if (!m_Printer)
            return -1;
        return m_Printer->index();
    }

    void PlateInfo::setPlateIndex(int index)
    {
        if (!m_Printer)
            return;

        m_ModelData->filterModelByPlate(index);
    }

    
    bool PlateInfo::isExpend()
    {
        return m_isExpend;
    }

    void PlateInfo::setIsExpend(bool expend)
    {
        m_isExpend = expend;
        emit isExpendChanged();
    }

    QString PlateInfo::plateName()
    {
        return QString();
    }

    void PlateInfo::setPlateName(QString name)
    {

    }

    Printer* PlateInfo::printer()
    {
        return m_Printer;
    }

    CusModelListModelProxy* PlateInfo::modelsData()
    {
        return m_ModelData;
    }

	PlatesDataManager::PlatesDataManager(QObject* parent)
        :QAbstractListModel(parent)
	{
        PrinterMangager* pm = getPrinterMangager();

        connect(pm, &PrinterMangager::signalAddPrinter, this, &PlatesDataManager::slotAddPlate);
        connect(pm, &PrinterMangager::signalDeletePrinter, this, &PlatesDataManager::slotDeletePlate);
        connect(pm, &PrinterMangager::signalModelsOutOfPrinterChange, this, &PlatesDataManager::slotModelsOutPlateChange);

	}

	PlatesDataManager::~PlatesDataManager()
	{

	}

    int PlatesDataManager::plateCount()
    {
        PrinterMangager* pm = getPrinterMangager();
        return pm->printersCount();
    }

    int PlatesDataManager::currentPlateIndex()
    {
        PrinterMangager* pm = getPrinterMangager();
        Printer* pt = pm->getSelectedPrinter();
        if (!pt)
            return -1;
        int index = pt->index();
        return index;
    }

    void PlatesDataManager::setCurrentPlateIndex(int index)
    {
        PrinterMangager* pm = getPrinterMangager();
        pm->selectPrinter(index);

        emit currentPlateIndexChanged();
    }

	void PlatesDataManager::registerModelList(CusModelListModel* modelList)
	{
        m_SourceModelList = modelList;

        //默认添加盘外对象，盘外对应的默认索引为-1
        CusModelListModelProxy* cusModelList = new CusModelListModelProxy();
        cusModelList->setSourceModel(m_SourceModelList);
        cusModelList->filterModelByPlate(-1);

        PlateInfo* pi = new PlateInfo(cusModelList, this);
        pi->setPlateIndex(-1);
        m_PlatesInfo[-1] = pi;

        connect(this, &PlatesDataManager::plateModelListChanged, modelList, 
            &CusModelListModel::plateModelListChanged);

        connect(m_SourceModelList, &CusModelListModel::selectionChanged, this, [this](){
            QList <ModelInfo*> modelsInfoList = m_SourceModelList->modelList();
            for (ModelInfo* info : modelsInfoList)
            {
                bool sel = info->model()->selected();
                if (info->isVisible() && sel)
                {
                    int index = info->plateIndex();
                    PlateInfo* pi = m_PlatesInfo.value(index);
                    if (pi)
                        pi->setIsExpend(sel);
                }
            }
            });
	}

    void PlatesDataManager::addPlate(Printer* p)
    {
        int index = p->index();
        addPlateDataModelProxy(p);
        refreshPlatesIndex();
        emit plateCountChanged();
    }

    void PlatesDataManager::deletePlate(Printer* p)
    {
        int index = p->index();
        PlateInfo* pi = m_PlatesInfo[index];
        pi->deleteLater();
        m_PlatesInfo.remove(index);
        refreshPlatesIndex();
      
        emit plateCountChanged();
    }

    void PlatesDataManager::selectPlate(int index)
    {
        PrinterMangager* pm = getPrinterMangager();
        pm->selectPrinter(index);

        emit currentPlateIndexChanged();
    }

    int PlatesDataManager::rowCount(const QModelIndex& parent) const
    {
        return m_PlatesInfo.count();
    }

    QVariant PlatesDataManager::data(const QModelIndex& index, int role) const
    {
        if (index.row() < 0 || index.row() >= m_PlatesInfo.count())
            return QVariant();

        PlateInfo* fInfo = m_PlatesInfo[index.row()];
        if (index.row() == m_PlatesInfo.count() - 1)
        {
            fInfo = m_PlatesInfo[-1];

        }
        if (!fInfo)
            return QVariant();

        switch (role)
        {
        case PR_PlateIndex:
            return fInfo->plateIndex();
            break;
        case PR_PlateModels:
            return QVariant::fromValue(fInfo->modelsData());
            break; 
        case PR_PLateName:
            return fInfo->plateName();
        case PR_PLATEINFO:
            return QVariant::fromValue(fInfo);
        case PR_PLateObj:
            return QVariant::fromValue(fInfo->printer());
            break;
        }
        return QVariant();
    }

    QHash<int, QByteArray> PlatesDataManager::roleNames() const
    {
        QHash<int, QByteArray> hash;
        hash[PR_PlateIndex] = "plateIndex";
        hash[PR_PlateModels] = "plateModels";
        hash[PR_PLateName] = "plateName";
        hash[PR_PLATEINFO] = "plateInfo";
        hash[PR_PLateObj] = "plateObj";

        return hash;
    }

    bool PlatesDataManager::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        return QAbstractListModel::setData(index, value, role);
    }

    void PlatesDataManager::slotAddPlate(Printer* p)
    {
        PrinterMangager* pm = getPrinterMangager();
        int count = pm->printersCount();
        p->setObjectName(QString(count));
        addPlate(p);

        //盘内模型
        bool res = connect(p, &Printer::signalModelsInsideChange, this, [this](const QList<ModelN*>& modelList) {
             QList<ModelN*> list = modelList;
             QObject* obj = sender();
             Printer* senderPrinter = qobject_cast<Printer*>(sender());
             int printerIndex = senderPrinter->index();
             emit plateModelListChanged(printerIndex, modelList);
        });
    }

    void PlatesDataManager::slotDeletePlate(Printer* p)
    {
        //
        deletePlate(p);
    }

    void PlatesDataManager::slotModelsOutPlateChange(const QList<ModelN*>& list)
    {
        emit plateModelListChanged(-1, list);
    }

    void PlatesDataManager::addPlateDataModelProxy(Printer* pt)
    {
        //添加新盘的时候创建对应的代理模型，这个代理模型和盘的索引对应
        int index = pt->index();
        CusModelListModelProxy* cusModelList = new CusModelListModelProxy();
        cusModelList->setSourceModel(m_SourceModelList);
        cusModelList->filterModelByPlate(index);

        PlateInfo* pi = new PlateInfo(cusModelList, this, pt);
        pi->setPlateIndex(index);

        beginInsertRows(QModelIndex(), index, index);
        m_PlatesInfo[m_PlatesInfo.count()-1] = pi;
        endInsertRows();
    }   

    void PlatesDataManager::refreshPlatesIndex()
    {
        QList<PlateInfo*> ps = m_PlatesInfo.values();
        m_PlatesInfo.clear();

        beginResetModel();
        for (auto pi : ps)
        {
            pi->setPlateIndex(pi->plateIndex());
            m_PlatesInfo[pi->plateIndex()] = pi;
        }
        endResetModel();
    }
}
