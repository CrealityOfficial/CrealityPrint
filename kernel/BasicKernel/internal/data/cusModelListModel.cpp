#include "cusModelListModel.h"
#include "data/modeln.h"

#include "interface/selectorinterface.h"
#include "interface/modelinterface.h"
#include "interface/visualsceneinterface.h"
#include "kernel/kernel.h"

#include "interface/printerinterface.h"
#include "internal/parameter/printmachine.h"
#include "internal/parameter/parametermanager.h"
#include "internal/multi_printer/printermanager.h"
#include "interface/spaceinterface.h"

#pragma execution_character_set("utf-8")
namespace creative_kernel
{
    ModelInfo::ModelInfo(ModelN* model, QObject* parent)
        : QObject(parent)
        , m_model(model)
        , m_isChosed(false)
        , m_isVisible(true)
        , m_ItemVisible(true)
    {
        //assert(model);
        if (m_model)
        {
            //m_model->setDefaultColorIndex(m_ColorIndex);
            bool res = connect(model, &ModelN::seamsDataChanged, this, &ModelInfo::zSeamVisibleChanged);
            res = connect(model, &ModelN::colorsDataChanged, this, &ModelInfo::colors2FacetsChanged);
            res = connect(model, &ModelN::settingsChanged, this, &ModelInfo::objectSettingsChanged);
            res = connect(model, &ModelN::supportsDataChanged, this, &ModelInfo::supportVisibleChanged);
            res = connect(model, &ModelN::defaultColorIndexChanged, this, &ModelInfo::colorIndexChanged);
            res = connect(model, &ModelN::defaultColorIndexChanged, this, &ModelInfo::modelColorChanged);
            res = connect(model, &ModelN::adaptiveLayerDataChanged, this, &ModelInfo::modelAdptiveLayerChanged);


            connect(model, &ModelN::supportEnabledChanged, this, &ModelInfo::slotSupportTreeAdaptiveLayer);
            connect(model, &ModelN::supportStructureChanged, this, &ModelInfo::slotSupportTreeAdaptiveLayer);
        }
        
    }

    ModelInfo::~ModelInfo()
    {
    }

    ModelN* ModelInfo::model()
    {
        return m_model;
    }


    QString ModelInfo::fileName() const
    {
        if (!m_model)
        {
            return QString();
        }
        
        QString name = m_model->objectName();
        return name;
    }

    void ModelInfo::setFileName(const QString& fileName)
    {
        m_FileName = fileName;
    }

    QColor ModelInfo::getColor()
    {
        return m_Color;
    }

    void ModelInfo::setColor(const QString& color)
    {
        QColor qColor(color);
        m_Color = qColor;
    }

    int ModelInfo::plateIndex()
    {
        if (m_model)
        {
            m_PlateIndex = getModelPrinterIndex(m_model);
        }
        else
        {
            m_PlateIndex = -1;
        }
        
        return m_PlateIndex;
    }

    void ModelInfo::setPlateIndex(int index)
    {
        m_PlateIndex = index;
    }

    int ModelInfo::getColorIndex()
    {
        return m_ColorIndex;
    }

    void ModelInfo::setColorIndex(int colorIndex)
    { 
        /*if (m_ColorIndex == colorIndex)
            return;*/
        m_ColorIndex = colorIndex;
        m_model->setDefaultColorIndex(colorIndex);
    }

    int ModelInfo::triangleNum()
    {
        return 0;
    }

    void ModelInfo::setTriangleNum(int num)
    {

    }

    bool ModelInfo::isChosed() const
    {
        return m_isChosed;
    }

    void ModelInfo::setIsChosed_cpp(bool newIsChosed)
    {
        if (m_isChosed == newIsChosed)
            return;

        m_isChosed = newIsChosed;
        emit isChosedChanged();
    }

    bool ModelInfo::isVisible() const
    {
        return m_isVisible;
    }

    void ModelInfo::setIsVisible_cpp(bool newIsVisible)
    {
        if (m_isVisible == newIsVisible)
            return;

        m_isVisible = newIsVisible;
        if (m_model)
        {
            m_model->setVisibility(m_isVisible);
        }
        emit isVisibleChanged();
        creative_kernel::checkModelRange();
    }

