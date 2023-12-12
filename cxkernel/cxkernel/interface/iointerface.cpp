#include "iointerface.h"
#include "cxkernel/kernel/cxkernel.h"
#include "qtusercore/module/cxopenandsavefilemanager.h"

namespace cxkernel
{
    void setLastSaveFileName(const QString& fileName)
    {
        cxKernel->ioManager()->setLastSaveFileName(fileName);
    }

    void openLastSaveFolder()
    {
        cxKernel->ioManager()->openLastSaveFolder();
    }

    void setIOFilterKey(const QString& filterKey)
    {
        cxKernel->ioManager()->setFilterKey(filterKey);
    }

    QString currOpenFile()
    {
        return cxKernel->ioManager()->currOpenFile();
    }

    void saveFile(qtuser_core::CXHandleBase* handler, const QString& defaultName, const QString& title)
    {
        cxKernel->ioManager()->save(handler, defaultName, title);
    }

    void openFile(qtuser_core::CXHandleBase* handler, const QString& title)
    {
        cxKernel->ioManager()->open(handler, title);
    }

    bool openFileWithName(const QString& fileName)
    {
        return cxKernel->ioManager()->openWithName(fileName);
    }

    void openFileWithNames(const QStringList& fileNames)
    {
        cxKernel->ioManager()->openWithNames(fileNames);
    }

    bool openFileWithString(const QString& commonName)
    {
        return cxKernel->ioManager()->openWithString(commonName);
    }

    void openFileWithUrls(const QList<QUrl>& urls)
    {
        cxKernel->ioManager()->openWithUrls(urls);
    }

    void registerOpenHandler(qtuser_core::CXHandleBase* handler)
    {
        cxKernel->ioManager()->registerOpenHandler(handler);
    }

    void registerSaveHandler(qtuser_core::CXHandleBase* handler)
    {
        cxKernel->ioManager()->registerSaveHandler(handler);
    }

    void unRegisterOpenHandler(qtuser_core::CXHandleBase* handler)
    {
        cxKernel->ioManager()->unRegisterOpenHandler(handler);
    }

    void unRegisterSaveHandler(qtuser_core::CXHandleBase* handler)
    {
        cxKernel->ioManager()->unRegisterSaveHandler(handler);
    }
}