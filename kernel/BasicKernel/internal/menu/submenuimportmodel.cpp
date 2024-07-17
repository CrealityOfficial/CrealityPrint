#include "submenuimportmodel.h"
#include "menu/actioncommand.h"
#include "interface/commandinterface.h"
#include "importpresetcommand.h"
#include "../project_3mf/load3mf.h"
#include "../menu/openfilecommand.h"
namespace creative_kernel
{
    SubMenuImportModel* SubMenuImportModel::m_instance = nullptr;
    SubMenuImportModel* SubMenuImportModel::getInstance()
    {
        if (m_instance == nullptr)
        {
            m_instance = new SubMenuImportModel();
        }
        return m_instance;
    }

    SubMenuImportModel::SubMenuImportModel()
    {
        //m_actionname = tr("Test Model");
        m_actionNameWithout = "Import SubMenu";
        m_shortcut = "";      //不能有空格
        m_source = "";
        m_bSubMenu = true;
        m_insertKey = "subMenuFile";
        initActionModel();
        addUIVisualTracer(this,this);
    }

    SubMenuImportModel::~SubMenuImportModel()
    {

    }

    QAbstractListModel* SubMenuImportModel::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    QVariant SubMenuImportModel::getOpt(const QString& optName)
    {
        QList<ActionCommand*> commandList = m_actionModelList->actionCommands();
        QVariant res;
        foreach(auto command, commandList)
        {
            if (command->nameWithout() == optName)
            {
                res = QVariant::fromValue(command);
                break;
            }
        }

        return res;
    }

    void SubMenuImportModel::initActionModel()
    {
        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }
        
        ImportPresetCommand* s1 = new ImportPresetCommand();
        s1->setParent(this);
        m_actionModelList->addCommand(s1);

        OpenFileCommand* openFile = new OpenFileCommand();
        openFile->setParent(this);
        m_actionModelList->addCommand(openFile);

        // Load3MFCommand* load3MF = new Load3MFCommand();
        // load3MF->setParent(this);
        // load3MF->setBSeparator(true);
        // m_actionModelList->addCommand(load3MF);
    }
    void SubMenuImportModel::onThemeChanged(ThemeCategory category)
    {}
    void SubMenuImportModel::onLanguageChanged(MultiLanguage language)
    {}
}
