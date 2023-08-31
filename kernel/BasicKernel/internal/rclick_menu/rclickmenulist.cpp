#include "rclickmenulist.h"
#include"internal/rclick_menu/deletemodelaction.h"
#include"internal/rclick_menu/importmodelaction.h"
#include"internal/rclick_menu/cloneaction.h"
#include"internal/rclick_menu/selectallaction.h"
#include"internal/rclick_menu/clearallaction.h"
#include"internal/rclick_menu/resetallaction.h"
#include"internal/rclick_menu/placeonprinterbed.h"
#include"internal/rclick_menu/mergemodelaction.h"
#include"internal/rclick_menu/splitmodelaction.h"
#include "internal/rclick_menu/setnozzlerange.h"
#include"internal/rclick_menu/uploadmodelaction.h"
#include "internal/rclick_menu/layoutcommand.h"

#include "interface/machineinterface.h"

#include "kernel/kernel.h"

namespace creative_kernel
{
    RClickMenuList::RClickMenuList(QObject* parent)
        : QObject(parent)
        , m_nozzleRange(nullptr)
        , m_upAction(nullptr)
    {
    }

    RClickMenuList::~RClickMenuList()
    {

    }

    void RClickMenuList::addActionCommad(ActionCommand* pAction)
    {
        if (pAction)
        {
            m_varCommandList.push_back(pAction);
        }
    }

    void RClickMenuList::addActionCommad(ActionCommand* pAction, int index)
    {
        if (pAction == nullptr)
            return;
        if (index > m_varCommandList.size())
            return;

        m_varCommandList.insert(index, pAction);
    }

    void RClickMenuList::addActionCommad(ActionCommand* pAction, QString strInfo)
    {
        if (pAction == nullptr || m_varCommandList.isEmpty())
            return;

        int index = indexOfString(strInfo);
        m_varCommandList.insert(index + 1, pAction);
    }

    int RClickMenuList::indexOfString(QString strInfo)
    {
        int index = -1;
        if (m_varCommandList.isEmpty())
            return index;

        for (int i = 0; i < m_varCommandList.size(); i++)
        {
            if (m_varCommandList[i]->name() == strInfo)
            {
                index = i;
                break;
            }
        }
        return  index;
    }

    void RClickMenuList::removeActionCommand(ActionCommand* command)
    {
        int index = m_varCommandList.indexOf(command);
        if (index >= 0 && index < m_varCommandList.size())
        {
            m_varCommandList.removeAt(index);
        }
    }

    QVariantList RClickMenuList::getCommandsList()
    {
        startAddCommads();
        setNozzleCount(currentMachineExtruderCount());
        QVariantList datas;
        for (ActionCommand* command : m_varCommandList) {
            datas.append(QVariant::fromValue(command));
        }
        return datas;
    }
    QObject* RClickMenuList::getNozzleModel()
    {
        return qobject_cast<QObject*>(m_nozzleRange);
    }

    QObject* RClickMenuList::getUploadModel()
    {
        return qobject_cast<QObject*>(m_upAction);
    }

    void RClickMenuList::setNozzleCount(int nCount)
    {
        m_nozzleRange->setNozzleCount(nCount);
    }

    void RClickMenuList::startAddCommads()
    {
        if (!m_nozzleRange)
            m_nozzleRange = new SetNozzleRange(this);

        if (!m_upAction)
            m_upAction = new UploadModelAction(this);

        if (m_varCommandList.isEmpty())
        {
            ImportModelAction* pFile = new ImportModelAction(this);
            addActionCommad(pFile);

            DeleteModelAction* pFile2 = new DeleteModelAction(this);
            addActionCommad(pFile2);
            CloneAction* clone = new CloneAction(this);
            addActionCommad(clone);
            MergeModelAction* merge = new MergeModelAction(this);
            addActionCommad(merge);
            merge->setBSeparator(true);
            SplitModelAction* split = new SplitModelAction(this);
            addActionCommad(split);

            SelectAllAction* select = new SelectAllAction(this);
            addActionCommad(select);
            select->setBSeparator(true);
            LayoutCommand* layout = new LayoutCommand(this);
            addActionCommad(layout);

            ResetAllAction* resetall = new ResetAllAction(this);
            addActionCommad(resetall);

            ClearAllAction* clearAll = new ClearAllAction(this);
            addActionCommad(clearAll);

            PlaceOnPrinterBed* placeBed = new PlaceOnPrinterBed(this);
            addActionCommad(placeBed);
            placeBed->setBSeparator(true);
            //OneSideAsBottomFace* oneSide = new OneSideAsBottomFace();
            //addActionCommad(oneSide);

            //UploadModelAction* uploadModel = new UploadModelAction(this);
            //addActionCommad(uploadModel);
            //addActionCommad(m_nozzleRange);
        }
        else {
            Q_FOREACH(ActionCommand * pAction, m_varCommandList)
            {
                pAction->update();
            }
        }
    }
}
