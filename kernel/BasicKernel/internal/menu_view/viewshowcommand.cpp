#include "viewshowcommand.h"
#include "kernel/kernel.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "interface/commandinterface.h"
#include "data/modelspace.h"
#include <qtusercore/util/settings.h>
#include "interface/commandinterface.h"
#include "submenuviewsshow.h"
#include "viewshowcommand.h"

namespace creative_kernel
{
    ViewShowCommand::ViewShowCommand(EViewShow eType, QObject* parent)
        :ActionCommand(parent)
    {
        m_nShowType = eType;
        m_eParentMenu = eMenuType_View;
        switch (eType)
        {
        case ePerspectiveViewShow:
            m_actionname = tr("Perspective View");
            break;
        case eOrthographicViewShow:
            m_actionname = tr("Orthographic View");
            break;
        }


        addUIVisualTracer(this,this);
    }

    ViewShowCommand::~ViewShowCommand()
    {
    }

    void ViewShowCommand::onThemeChanged(ThemeCategory category)
    {
    }

    void ViewShowCommand::onLanguageChanged(MultiLanguage language)
    {
        switch (m_nShowType)
        {
        case ePerspectiveViewShow:
            m_actionname = tr("Perspective View");    //透视
            break;
        case eOrthographicViewShow:
            m_actionname = tr("Orthographic View");   //正交
            break;
        }
    }

    void ViewShowCommand::execute()
    {
        //    using namespace qtuser_3d;
        qtuser_3d::CameraController* obj = getKernel()->cameraController();  
        qtuser_core::VersionSettings setting;
        setting.beginGroup("view_show");
        setting.setValue("perspective_type", m_nShowType);
        setting.endGroup();

        switch (m_nShowType)
        {
        case ePerspectiveViewShow:
            obj->viewFromPerspective();
            break;
        case eOrthographicViewShow:
            obj->viewFromOrthographic();
            break;
        }

    }
    bool ViewShowCommand::enabled() {
        qtuser_core::VersionSettings setting;
        setting.beginGroup("view_show");
        int nPerspectiveType = setting.value("perspective_type").toInt();
        setting.endGroup();
        return nPerspectiveType == m_nShowType;
    }

    void ViewShowCommand::updateCheck() {
        update();
    }

     
}
