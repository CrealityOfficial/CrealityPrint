#include "infoplugin.h"

#include "interface/spaceinterface.h"
#include "interface/uiinterface.h"

#include "zsliderinfo.h"

using namespace creative_kernel;
InfoPlugin::InfoPlugin(QObject* parent)
	: QObject(parent)
    , m_btnUI(nullptr)
    //, m_zSliderUI(nullptr)
    , m_sliderInfo(nullptr)
{
    m_sliderInfo = new ZSliderInfo(this);
    registerContextObject("plugin_info_sliderinfo", m_sliderInfo);
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
}

void InfoPlugin::uninitialize()
{
    unTraceSpace(m_sliderInfo);
}
