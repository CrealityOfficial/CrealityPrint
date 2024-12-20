#include "submenuexportmodel.h"
#include "menu/actioncommand.h"
#include "interface/commandinterface.h"
#include "exportpresetcommand.h"
#include "../menu/saveascommand.h"
#include "../project_3mf/save3mf.h"
namespace creative_kernel
{
    SubMenuExportModel* SubMenuExportModel::m_instance = nullptr;
    SubMenuExportModel* SubMenuExportModel::getInstance()
    {
        if (m_instance == nullptr)
        {
            m_instance = new SubMenuExportModel();
        }
        return m_instance;
    }

    SubMenuExportModel::SubMenuExportModel()
    {
        //m_actionname = tr("Test Model");
        m_actionNameWithout = "Export SubMenu";
        m_shortcut = "";      //不能有空格
        m_source = "";
        m_bSubMenu = true;
        m_insertKey = "";
        initActionModel();
        addUIVisualTracer(this,this);
    }

    SubMenuExportModel::~SubMenuExportModel()
    {

    }

    QAbstractListModel* SubMenuExportModel::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    void SubMenuExportModel::initActionModel()
    {
        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }
        
        ExportPresetCommand* exportPresetCmd = new ExportPresetCommand();
        exportPresetCmd->setParent(this);
        exportPresetCmd->setBSeparator(true);
        m_actionModelList->addCommand(exportPresetCmd);


        SaveAsCommand* modelfile = new SaveAsCommand();
        modelfile->setParent(this);
        m_actionModelList->addCommand(modelfile);

         Save3MFCommand* save3MF = new Save3MFCommand();
         save3MF->setParent(this);
        // save3MF->setBSeparator(true);
        // m_actionModelList->addCommand(save3MF);
    }
    void SubMenuExportModel::onThemeChanged(ThemeCategory category)
    {}
    void SubMenuExportModel::onLanguageChanged(MultiLanguage language)
    {}
}