    bool ModelInfo::itemVisible()
    {
        return m_ItemVisible;
    }

    void ModelInfo::setItemVisible(bool visible)
    {
        m_ItemVisible = visible;
    }

    bool ModelInfo::zSeamVisible()
    {
        bool res = m_model->hasSeamsData();
        return res;
    }

    void ModelInfo::setZSeamVisible(bool visible)
    {

    }

    bool ModelInfo::colors2Facets()
    {
        bool res = m_model->hasColors();
        return res;
    }

    void ModelInfo::setColors2Facets(bool visible)
    {

    }
    bool ModelInfo::modelAdptiveLayer()
    {
        int nRet = m_model->layerHeightProfile().size();
        return nRet > 0;
    }
    void ModelInfo::setModelAdaptiveLayer(bool visible)
    {
        
    }
    bool ModelInfo::objectSettings()
    {
        bool res = m_model->isDirty();
        return res;
    }

    void ModelInfo::setObjectSettings(bool visible)
    {
        
    }

    bool ModelInfo::supportVisible()
    {
        bool res = m_model->hasSupportsData();
        return res;
    }

    int ModelInfo::colorIndex()
    {
        int index = m_model->defaultColorIndex();
        return index + 1;
    }

    QColor ModelInfo::modelColor()
    {
        PrintMachine* machine = qobject_cast<PrintMachine*>(getKernel()->parameterManager()->currentMachineObject());
        if (machine)
        {
            QColor color = machine->extruderColor(m_model->defaultColorIndex());
            return color;
        }
        return QColor();
    }
    //切换对象参数 树支撑，需要校验与自适应层高的可行性
    void ModelInfo::slotSupportTreeAdaptiveLayer()
    {
        ModelN *model  = qobject_cast<ModelN*>(sender());
        Printer *curPrinter = getSelectedPrinter();
        if (model)
        {
            QList<ModelN*> listModels = curPrinter->currentModelsInsideVisible();
            if (listModels.indexOf(model) > -1)
            {
                creative_kernel::_checkModelInsideVisibleLayers(true);
            }
        } 
    }

    CusModelListModel::CusModelListModel(QObject* parent)
        : QAbstractListModel(parent)
    {
       
    }

    CusModelListModel::~CusModelListModel()
    {
        for (auto model : m_modelsInfoList)
        {
            if (model)
            {
                delete model;
                model = nullptr;
            }
        }
        m_modelsInfoList.clear();
    }

    QList<ModelInfo*> CusModelListModel::modelList()
    {
        return m_modelsInfoList;
    }

    void CusModelListModel::addModel(ModelN* model)
    {
        ModelInfo* modelInfo = new ModelInfo(model, this);
        modelInfo->setColor("#006CFF");
        beginResetModel();
        m_modelsInfoList.push_back(modelInfo);
        endResetModel();
    }

    void CusModelListModel::addModel(ModelInfo* model)
    {
        beginResetModel();
        m_modelsInfoList.push_back(model);
        endResetModel();
    }

    void CusModelListModel::removeModel(ModelN* model)
    {
        ModelInfo* info = _index(model);
        if (!info)
            return;

        int index = m_modelsInfoList.indexOf(info);
        beginRemoveRows(QModelIndex(), index, index);
        m_modelsInfoList.removeAt(index);
        endRemoveRows();
    }

    int CusModelListModel::currentCheckedPlate()
    {
        int index = currentPrinterIndex();
        return index;
    }

    void CusModelListModel::addModel()
    {
        openMeshFile();
    }

    void CusModelListModel::deleteSelections()
    {
        removeSelectionModels(true);
    }

    void CusModelListModel::checkAll(bool on)
    {
        if (on)
            selectAll();
        else
            unselectAll();
    }

    void CusModelListModel::checkOne(int index)
    {
        selectOne(m_modelsInfoList.at(index)->model());
    }

    void CusModelListModel::checkAppendOne(int index)
    {
        appendSelect(m_modelsInfoList.at(index)->model());
    }

