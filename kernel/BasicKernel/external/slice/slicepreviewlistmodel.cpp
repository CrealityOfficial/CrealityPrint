#include "slicepreviewlistmodel.h"
#include <QtQml>
#include <QDebug>
#include "internal/multi_printer/printer.h"
#include "interface/printerinterface.h"
#include "../printer/capturepreviewworker.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/reuseableinterface.h"
#include "slicepreviewflow.h"
#include "sliceattain.h"

namespace creative_kernel {

    SlicePreviewListModel::SlicePreviewListModel(QObject* parent) : QAbstractListModel(parent)
    {

    }

    SlicePreviewListModel::~SlicePreviewListModel()
    {

    }

    void SlicePreviewListModel::update()
    {       
        beginModify();
        endModify();
    }

    void SlicePreviewListModel::beginModify()
    {
        m_isChanging = true;
        beginResetModel();
    }

    void SlicePreviewListModel::endModify()
    {
        endResetModel();
        m_isChanging = false;
    }

    QImage SlicePreviewListModel::picture(int idx) const
    {
        if (idx < 0 || idx >= printersCount())
            return QImage();

        return getPrinter(idx)->picture();
    }

    QString SlicePreviewListModel::materialWeight(int idx) const
    {
        if (idx < 0 || idx >= printersCount())
            return QString();

        SliceAttain* attain = dynamic_cast<SliceAttain*>(getPrinter(idx)->attain());
        if (attain)
            return attain->material_weight();
        else
            return QString();
    }
    QString SlicePreviewListModel::printTime(int idx) const
    {
        if (idx < 0 || idx >= printersCount())
            return QString();

        SliceAttain* attain = dynamic_cast<SliceAttain*>(getPrinter(idx)->attain());

        if (attain)
            return attain->printing_time();
        else
            return QString();
    }
    QString SlicePreviewListModel::getGcodeFileName(int idx) const
    {
            if (idx < 0 || idx >= printersCount())
                return QString();

            SliceAttain* attain = dynamic_cast<SliceAttain*>(getPrinter(idx)->attain());
            if (attain)
                return attain->tempGCodeFileName();
            else
                return QString();
        }
    QStringList SlicePreviewListModel::getModelInsideUseColors(int idx) const
    {
        if (idx < 0 || idx >= printersCount())
            return QStringList();
        QSet<int> listColors = getPrinter(idx)->getUsedExtruders(true);
        QStringList stringList;
        for (QSet<int>::const_iterator it = listColors.constBegin(); it != listColors.constEnd(); ++it) {
            stringList << QString::number(*it);
        }
        return stringList;
    }

    

    void SlicePreviewListModel::setIsSliced(int idx, bool isSliced)
    {
        if (idx < 0 || idx >= printersCount())
            return;

        getPrinter(idx)->setIsSliced(isSliced);

        if (!m_isChanging)
        {
            beginResetModel();
            endResetModel();
            emit dataChanged();
        }
    }

    bool SlicePreviewListModel::isSliced(int idx)
    {
        if (idx < 0 || idx >= printersCount())
            return false;

        return getPrinter(idx)->isSliced();
    }

    bool SlicePreviewListModel::isEmpty(int idx) const
    {
        if (idx < 0 || idx >= printersCount())
            return true;

        return !getPrinter(idx)->modelsInsideCount();
    }

    void SlicePreviewListModel::setAttain(int idx, SliceAttain* attain)
    {
        if (idx < 0 || idx >= printersCount())
            return;

        getPrinter(idx)->setAttain(attain);
    }

    SliceAttain* SlicePreviewListModel::attain(int idx)
    {
        if (idx < 0 || idx >= printersCount())
            return NULL;
            
        return (SliceAttain*)getPrinter(idx)->attain();
    }
    
    QVariant SlicePreviewListModel::data(const QModelIndex& index, int role) const 
    {        
        int i = index.row();
        if (i < 0 || i >= printersCount())
            return QVariant(QVariant::Invalid);

        QVariant res;
        QString path;
        switch (role)
        {
        case PictureRole:
            res = getPrinter(i)->picture();
            break;
        case IsSlicedRole:
            res = getPrinter(i)->isSliced();
            break;
        case imageSourceRole:
            path = getPrinter(i)->pictureUrl();
            res = path;
            break;
        case MaterialWeight:
            res = materialWeight(i);
            break;
        case PrintTime:
            res = printTime(i);
            break;
        case IsEmptyRole:
            res = isEmpty(i);
            break;
        case GCodePath:
            SliceAttain* attain = dynamic_cast<SliceAttain*>(getPrinter(i)->attain()); 
            res = attain->tempGCodeFileName();
            break;
        }

        return res;
    }

    QHash<int, QByteArray> SlicePreviewListModel::roleNames() const 
    {
        QHash<int, QByteArray> roles;
        roles[PictureRole] = "picture";
        roles[IsSlicedRole] = "isSliced";
        roles[imageSourceRole] = "imageSource";
        roles[IsEmptyRole] = "isEmpty";
        roles[MaterialWeight] = "materialWeight";
        roles[PrintTime] = "printTime";
        roles[GCodePath] = "gCodePath";
        return roles;
    }

    int SlicePreviewListModel::rowCount(const QModelIndex& parent) const 
    {
        return m_enabled ? printersCount() : 0;
    }

    void SlicePreviewListModel::setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }
    
};
