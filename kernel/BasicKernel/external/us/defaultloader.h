#ifndef _US_DEFAULTLOADER_1589458301532_H
#define _US_DEFAULTLOADER_1589458301532_H
#include "basickernelexport.h"

namespace creative_kernel
{
	struct MachineData;
}

namespace us
{
	class USettings;
	class BASIC_KERNEL_API DefaultLoader: public QObject
	{
	public:
		DefaultLoader(QObject* parent = nullptr);
		virtual ~DefaultLoader();

		void loadDefault(const QString& file, USettings* uSettings, QList<us::USettings*>* extruderSettings = nullptr, creative_kernel::MachineData* data = nullptr);

	private:
		USettings* m_defRecommendSetting;
		QString m_configRoot;
	};
}
#endif // _US_DEFAULTLOADER_1589458301532_H
