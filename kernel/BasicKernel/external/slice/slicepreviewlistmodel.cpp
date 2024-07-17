#include "slicepreviewlistmodel.h"
#include <QQuickImageProvider>
#include <QtQml>
#include "cxkernel/interface/uiinterface.h"
#include <QDebug>
#include "internal/multi_printer/printer.h"
#include "interface/printerinterface.h"
#include "../printer/capturepreviewworker.h"
#include "interface/commandinterface.h"
#include "cxkernel/interface/jobsinterface.h"
#include "interface/reuseableinterface.h"
#include "slicepreviewflow.h"

namespace creative_kernel {

    class ImageProvider : public QQuickImageProvider
    {
    public:
        ImageProvider(const QImage& _image) : QQuickImageProvider(QQuickImageProvider::Image)
        {
            image = _image;
        }

        QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override
        {
            Q_UNUSED(id);

            if (size)
                *size = image.size();

            return image;
        }
    private:
        QImage image;
    };


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

    void SlicePreviewListModel::updatePicture(int idx)
    {
		auto captureCallback = [=](QObject* receiver, const QImage& image)
		{
            Printer* printer = dynamic_cast<Printer*>(receiver);
            if (printer)
                printer->setPicture(image);

            m_captureQueue.removeOne(printer);

            if (m_captureQueue.isEmpty())
            {
                QTimer::singleShot(100, [=] ()
                {
                    m_captor->endCapturePreview();
                    beginModify();
                    updateImageProvider();
                    endModify();
                    emit updatePictureFinished();
                });
            }
		};

        if (m_captureQueue.isEmpty())
            m_captor->beginCapturePreview();

        if (idx == -1)
        {
            for (int i = 0, count = printersCount(); i < count; ++i)
            {
                Printer* printer = getPrinter(i);
                if (printer && !printer->isDirty())
                    continue;
                m_captureQueue << printer;
                m_captor->capturePrinter(printer, captureCallback);
            }
        }
        else 
        {
            Printer* printer = getPrinter(idx);
            if (printer && printer->isDirty())
            {
                m_captureQueue << printer;
                m_captor->capturePrinter(printer, captureCallback);
            }
        }

        if (m_captureQueue.isEmpty())
        {
            m_captor->endCapturePreview();
            emit updatePictureFinished();
        }
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

    void SlicePreviewListModel::updateImageProvider()
    {
        if (m_imageBase == 0)
            m_imageBase = 30;
        else 
            m_imageBase = 0;

        for (int i = 0, count = printersCount(); i < count; ++i)
        {
            auto p = getPrinter(i);
            auto provider = new ImageProvider(p->picture());
            QString name = QString("active_slice%1").arg(m_imageBase + i);
            cxkernel::registerImageProvider(name, provider);
           // p->picture().save(QString("D:/%1.png").arg(name));
        }
    }

    void SlicePreviewListModel::setPicture(int idx, const QImage& img)
    {
        if (idx < 0 || idx >= printersCount())
            return;

        getPrinter(idx)->setPicture(img);
        cxkernel::registerImageProvider(QString("active_slice%1").arg(idx), new ImageProvider(img));
        
        if (!m_isChanging)
        {
            beginResetModel();
            endResetModel();
            emit dataChanged();
        }
    }

    QImage SlicePreviewListModel::picture(int idx) const
    {
        if (idx < 0 || idx >= printersCount())
            return QImage();

        return getPrinter(idx)->picture();
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
        switch (role)
        {
        case PictureRole:
            res = getPrinter(i)->picture();
            break;
        case IsSlicedRole:
            res = getPrinter(i)->isSliced();
            break;
        case imageSourceRole:
            res = QString("image://active_slice%1").arg(m_imageBase + i);
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
