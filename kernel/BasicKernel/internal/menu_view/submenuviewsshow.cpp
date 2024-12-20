#include "submenuviewsshow.h"
#include "viewshowcommand.h"
#include "interface/commandinterface.h"
#include <qtusercore/util/settings.h>
#include "qtuser3d/camera/cameracontroller.h"
#include "kernel/kernel.h"

namespace creative_kernel
{
    SubMenuViewShow::SubMenuViewShow()
    {
        m_actionname = tr("View Show");
        m_actionNameWithout = "View Show";
        m_shortcut = "";      //不能有空格
        m_source = "";
        m_eParentMenu = eMenuType_View;
        m_bSubMenu = true;

        addUIVisualTracer(this,this);
        initActionModel();
    }

    SubMenuViewShow::~SubMenuViewShow()
    {

    }

    void SubMenuViewShow::onThemeChanged(ThemeCategory category)
    {

    }

    void SubMenuViewShow::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("View Show");
    }

    QAbstractListModel* SubMenuViewShow::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    bool SubMenuViewShow::enabled()
    {
        return true;
    }

    void SubMenuViewShow::initActionModel()
    {
        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }
        qtuser_core::VersionSettings setting;
        setting.beginGroup("view_show");
        int nPerspectiveType = setting.value("perspective_type").toInt();
        if (!nPerspectiveType) {
            setting.setValue("perspective_type", ePerspectiveViewShow);
        }
        setting.endGroup();

        ViewShowCommand* pPerspective = new ViewShowCommand(ePerspectiveViewShow);
        pPerspective->setParent(this);
        // pPerspective->setChecked(nPerspectiveType == ePerspectiveViewShow);
        m_actionModelList->addCommand(pPerspective);
         ViewShowCommand* pOrthographic = new ViewShowCommand(eOrthographicViewShow);
        pOrthographic->setParent(this);
        //pOrthographic->setChecked(nPerspectiveType == eOrthographicViewShow);
        m_actionModelList->addCommand(pOrthographic);


    }
}
