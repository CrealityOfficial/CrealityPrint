#include "viewshowcommand.h"
#include "kernel/kernel.h"
#include "qtuser3d/camera/cameracontroller.h"
#include "interface/commandinterface.h"
#include "data/modelspace.h"

namespace creative_kernel
{
    ViewShowCommand::ViewShowCommand(EViewShow eType, QObject* parent)
        :ActionCommand(parent)
    {
        m_nShowType = eType;
        m_eParentMenu = eMenuType_View;
        switch (eType)
        {
        case eFrontViewShow:
            m_actionname = tr("Front View");
            break;
        case eBackViewShow:
            m_actionname = tr("Back View");
            break;
        case eLeftViewShow:
            m_actionname = tr("Left View");
            break;
        case eRightViewShow:
            m_actionname = tr("Right View");
            break;
        case eTopViewShow:
            m_actionname = tr("Top View");
            break;
        case eBottomViewShow:
            m_actionname = tr("Bottom View");
            break;
        case ePerspectiveViewShow:
            m_actionname = tr("Perspective View");
            break;
        case eOrthographicViewShow:
            m_actionname = tr("Orthographic View");
            break;
        }

        addUIVisualTracer(this);
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
        case eFrontViewShow:
            m_actionname = tr("Front View");
            break;
        case eBackViewShow:
            m_actionname = tr("Back View");
            break;
        case eLeftViewShow:
            m_actionname = tr("Left View");
            break;
        case eRightViewShow:
            m_actionname = tr("Right View");
            break;
        case eTopViewShow:
            m_actionname = tr("Top View");
            break;
        case eBottomViewShow:
            m_actionname = tr("Bottom View");
            break;
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
        qtuser_3d::Box3D box = getKernel()->modelSpace()->baseBoundingBox();
        QVector3D boxCenter = box.center();
        boxCenter.setZ(0);

        switch (m_nShowType)
        {
        case eFrontViewShow:
            obj->setviewCenter(boxCenter);   // 恢复视角的中心点
            obj->viewFromFront();
            break;
        case eBackViewShow:
            obj->setviewCenter(boxCenter);
            obj->viewFromBack();
            break;
        case eLeftViewShow:
            obj->setviewCenter(boxCenter);
            obj->viewFromLeft();
            break;
        case eRightViewShow:
            obj->setviewCenter(boxCenter);
            obj->viewFromRight();
            break;
        case eTopViewShow:
            obj->setviewCenter(boxCenter);
            obj->viewFromTop();
            break;
        case eBottomViewShow:
            obj->setviewCenter(boxCenter);
            obj->viewFromBottom();
            break;
        case ePerspectiveViewShow:
            obj->viewFromPerspective();
            break;
        case eOrthographicViewShow:
            obj->viewFromOrthographic();
            break;
        }
    }
}
