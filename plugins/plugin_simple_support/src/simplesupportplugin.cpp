#include "simplesupportplugin.h"
#include "supportcommand.h"

#include "supportui.h"
#include "interface/selectorinterface.h"
#include "interface/uiinterface.h"

namespace creative_kernel
{
    SimpleSupportPlugin::SimpleSupportPlugin(QObject* parent)
        :QObject(parent)
        , m_supportUI(nullptr)
        //,m_command(nullptr)
    {
    }

    SimpleSupportPlugin::~SimpleSupportPlugin()
    {
    }

    QString SimpleSupportPlugin::name()
    {
        return "SimpleSupportPlugin";
    }

    QString SimpleSupportPlugin::info()
    {
        return "SimpleSupportPlugin";
    }

    void SimpleSupportPlugin::initialize()
    {
        m_supportUI = new SupportUI(this);
        m_supportUI->showOnKernelUI(true);
        m_supportUI->setFDMVisible(true);
        //addSelectTracer(m_supportUI);

        registerContextObject("supportUICpp", m_supportUI);
    }

    void SimpleSupportPlugin::uninitialize()
    {
    }
}
