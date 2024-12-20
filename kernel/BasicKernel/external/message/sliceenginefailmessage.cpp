#include "sliceenginefailmessage.h"
#include "data/modeln.h"
#include "interface/selectorinterface.h"
#include "interface/spaceinterface.h"
#include "interface/commandinterface.h"

namespace creative_kernel
{
    SliceEngineFailMessage::SliceEngineFailMessage(const QString& failReason, QObject* parent)
        : MessageObject(parent)
        , m_level(0)
        , m_failReasonDesc(failReason)
        , m_sceneObjectId(-1)
    {
        int specialPos = m_failReasonDesc.indexOf('@');
        QString sceneObjectIdStr = m_failReasonDesc.mid(specialPos + 1);

        if (sceneObjectIdStr.isEmpty())
        {
            int levelPos = m_failReasonDesc.lastIndexOf("##");
            if (levelPos < 0)
            {
                m_level = 0;  //warning; such as "user cancelled"
            }
            else
            {
                QString levelStr = m_failReasonDesc.mid(levelPos + 2, 1);
                if (!levelStr.isEmpty())
                {
                    m_level = levelStr.toInt();
                    m_failReasonDesc = m_failReasonDesc.mid(0, levelPos) + '@';
                }
            }

        }
        else
        {
            m_sceneObjectId = sceneObjectIdStr.toInt();
            m_level = 2;  // error
        }
    }

    SliceEngineFailMessage::~SliceEngineFailMessage()
    {
    }

    int SliceEngineFailMessage::codeImpl()
    {
        return MessageType::SliceEngineFail + m_sceneObjectId;
    }

    int SliceEngineFailMessage::levelImpl()
    {
        return m_level;
    }

    QString SliceEngineFailMessage::messageImpl()
    {
        int specialPos = m_failReasonDesc.indexOf('@');
        QString failReason = m_failReasonDesc.mid(0, specialPos);

        int part1Pos = failReason.indexOf("$$");
        int lastPartPos = failReason.lastIndexOf("$$");

        if (part1Pos < 0)
        {
            return tr(failReason.toLatin1());
        }
        else
        {
            QString part1 = failReason.mid(0, part1Pos);
            QString noNeedTranslationPart = failReason.mid(part1Pos+2, lastPartPos - part1Pos-2);
            QString lastPart = failReason.mid(lastPartPos+2);
            return  tr(part1.toLatin1()) + noNeedTranslationPart + tr(lastPart.toLatin1());
        }

    }

    QString SliceEngineFailMessage::linkStringImpl()
    {
        ModelGroup* mg = getModelGroupByObjectId(m_sceneObjectId);
        if (!mg)
        {
            return QString("");
        }
        else
        {
            return tr("Jump to ") + "[" + mg->groupName() + QString("]");
        }
    }

    void SliceEngineFailMessage::handleImpl()
    {
        ModelGroup* group = getModelGroupByObjectId(m_sceneObjectId);
        if (!group)
            return;

        setKernelPhase(KernelPhaseType::kpt_prepare);
        selectGroup(group);

    }

    void SliceEngineFailMessage::setLevel(int level)
    {
        m_level = level;
    }

    int64_t SliceEngineFailMessage::getRelatedSceneObjectId()
    {
        return m_sceneObjectId;
    }

    Crslice2InfoMessage::Crslice2InfoMessage(const QString& info, QObject* parent)
        : MessageObject(parent)
        , m_level(0)
        , m_info(info)
    {
    }

    Crslice2InfoMessage::~Crslice2InfoMessage()
    {
    }

    int Crslice2InfoMessage::codeImpl()
    {
        return MessageType::SliceEngineFail;
    }

    int Crslice2InfoMessage::levelImpl()
    {
        return m_level;
    }

    QString Crslice2InfoMessage::messageImpl()
    {
        return tr(m_info.toLatin1());
    }

    QString Crslice2InfoMessage::linkStringImpl()
    {
        return "";
    }

    void Crslice2InfoMessage::handleImpl()
    {

    }

    void Crslice2InfoMessage::setLevel(int level)
    {
        m_level = level;
    }
}