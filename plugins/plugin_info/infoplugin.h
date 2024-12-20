#ifndef _NULLSPACE_INFOPLUGIN_1589979999085_H
#define _NULLSPACE_INFOPLUGIN_1589979999085_H
#include "qtusercore/module/creativeinterface.h"
#include "infoprovider.h"
class ZSliderInfo;
namespace {
	class InfoProvider;
}

class InfoPlugin : public QObject, public CreativeInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "creative.InfoPlugin")
	Q_INTERFACES(CreativeInterface)

public:
	InfoPlugin(QObject* parent = nullptr);
	virtual ~InfoPlugin();

	QString name() override;
	QString info() override;

	void initialize() override;
	void uninitialize() override;

protected:
    QObject* m_btnUI;
    //QObject* m_zSliderUI;

    ZSliderInfo *m_sliderInfo;
	creative_kernel::InfoProvider *m_InfoProvider;
};
#endif // _NULLSPACE_INFOPLUGIN_1589979999085_H
