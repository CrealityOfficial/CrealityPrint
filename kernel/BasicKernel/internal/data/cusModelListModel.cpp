#include "cusModelListModel.h"
#include "data/modeln.h"

#include "interface/selectorinterface.h"
#include "interface/modelinterface.h"
#include "interface/visualsceneinterface.h"

#pragma execution_character_set("utf-8")
namespace creative_kernel
{
    ModelInfo::ModelInfo(ModelN* model, QObject* parent)
        : QObject(parent)
        , m_model(model)
        , m_isChosed(false)
        , m_isVisible(true)
    {
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
        return m_model->objectName();
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
        m_model->setVisibility(m_isVisible);
        emit isVisibleChanged();
    }

    CusModelListModel::CusModelListModel(QObject* parent)
        : QAbstractListModel(parent)
    {
    }

    CusModelListModel::~CusModelListModel()
    {
    }

    void CusModelListModel::addModel(ModelN* model)
    {
        ModelInfo* modelInfo = new ModelInfo(model, this);

        beginInsertRows(QModelIndex(), m_modelsInfoList.count(), m_modelsInfoList.count());
        m_modelsInfoList.push_back(modelInfo);
        endInsertRows();
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
        for (ModelInfo* info : m_modelsInfoList)
        {
            bool hasSup = info->model()->hasFDMSupport();
            if (hasSup)
            {
                res = true;
            }
        }
        return res;
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
            return fInfo->fileName();
            break;
        case File_Visible:
            return fInfo->isVisible();
            break;
        case File_Checked:
            return fInfo->isChosed();
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
}