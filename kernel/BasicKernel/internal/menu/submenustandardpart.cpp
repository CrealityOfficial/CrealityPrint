#include "submenustandardpart.h"
#include "menu/actioncommand.h"
#include "interface/commandinterface.h"
#include "interface/selectorinterface.h"
#include "standardmodelimport.h"
#include "submenuimportmodel.h"
#include "openfilecommand.h"

namespace creative_kernel
{
    SubMenuStandardPart::SubMenuStandardPart(ModelNType partType, QObject* parent) :
        ActionCommand(parent),
        m_partType(partType)
    {
        m_shortcut = "";      //不能有空格
        m_source = "";
        m_bSubMenu = true;
        // m_insertKey = "subMenuFile";

        if (m_partType == ModelNType::normal_part)
        {
            m_actionNameWithout = "Add Part";
            m_source = "qrc:/UI/photo/menuImg/add_part_n.svg";
            m_icon = "qrc:/UI/photo/menuImg/add_negative_part_p.svg";
        }
        else if (m_partType == ModelNType::negative_part)
        {
            m_actionNameWithout = "Add Negative Part";
            m_source = "qrc:/UI/photo/menuImg/add_negative_part_n.svg";
            m_icon = "qrc:/UI/photo/menuImg/add_negative_part_p.svg";
        }
        else if (m_partType == ModelNType::modifier)
        {
            m_actionNameWithout = "Add Modifier";
            m_source = "qrc:/UI/photo/menuImg/add_modifier_n.svg";
            m_icon = "qrc:/UI/photo/menuImg/add_modifier_p.svg";
        }
        else if (m_partType == ModelNType::support_defender)
        {
            m_actionNameWithout = "Add Support Blocker";
            m_source = "qrc:/UI/photo/menuImg/add_support_blocker_n.svg";
            m_icon = "qrc:/UI/photo/menuImg/add_support_blocker_p.svg";
        }
        else if (m_partType == ModelNType::support_generator)
        {
            m_actionNameWithout = "Add Support Enforcer";
            m_source = "qrc:/UI/photo/menuImg/add_support_enforcer_n.svg";
            m_icon = "qrc:/UI/photo/menuImg/add_support_enforcer_p.svg";
        }

        setName(tr(m_actionNameWithout.toLatin1())); 
        initActionModel();
        
        addUIVisualTracer(this,this);
    }

    SubMenuStandardPart::~SubMenuStandardPart()
    {

    }

    QAbstractListModel* SubMenuStandardPart::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    void SubMenuStandardPart::initActionModel()
    {
        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }

        OpenFileCommand* command = new OpenFileCommand;
        command->setNameWithout("Import");
        command->setSource("qrc:/UI/photo/menuImg/load_file_n.svg");
        command->setIcon("qrc:/UI/photo/menuImg/load_file_p.svg");
        command->setShortcut("");
        command->setParent(this);
        command->setJoinGroupEnabled(true);
        command->setModelType(m_partType);
        command->setFileType(OpenFileCommand::Mesh);
        m_actionModelList->addCommand(command);

        StandardModelImport* s1 = new StandardModelImport(StandardModelImport::ST_CUBOID);
        s1->setParent(this);
        s1->setModelType(m_partType);
        s1->setAddPartFlag(true);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_SPHERICAL);
        s1->setParent(this);
        s1->setModelType(m_partType);
        s1->setAddPartFlag(true);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_CYLINDER);
        s1->setParent(this);
        s1->setModelType(m_partType);
        s1->setAddPartFlag(true);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_CONE);
        s1->setParent(this);
        s1->setModelType(m_partType);
        s1->setAddPartFlag(true);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_RING);
        s1->setParent(this);
        s1->setModelType(m_partType);
        s1->setAddPartFlag(true);
        m_actionModelList->addCommand(s1);

        s1 = new StandardModelImport(StandardModelImport::ST_DISC);
        s1->setParent(this);
        s1->setModelType(m_partType);
        s1->setAddPartFlag(true);
        m_actionModelList->addCommand(s1);

    }

    void SubMenuStandardPart::onThemeChanged(ThemeCategory category)
    {

    }

    void SubMenuStandardPart::onLanguageChanged(MultiLanguage language)
    {
        setName(tr(m_actionNameWithout.toLatin1())); 
        // m_actionname = tr("Recently files");
    }
}