    void CusModelListModel::checkRange(int begin, int index)
    {
        int _begin = begin;
        if (_begin >= m_modelsInfoList.count())
            _begin = 0;
        int _end = index;

        if (_begin > _end)
            std::swap(_begin, _end);

        QList<qtuser_3d::Pickable*> models;
        for (int i = _begin; i <= _end; ++i)
            if(i < m_modelsInfoList.count())
                models << m_modelsInfoList.at(i)->model();
        appendSelects(models);
    }

    void CusModelListModel::setItemVisible(int index, bool visible)
    {
        ModelInfo* info = m_modelsInfoList.at(index);
        if (info->isVisible() == visible)
            return;

        info->setIsVisible_cpp(visible);
        if(!visible)
            unselectOne(info->model());
        requestVisUpdate(true);
    }

    bool CusModelListModel::hasSupport()
    {
        bool res = false;
        return res;
    }

    void CusModelListModel::setColor(int index, const QString color, int colorIndex)
    {
        if (index >= 0)
        {
            ModelInfo* info = m_modelsInfoList.at(index);
            info->setColor(color);
            info->setColorIndex(colorIndex);

        }else 
        {
            for (auto modelInfo : m_modelsInfoList)
            {
                if (modelInfo->getColorIndex() == colorIndex)
                {
                    modelInfo->setColor(color);
                }
            }
        }
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex);

