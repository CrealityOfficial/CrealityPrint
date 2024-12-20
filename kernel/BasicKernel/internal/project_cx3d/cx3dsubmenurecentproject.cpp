#include "cx3dsubmenurecentproject.h"
#include "cx3drecentprojectcommand.h"
#include "cx3dclearprojectcommand.h"
#include "interface/commandinterface.h"
#include "interface/projectinterface.h"

namespace creative_kernel
{
    Cx3dSubmenuRecentProject* Cx3dSubmenuRecentProject::m_instance = nullptr;
    Cx3dSubmenuRecentProject* Cx3dSubmenuRecentProject::getInstance()
    {
        return m_instance;
    }
    Cx3dSubmenuRecentProject* Cx3dSubmenuRecentProject::createInstance(QObject *parent)
    {
        if (m_instance == nullptr)
        {
            m_instance = new Cx3dSubmenuRecentProject(parent);
        }
        return m_instance;
    }
    Cx3dSubmenuRecentProject::Cx3dSubmenuRecentProject(QObject *parent):ActionCommand(parent)
    {
        m_actionname = tr("Recently Opened Project");
        m_actionNameWithout = "Recently Opened Project";
        m_shortcut = "";      //不能有空格
        m_source = "";
        m_eParentMenu = eMenuType_File;
        m_bSubMenu = true;

        initActionModel();

        QSettings settings;

        if (!settings.value(recentFileCount).isValid())
            settings.setValue(recentFileCount, QVariant(4));

        // If there are no recent files, initialize an empty list
        if (!settings.allKeys().contains(recentFileListId)) {
            settings.setValue(recentFileListId, QVariant(QStringList()));
        }
        updateRecentFiles(settings);

        addUIVisualTracer(this,this);
    }

    Cx3dSubmenuRecentProject::~Cx3dSubmenuRecentProject()
    {
    }

    QAbstractListModel* Cx3dSubmenuRecentProject::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    void Cx3dSubmenuRecentProject::initActionModel()
    {
        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }
        Cx3dClearProjectCommand* clear = new Cx3dClearProjectCommand(this);
        clear->setBSeparator(true);
        m_actionModelList->addCommand(clear);
        connect(clear, SIGNAL(sigExecute()), this, SLOT(slotClear()));
        setNumOfRecentFiles(Max_RecentFile_Size);
    }

    void Cx3dSubmenuRecentProject::onThemeChanged(ThemeCategory category)
    {

    }

    void Cx3dSubmenuRecentProject::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = tr("Recently Opened Project");
    }

    void Cx3dSubmenuRecentProject::slotClear()
    {
        setEnabled(false);
        m_actionModelList->removeCommandButLastIndex();
        QSettings settings;
        QStringList recentFileList = settings.value(recentFileListId).toStringList();
        recentFileList.clear();
        settings.setValue(recentFileListId, QVariant(recentFileList));
    }

    //Update recent file list
    void Cx3dSubmenuRecentProject::updateRecentFiles(QSettings& settings)
    {
        int numOfRecentFiles = settings.value(recentFileCount, QVariant(4)).toInt();
        QStringList MyRecentFileList = settings.value(recentFileListId).toStringList();
        if (MyRecentFileList.size() > 0)
        {
            setEnabled(true);
            MyRecentFileList.removeOne(projectUI()->getAutoProjectPath());
        }
        //
        int nCount = MyRecentFileList.size();
        while (nCount - numOfRecentFiles > 0)
        {
            MyRecentFileList.pop_front();
            nCount--;
        }
        m_actionModelList->removeCommandButLastIndex();

        settings.setValue(recentFileListId, QVariant(MyRecentFileList));
        foreach(auto var, MyRecentFileList)
        {
            Cx3dRecentProjectCommand* p = new Cx3dRecentProjectCommand(this);
            p->setName(var);
            m_actionModelList->addCommandFront(p);
        }
    }

    //recentfile max size
    void Cx3dSubmenuRecentProject::setNumOfRecentFiles(int n)
    {
        QSettings settings;
        settings.setValue(recentFileCount, QVariant(n));
        QStringList MyRecentFileList = settings.value(recentFileListId).toStringList();
        updateRecentFiles(settings);
    }

    // recent file list
    void Cx3dSubmenuRecentProject::setMostRecentFile(const QString fileName)
    {
        if (fileName.isEmpty())
            return;
        QSettings settings;
        QStringList recentFileList = settings.value(recentFileListId).toStringList();
        recentFileList.removeOne(fileName);
        recentFileList.append(fileName);
        settings.setValue(recentFileListId, QVariant(recentFileList));
        updateRecentFiles(settings);
    }
}
