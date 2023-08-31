#include "cx3dexporter.h"
#include <QtCore/QDebug>
#include <QtCore/QStandardPaths>

#include "cx3dexportercommand.h"
#include "cx3dimportercommand.h"
#include "menu/ccommandlist.h"
#include "cx3dautosaveproject.h"
#include "cx3dsubmenurecentproject.h"
#include "kernel/kernelui.h"

#include "cx3dcloseprojectcommand.h"
#include "qtusercore/string/resourcesfinder.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    CX3DExporter::CX3DExporter(QObject* parent)
        :QObject(parent)
        , m_exportcommand(nullptr)
        , m_importcommand(nullptr)
    {
    }

    CX3DExporter::~CX3DExporter()
    {
    }

    QString CX3DExporter::name()
    {
        return "";
    }

    QString CX3DExporter::info()
    {
        return "";
    }


    void CX3DExporter::initialize()
    {
        m_exportcommand = new CX3DExporterCommand(this);
        m_exportcommand->setParent(this);
        CCommandList::getInstance()->addActionCommad(m_exportcommand, "saveSTL");
        m_importcommand = new CX3DImporterCommand();
        m_importcommand->setParent(this);
        CCommandList::getInstance()->addActionCommad(m_importcommand, "newPro");

        //    Cx3dAutoSaveProject * p = new Cx3dAutoSaveProject();
        //    p->CreateOpenProject();
        Cx3dSubmenuRecentProject* rectfile = Cx3dSubmenuRecentProject::createInstance(this);
        rectfile->setParent(this);
        CCommandList::getInstance()->addActionCommad(rectfile, "subMenuFile");
        //
        rectfile->setNumOfRecentFiles(10);

        Cx3dCloseProjectCommand* closePro = new Cx3dCloseProjectCommand();
        closePro->setParent(this);
        CCommandList::getInstance()->addActionCommad(closePro);

        Cx3dAutoSaveProject::instance()->start();

        registerContextObject("projectSubmodel", rectfile);
    }

    void CX3DExporter::uninitialize()
    {
        CCommandList::getInstance()->removeActionCommand(m_exportcommand);
        CCommandList::getInstance()->removeActionCommand(m_importcommand);
    }

    void CX3DExporter::setOpenedProjectPath(const QUrl& url)
    {
        m_openedUrl = url;
    }

    QUrl CX3DExporter::openedPath()
    {
        return m_openedUrl;
    }

    void CX3DExporter::openDefualtProject()
    {
        QString strLocalPath = qtuser_core::getOrCreateAppDataLocation("");
        QSettings setting;
        setting.beginGroup("AutoSave_Path");
        QString strFileName = setting.value("fileName", "defualt.cxprj").toString();
        QString strDefualtFile = strLocalPath + "/" + strFileName;

        QTimer* timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(update()));
        timer->start(3000);
    }

    void CX3DExporter::update()
    {
        qDebug() << "update!!!!";
        QSettings setting;
        setting.beginGroup("AutoSave_Path");
        setting.setValue("fileName", "defualt.cxprj");
        setting.endGroup();
    }
}