        updateWipeTowers();
    }

    void CusModelListModel::plateModelListChanged(int plateIndex, const QList<ModelN*>& modelList)
    {
        int index = 0;
        for (auto modelInfo : m_modelsInfoList)
        {
            ModelN* modelN = modelInfo->model();
            if (modelList.indexOf(modelN) != -1)
            {
                QModelIndex begin = createIndex(index, 0);
                emit dataChanged(begin, begin);
            }
            ++index;
        }
    }

    void CusModelListModel::onChangeColors()
    {
        for (auto modelData : m_modelsInfoList)
        {
            modelData->modelColorChanged();
        }
    }

    bool CusModelListModel::isCheckedAll()
    {
        return m_isCheckedAll;
    }

    int CusModelListModel::rowCount(const QModelIndex& parent) const
    {
        Q_UNUSED(parent)
        return m_modelsInfoList.count();
    }

    QVariant CusModelListModel::data(const QModelIndex& index, int role) const
    {
        if (index.row() < 0 || index.row() >= m_modelsInfoList.count())
            return QVariant();

        ModelInfo* fInfo = m_modelsInfoList[index.row()];
        switch (role)
        {
        case File_Name:
        {
            QString strFileName = fInfo->fileName();
            strFileName.replace(".stl", "");
            return strFileName;
            break;
        }
        case File_Visible:
            return fInfo->isVisible();
            break; 
        case File_Checked:
            return fInfo->isChosed();
            break;
        case File_Color:
            return fInfo->getColor();
            break;
        case File_ColorIndex:
            return fInfo->getColorIndex();
            break;
        case File_PlateIndex:
            return fInfo->plateIndex();
            break;       
        case File_ItemVisible:
            return fInfo->itemVisible();
            break;
        case File_ZSeamVisible:
            return fInfo->itemVisible();
            break;
        case File_Colors2Facets:
            return fInfo->itemVisible();
            break;
        case File_Data:
            return QVariant::fromValue(fInfo);
            break;   
        }
        return QVariant();
    }

    QHash<int, QByteArray> CusModelListModel::roleNames() const
    {
        QHash<int, QByteArray> hash;
        hash[File_Name] = "fileName";
        hash[File_Visible] = "fileVisible";
        hash[File_Checked] = "fileChecked";
        hash[File_Data] = "fileData";
        hash[File_Color] = "fileColor";
        hash[File_ColorIndex] = "fileColorIndex";
        hash[File_PlateIndex] = "filePlateIndex";
        hash[File_ItemVisible] = "fileItemVisible";

        return hash;
    }

    bool CusModelListModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        return QAbstractListModel::setData(index, value, role);
    }

    void CusModelListModel::onSelectionsChanged()
    {
        QList<ModelN*> sels = selectionms();
        m_isCheckedAll = (sels.size() > 0 && sels.size() == m_modelsInfoList.size());
        emit isCheckedAllChanged();

        for (ModelInfo* info : m_modelsInfoList)
        {
            bool sel = info->model()->selected();
            info->setIsChosed_cpp(sel);
            if (sel && !info->isVisible())
                setItemVisible(m_modelsInfoList.indexOf(info), true);
        }

        emit selectionChanged();
    }

    void CusModelListModel::selectChanged(qtuser_3d::Pickable* pickable)
    {
    }

    ModelInfo* CusModelListModel::_index(ModelN* model)
    {
        ModelInfo* info = nullptr;
        int count = m_modelsInfoList.size();
        for (int i = 0; i < count; ++i)
        {
            if (m_modelsInfoList.at(i)->model() == model)
            {
                info = m_modelsInfoList.at(i);
                break;
            }
        }
        return info;
    }
    CusModelListModelProxy::CusModelListModelProxy(QObject* parent):
        QSortFilterProxyModel(parent)
    {
    }

    CusModelListModelProxy::~CusModelListModelProxy()
    {
    }

    void CusModelListModelProxy::filterModelByPlate(int plateIndex)
    {
        m_PlateIndex = plateIndex;
    }

    QAbstractItemModel* CusModelListModelProxy::sourceModelData()
    {
        return sourceModel();
    }

    void CusModelListModelProxy::checkOne(int checkIndex)
    {
        QModelIndex filterIndex = index(checkIndex, 0, QModelIndex());
        QModelIndex sourceIndex = mapToSource(filterIndex);
        int dataIndex = sourceIndex.row();

        CusModelListModel* cmlm = dynamic_cast<CusModelListModel*>(sourceModel());
        if (!cmlm)
            return;
        cmlm->checkOne(dataIndex);
    }

    void CusModelListModelProxy::checkAppendOne(int checkIndex)
    {
        QModelIndex filterIndex = index(checkIndex, 0, QModelIndex());
        QModelIndex sourceIndex = mapToSource(filterIndex);
        int dataIndex = sourceIndex.row();

        CusModelListModel* cmlm = dynamic_cast<CusModelListModel*>(sourceModel());
        if (!cmlm)
            return;
        cmlm->checkAppendOne(dataIndex);
    }

    void CusModelListModelProxy::checkRange(int checkBeginIndex, int checkEndIndex)
    {
        QModelIndex filterIndex = index(checkBeginIndex, 0, QModelIndex());
        QModelIndex sourceIndex = mapToSource(filterIndex);
        int beginIndex = sourceIndex.row();

        filterIndex = index(checkEndIndex, 0, QModelIndex());
        sourceIndex = mapToSource(filterIndex);
        int endIndex = sourceIndex.row();

        CusModelListModel* cmlm = dynamic_cast<CusModelListModel*>(sourceModel());
        if (!cmlm)
            return;

        cmlm->checkRange(beginIndex, endIndex);
    }

    void CusModelListModelProxy::setItemVisible(int modelIndex, bool visible)
    {
        QModelIndex filterIndex = index(modelIndex, 0, QModelIndex());
        QModelIndex sourceIndex = mapToSource(filterIndex);
        int sourceModelIndex = sourceIndex.row();

        CusModelListModel* cmlm = dynamic_cast<CusModelListModel*>(sourceModel());
        if (!cmlm)
            return;

        cmlm->setItemVisible(sourceModelIndex, visible);
    }

    void CusModelListModelProxy::setColor(int modelIndex, const QString color, int colorIndex)
    {
        QModelIndex filterIndex = index(modelIndex, 0, QModelIndex());
        QModelIndex sourceIndex = mapToSource(filterIndex);
        int sourceModelIndex = sourceIndex.row();

        CusModelListModel* cmlm = dynamic_cast<CusModelListModel*>(sourceModel());
        if (!cmlm)
            return;

        cmlm->setColor(sourceModelIndex, color, colorIndex);
    }

    bool CusModelListModelProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        QModelIndex stateIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        QString meName = sourceModel()->data(stateIndex, CusModelListModel::File_PlateIndex).toString();

        CusModelListModel* sModel = qobject_cast<CusModelListModel*>(sourceModel());
        QList<ModelInfo*> modelList = sModel->modelList();

        bool res = false;
        ModelInfo* materialData = modelList.at(sourceRow);

        if (materialData->plateIndex() == m_PlateIndex)
        {
            res = true;
        }

        return res;
    }
}