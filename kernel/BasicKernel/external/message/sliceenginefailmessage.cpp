#include "sliceenginefailmessage.h"
#include "data/modeln.h"
#include "interface/selectorinterface.h"
#include "interface/modelinterface.h"

namespace creative_kernel
{
    SliceEngineFailMessage::SliceEngineFailMessage(const QString& failReason, QObject* parent)
        : MessageObject(parent)
        , m_level(0)
        , m_failReasonDesc(failReason)
    {
        int specialPos = m_failReasonDesc.indexOf('@');
        QString modelObjectName = m_failReasonDesc.mid(specialPos + 1);

        if (modelObjectName.isEmpty())
            m_level = 0;  //warning; such as "user cancelled"
        else
            m_level = 2;  // error
    }

    SliceEngineFailMessage::~SliceEngineFailMessage()
    {
    }

    int SliceEngineFailMessage::codeImpl()
    {
        return MessageType::SliceEngineFail;
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
        int specialPos = m_failReasonDesc.indexOf('@');
        QString modelObjectName = m_failReasonDesc.mid(specialPos+1);

        if (modelObjectName.isEmpty())
            return QString("");

        return tr("Jump to ") + "[" + modelObjectName + QString("]");
    }

    void SliceEngineFailMessage::handleImpl()
    {
        int specialPos = m_failReasonDesc.indexOf('@');
        QString modelObjectName = m_failReasonDesc.mid(specialPos + 1);

        if (modelObjectName.isEmpty())
            return;

        ModelN* model = getModelNByObjectName(modelObjectName);
        if (model)
        {
            QList<qtuser_3d::Pickable*> pickables;
            pickables << model;

            unselectAll();
            selectMore(pickables);
        }

    }

    void SliceEngineFailMessage::setLevel(int level)
    {
        m_level = level;
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