#include "submenutestmodel.h"
#include "menu/actioncommand.h"
#include "interface/commandinterface.h"
#include "testmodelimport.h"

namespace creative_kernel
{
    SubMenuTestModel* SubMenuTestModel::m_instance = nullptr;
    SubMenuTestModel* SubMenuTestModel::getInstance()
    {
        if (m_instance == nullptr)
        {
            m_instance = new SubMenuTestModel();
        }
        return m_instance;
    }

    SubMenuTestModel::SubMenuTestModel()
    {
        m_actionname = tr("Test Model");
        m_actionNameWithout = "Test Model";
        m_shortcut = "";      //不能有空格
        m_source = "";
        m_bSubMenu = true;
        m_insertKey = "subMenuFile";
        initActionModel();
        addUIVisualTracer(this,this);
    }

    SubMenuTestModel::~SubMenuTestModel()
    {

    }

    QAbstractListModel* SubMenuTestModel::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    void SubMenuTestModel::initActionModel()
    {
        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }
        TestModelImport* s1 = new TestModelImport(TestModelImport::BLOCK20XY);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new TestModelImport(TestModelImport::BOAT);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new TestModelImport(TestModelImport::COMPLEX);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new TestModelImport(TestModelImport::OVERHANG);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new TestModelImport(TestModelImport::SQUARE_COLUMNS_Z_AXIS);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        s1 = new TestModelImport(TestModelImport::SQUARE_PRISM_Z_AXIS);
        s1->setParent(this);
        m_actionModelList->addCommand(s1);
    }

    void SubMenuTestModel::onThemeChanged(ThemeCategory category)
    {

    }

    void SubMenuTestModel::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Test Model");
    }
}
