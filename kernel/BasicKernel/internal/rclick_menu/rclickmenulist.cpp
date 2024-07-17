#include "rclickmenulist.h"
#include"internal/rclick_menu/deletemodelaction.h"
#include"internal/rclick_menu/importmodelaction.h"
#include"internal/rclick_menu/cloneaction.h"
#include"internal/rclick_menu/selectallaction.h"
#include"internal/rclick_menu/clearallaction.h"
#include"internal/rclick_menu/resetallaction.h"
#include"internal/rclick_menu/placeonprinterbed.h"
#include"internal/rclick_menu/mergemodelaction.h"
#include"internal/rclick_menu/mergemodellocation.h"
#include"internal/rclick_menu/splitmodelaction.h"
#include "internal/rclick_menu/setnozzlerange.h"
#include"internal/rclick_menu/uploadmodelaction.h"
#include "internal/rclick_menu/layoutcommand.h"
#include "internal/rclick_menu/layoutplateaction.h"
#include "internal/rclick_menu/simplifiedModel.h"
#include "internal/rclick_menu/coverdModelAction.h"
#include "internal/rclick_menu/selecttoolaction.h"
#include "internal/rclick_menu/pickbottomaction.h"
#include "internal/rclick_menu/lockplateaction.h"
#include "internal/rclick_menu/deleteplateaction.h"
#include "internal/rclick_menu/modelsettingaction.h"
#include "internal/rclick_menu/selectprinterallaction.h"
#include "internal/rclick_menu/clearprinteraction.h"
#include "internal/rclick_menu/mergemodeltoprinteraction.h"

#include "internal/menu/submenutestmodel.h"
#include "internal/menu/submenurecentfiles.h"

#include "interface/machineinterface.h"

#include "kernel/kernel.h"

namespace creative_kernel
{
    RClickMenuList::RClickMenuList(QObject* parent)
        : QObject(parent)
        , m_nozzleRange(nullptr)
        , m_upAction(nullptr)
    {
        m_recentFiles = SubMenuRecentFiles::getInstance();

        m_testModels = SubMenuTestModel::getInstance();
        m_testModels->setBSeparator(true);

        // setNozzleCount(currentMachineExtruderCount());
        // m_ExtruderColors = currentMachineExtruderColorList();
    }

    RClickMenuList::~RClickMenuList()
    {

    }

    void RClickMenuList::updateState()
    {
        for (ActionCommand* command : m_commandsMap) {
            if (command)
                command->update();
        }
    }

    QVariantList RClickMenuList::noModelsMenuList()
    {
        QVariantList datas;
        for (ActionCommand* command : m_noModelMenuList) {
            datas.append(QVariant::fromValue(command));
        }
        return datas;
    }

    QVariantList RClickMenuList::singleModelMenuList()
    {
        QVariantList datas;
        for (ActionCommand* command : m_singleModelMenuList) {
            datas.insert(0, QVariant::fromValue(command));
        }
        return datas;
    }

    QVariantList RClickMenuList::multiModelsMenuList()
    {  
         QVariantList datas;
          for (ActionCommand* command : m_multiModelsMenuList) {
              datas.append(QVariant::fromValue(command));
          }
         return datas;
    }

    QObject* RClickMenuList::recentFilesModel()
    {
        if (m_menuType == Kernel::NoModelMenu)
        {
            return qobject_cast<QObject*>(m_recentFiles);
        }
        else 
        {
            return NULL;
        }

    }

    QObject* RClickMenuList::testModelsModel()
    {
        if (m_menuType == Kernel::NoModelMenu)
        {
            return qobject_cast<QObject*>(m_testModels);
        }
        else 
        {
            return NULL;
        }
    }

    QObject* RClickMenuList::nozzleModel()
    {
        return qobject_cast<QObject*>(m_nozzleRange);
    }

    QObject* RClickMenuList::uploadModel()
    {
        return qobject_cast<QObject*>(m_upAction);
    }

    void RClickMenuList::setMenuType(int type)
    {
        if (m_menuType == type)
            return;

        m_menuType = type;
    }

    int RClickMenuList::menuType()
    {
        return m_menuType;
    }

    QObject* RClickMenuList::getActionObject(const QString& actionName)
    {
        if (m_commandsMap.isEmpty())
            startAddCommads();
        return (QObject*)m_commandsMap[actionName];
    }

    QObject* RClickMenuList::getSubMenuObject(const QString& menuName)
    {
        if (menuName == "recent files")
        {
            return (QObject*)m_recentFiles;
        } 
        else if (menuName == "test models")
        {
            return (QObject*)m_testModels;
        }
        else 
        {
            return NULL;
        }
    }

    void RClickMenuList::setNozzleCount(int nCount)
    {
        m_nozzleRange->setNozzleCount(nCount);
    }

    void RClickMenuList::startAddCommads()
    {
        m_commandsMap["select all"] = (ActionCommand*)(new SelectPrinterAllAction(this));
        m_commandsMap["clear all"] = (ActionCommand*)(new ClearPrinterAction(this));
        m_commandsMap["pick bottom"] = (ActionCommand*)(new PickBottomAction(this));
        m_commandsMap["layout"] = (ActionCommand*)(new LayoutPlateAction(this));
        m_commandsMap["delete plate"] = (ActionCommand*)(new DeletePlateAction(this));
        m_commandsMap["lock plate"] = (ActionCommand*)(new LockPlateAction(this));
        m_commandsMap["cover model"] = (ActionCommand*)(new CoverModelAction(this));
        m_commandsMap["delete"] = (ActionCommand*)(new DeleteModelAction(this));
        m_commandsMap["split model"] = (ActionCommand*)(new SplitModelAction(this));
        m_commandsMap["import model"] = (ActionCommand*)(new ImportModelAction(this));

        SelectToolAction* moveAction = new SelectToolAction("move", 0, this);
        moveAction->setSource("qrc:/UI/photo/menuImg/move_n.svg");
        moveAction->setIcon("qrc:/UI/photo/menuImg/move_p.svg");
        moveAction->setShortcut("T");
        m_commandsMap["move"] = (ActionCommand*)moveAction;
        
        SelectToolAction* rotateAction = new SelectToolAction("rotate", 1, this);
        rotateAction->setSource("qrc:/UI/photo/menuImg/rotate_n.svg");
        rotateAction->setIcon("qrc:/UI/photo/menuImg/rotate_p.svg");
        // rotateAction->setShortcut("R");
        rotateAction->setShortcut("R");
        m_commandsMap["rotate"] = (ActionCommand*)(rotateAction);
        
        SelectToolAction* scaleAction = new SelectToolAction("scale", 10, this);
        scaleAction->setSource("qrc:/UI/photo/menuImg/scale_n.svg");
        scaleAction->setIcon("qrc:/UI/photo/menuImg/scale_p.svg");
        scaleAction->setShortcut("Ctrl+R");
        m_commandsMap["scale"] = (ActionCommand*)scaleAction;

        m_commandsMap["merge model"] = (ActionCommand*)(new MergeModelAction(this));
        m_commandsMap["merge model location"] = (ActionCommand*)(new MergeModelToPrinterAction(this));
        m_commandsMap["model setting"] = (ActionCommand*)(new ModelSettingAction(this));

    }
}
