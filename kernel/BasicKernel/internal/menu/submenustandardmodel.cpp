#include "submenustandardmodel.h"
#include "menu/actioncommand.h"
#include "interface/commandinterface.h"
#include "standardmodelimport.h"
#include "buildinfo.h"
#include <QCoreApplication>

namespace creative_kernel
{
    SubMenuStandardModel* SubMenuStandardModel::m_instance = nullptr;
    SubMenuStandardModel* SubMenuStandardModel::getInstance()
    {
        if (m_instance == nullptr)
        {
            m_instance = new SubMenuStandardModel();
        }
        return m_instance;
    }

    SubMenuStandardModel::SubMenuStandardModel()
    {
        m_actionname = tr("Standard Model");
        m_actionNameWithout = "Standard Model";
        m_shortcut = "";      //不能有空格
        m_source = "";
        m_bSubMenu = true;
        m_insertKey = "subMenuFile";
        initActionModel();
    }

    SubMenuStandardModel::~SubMenuStandardModel()
    {

    }

    QAbstractListModel* SubMenuStandardModel::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    void SubMenuStandardModel::initActionModel()
    {
        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }
        StandardModelImport* s1 = new StandardModelImport(StandardModelImport::ST_CUBOID);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_SPHERICAL);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_CYLINDER);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

   /*     s1 = new StandardModelImport(StandardModelImport::ST_SPHERICALSHELL);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);*/

        s1 = new StandardModelImport(StandardModelImport::ST_CONE);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_TRUNCATEDCONE);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_RING);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_PYRAMID);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_PRISM);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_DISC);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);


        //s1 = new StandardModelImport(StandardModelImport::ST_RING);
        //s1->setParent(this);
        //m_actionModelList->addCommand(s1);
    }

    void SubMenuStandardModel::onThemeChanged(ThemeCategory category)
    {

    }

    void SubMenuStandardModel::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = QCoreApplication::translate(TRANSLATE_CONTEXT, "Recent Files");
    }
}
