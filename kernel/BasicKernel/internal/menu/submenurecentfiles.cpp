#include "submenurecentfiles.h"

#include <qtusercore/util/settings.h>

#include "menu/actioncommand.h"
#include "openfilecommand.h"
#include "clearrecentfilecommand.h"
#include "openrecentfilecmd.h"
#include "interface/commandinterface.h"
#include "buildinfo.h"
#include <QCoreApplication>

namespace creative_kernel
{
    SubMenuRecentFiles* SubMenuRecentFiles::m_instance = nullptr;
    SubMenuRecentFiles* SubMenuRecentFiles::getInstance()
    {
        if (m_instance == nullptr)
        {
            m_instance = new SubMenuRecentFiles();
        }
        return m_instance;
    }

    SubMenuRecentFiles::SubMenuRecentFiles()
    {
        m_actionname = QCoreApplication::translate(TRANSLATE_CONTEXT, "Recent Files");
        m_actionNameWithout = "Recently files";
        m_shortcut = "";      //不能有空格
        m_source = "";
        m_eParentMenu = eMenuType_File;
        m_bSubMenu = true;
        m_insertKey = "subMenuFile";
        initActionModel();

        qtuser_core::VersionSettings settings;

        if (!settings.value(recentFileCount).isValid())
            settings.setValue(recentFileCount, QVariant(4));

        // If there are no recent files, initialize an empty list
        if (!settings.allKeys().contains(recentFileListId)) {
            settings.setValue(recentFileListId, QVariant(QStringList()));
        }
        updateRecentFiles(settings);

        addUIVisualTracer(this,this);
    }

    SubMenuRecentFiles::~SubMenuRecentFiles()
    {

    }

    QAbstractListModel* SubMenuRecentFiles::subMenuActionmodel()
    {
        return m_actionModelList;
    }

    void SubMenuRecentFiles::initActionModel()
    {
        if (nullptr == m_actionModelList)
        {
            m_actionModelList = new ActionCommandModel(this);
        }
        ClearRecentFileCommand* p4 = new ClearRecentFileCommand();
        p4->setParent(this);
        p4->setBSeparator(true);
        m_actionModelList->addCommand(p4);
        connect(p4, SIGNAL(sigExecute()), this, SLOT(slotClear()));
        // setNumOfRecentFiles(Max_RecentFile_Size);
    }

    void SubMenuRecentFiles::onThemeChanged(ThemeCategory category)
    {

    }

    void SubMenuRecentFiles::onLanguageChanged(MultiLanguage language)
    {
        m_actionname = QCoreApplication::translate(TRANSLATE_CONTEXT, "Recent Files");
    }

    void SubMenuRecentFiles::slotClear()
    {
        setEnabled(false);
        m_actionModelList->removeCommandButLastIndex();
        qtuser_core::VersionSettings settings;
        QStringList recentFileList = settings.value(recentFileListId).toStringList();
        recentFileList.clear();
        settings.setValue(recentFileListId, QVariant(recentFileList));
    }

    //Update recent file list
    void SubMenuRecentFiles::updateRecentFiles(QSettings& settings)
    {
        int numOfRecentFiles = settings.value(recentFileCount, QVariant(4)).toInt();
        QStringList MyRecentFileList = settings.value(recentFileListId).toStringList();
        if (MyRecentFileList.size() > 0)
        {
            setEnabled(true);
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
            OpenRecentFileCmd* p = new OpenRecentFileCmd();
            p->setParent(this);
            p->setName(var);
            m_actionModelList->addCommandFront(p);
        }
    }

    //recentfile max size
    void SubMenuRecentFiles::setNumOfRecentFiles(int n)
    {
        qtuser_core::VersionSettings settings;
        settings.setValue(recentFileCount, QVariant(n));
        QStringList MyRecentFileList = settings.value(recentFileListId).toStringList();
        updateRecentFiles(settings);
    }

    // recent file list
    void SubMenuRecentFiles::setMostRecentFile(const QString fileName)
    {
        if (fileName.isEmpty())
            return;
        qtuser_core::VersionSettings settings;
        QStringList recentFileList = settings.value(recentFileListId).toStringList();
        recentFileList.removeOne(fileName);
        recentFileList.append(fileName);
        settings.setValue(recentFileListId, QVariant(recentFileList));
        updateRecentFiles(settings);
    }
}
