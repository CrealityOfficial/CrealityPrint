#include "localnetplugin.h"
#include "interface/uiinterface.h"

#include <QSettings>

using namespace creative_kernel;

LocalNetPlugin::LocalNetPlugin(QObject* parent) : QObject(parent)
{
	m_PrinterList = new CusListModel(this);
	m_SearchMacList = new CusScanModel(this);
	m_GcodeFileList = new GcodeListModel(this);
	m_HistoryFileList = new HistoryListModel(this);
	m_videoList = new VideoListModel(this);
	m_materialBoxList = new MaterialBoxListModel(this);

	m_fliterModel = new FliterProxyModel(this);
	m_fliterModel->setSourceModel(m_PrinterList);

	m_gcodeSortModel = new SortProxyModel(this);
	m_gcodeSortModel->setSourceModel(m_GcodeFileList);

	registerContextObject("printerListModel", m_fliterModel);
	registerContextObject("searchMacListModel", m_SearchMacList);
	registerContextObject("gcodeFileListModel", m_gcodeSortModel);
	registerContextObject("historyFileListModel", m_HistoryFileList);
	registerContextObject("videoListModel", m_videoList);
	registerContextObject("materialBoxListModel", m_materialBoxList);
}

LocalNetPlugin::~LocalNetPlugin()
{

}

QString LocalNetPlugin::name()
{
	return "InfoPlugin";
}

QString LocalNetPlugin::info()
{
	return "";
}

void LocalNetPlugin::initialize()
{
	m_PrinterList->initialize();
}

void LocalNetPlugin::uninitialize()
{
	m_PrinterList->uninitialize();
}

