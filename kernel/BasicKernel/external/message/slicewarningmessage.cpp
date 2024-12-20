#include "slicewarningmessage.h"
#include <QtCore/QDebug>
#include "interface/spaceinterface.h"
#include "interface/selectorinterface.h"
#include "external/kernel/kernel.h"
#include "external/interface/commandinterface.h"

namespace creative_kernel
{
    SliceWarningMessage::SliceWarningMessage(const QMap<QString, QPair<QString, int64_t> >& warningDetails, QObject* parent)
        : MessageObject(parent)
        , m_warningDetails(warningDetails)
        , m_sceneObjId(-1)
        , m_needJumpTo(true)
    {
    }

	SliceWarningMessage::~SliceWarningMessage()
    {

    }

    int64_t SliceWarningMessage::getRelatedSceneObjectId()
    {
        return m_sceneObjId;
    }

    int SliceWarningMessage::codeImpl()
    {
        return MessageType::SliceEngineWarning + m_sceneObjId;
    }

    int SliceWarningMessage::levelImpl()
    {
        return int(MessageLevel::Warning);
    }

    QString SliceWarningMessage::messageImpl()
    {
        auto itrWarn = m_warningDetails.begin();
        for (; itrWarn != m_warningDetails.end(); itrWarn++)
        {
            m_sceneObjId = itrWarn.value().second;

            if (QString("Path_Conflict") == itrWarn.key())
            {
                m_needJumpTo = false;
                return pathConflictImpl(itrWarn.value().first);
            }
            else if (QString("SlicingNeedSupportOn") == itrWarn.key())
            {
                return sliceNeedSupportImpl(itrWarn.value().first);
            }
        }

    }

    QString SliceWarningMessage::pathConflictImpl(const QString& strDetails)
    {
        int part1Pos = strDetails.indexOf("#");
        int heightWordPos = strDetails.lastIndexOf("#");
        int heightValuePos = strDetails.indexOf("$");
        int lastPartPos = strDetails.lastIndexOf("$");
        int finalAtPos = strDetails.lastIndexOf("@");

        if (part1Pos < 0)
        {
            return tr(strDetails.toLatin1());
        }
        else
        {
            int len = strDetails.length();
            // Conflicts of gcode paths have been found at layer# 3, #height$ 0.800000 $ Please separate the conflicted objects further@ (长方体 <-> 长方体).
            QString part1 = strDetails.mid(0, part1Pos);
            QString layerValuePart = strDetails.mid(part1Pos + 1, heightWordPos - part1Pos - 1);
            QString heightWordPart = strDetails.mid(heightWordPos + 1, heightValuePos - heightWordPos - 1);
            QString heightValuePart = strDetails.mid(heightValuePos + 1, lastPartPos - heightValuePos - 1);
            QString lastPart = strDetails.mid(lastPartPos + 2, finalAtPos - lastPartPos - 2);
            QString objName1 = strDetails.mid(finalAtPos + 1); // objName1, could possibly be "WipeTower"

            QString objName2 = "";
            ModelGroup* group = getModelGroupByObjectId(m_sceneObjId);
            if (group)
            {
                objName2 = group->groupName();
            }

            //text = (boost::format(_u8L("Conflicts of gcode paths have been found at layer# %d, #height$ %.2f mm.$ Please separate the conflicted objects further@ (%s <-> %s).")) % layer % height %
            //    objName1 % objName2)
            //    .str();

            //text = (boost::format(_u8L("Conflicts of gcode paths have been found at layer# %d, #height$ %.2f mm.$ Please separate the conflicted objects further@%s")) % layer % height % objName1).str();

            return  tr(part1.toUtf8()) + layerValuePart + tr(heightWordPart.toUtf8()) + heightValuePart + tr(lastPart.toUtf8()) + QString("(") + tr(objName1.toUtf8()) + QString(" <-> ") + tr(objName2.toUtf8()) + QString(").");
        }
    }

    QString SliceWarningMessage::sliceNeedSupportImpl(const QString& strDetails)
    {
        QString objectName = "";
        ModelGroup* group = getModelGroupByObjectId(m_sceneObjId);
        if (group)
        {
            objectName = group->groupName();
        }

        //format(L("It seems object %s has %s. Please re-orient the object or enable support generation."),
        //"%s#Please re-orient the object or enable support generation."

        int part1Pos = strDetails.indexOf("#");
        if (part1Pos < 0)
        {
            return tr(strDetails.toUtf8());
        }

        QString part1 = strDetails.mid(0, part1Pos);
        QString part2 = strDetails.mid(part1Pos + 1);

        qDebug() << QString("SliceWarningMessage %1").arg(strDetails);
        //return tr(strDetails.toUtf8());

        return tr("It seems object ") + objectName + tr(" has ") + tr(part1.toUtf8()) + "." + tr(part2.toUtf8());
    }

    QString SliceWarningMessage::linkStringImpl()
    {
        if (!m_needJumpTo)
        {
            return QString("");
        }

        ModelGroup* group = getModelGroupByObjectId(m_sceneObjId);
        if (!group)
        {
            return QString("");
        }

        QString groupObjectName = group->groupName();

        if (groupObjectName.isEmpty())
            return QString("");

        return tr("Jump to ") + "[" + groupObjectName + QString("]");
    }

    void SliceWarningMessage::handleImpl()
    {
        if (!m_needJumpTo)
            return;

        ModelGroup* group = getModelGroupByObjectId(m_sceneObjId);
        if (!group)
            return;

        setKernelPhase(KernelPhaseType::kpt_prepare);
        selectGroup(group);
    }

}