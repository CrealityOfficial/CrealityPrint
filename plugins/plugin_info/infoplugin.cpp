#include "infoplugin.h"

#include "interface/spaceinterface.h"
#include "interface/uiinterface.h"
#include "interface/selectorinterface.h"
#include "zsliderinfo.h"
#include "infoprovider.h"
using namespace creative_kernel;
InfoPlugin::InfoPlugin(QObject* parent)
	: QObject(parent)
    , m_btnUI(nullptr)
    //, m_zSliderUI(nullptr)
    , m_sliderInfo(nullptr)
{
    m_sliderInfo = new ZSliderInfo(this);
    registerContextObject("plugin_info_sliderinfo", m_sliderInfo);
    m_InfoProvider = new creative_kernel::InfoProvider(this);
    registerContextObject("plugin_info", m_InfoProvider);
}

InfoPlugin::~InfoPlugin()
{

}

QString InfoPlugin::name()
{
	return "InfoPlugin";
}

QString InfoPlugin::info()
{
	return "InfoPlugin";
}

void InfoPlugin::initialize()
{
    traceSpace(m_sliderInfo);
    traceSpace(m_InfoProvider);

    addModelNSelectorTracer(m_InfoProvider);
}

void InfoPlugin::uninitialize()
{
    unTraceSpace(m_sliderInfo);
    unTraceSpace(m_InfoProvider);
}
