#ifndef _CXGCODE_DEFAULTLOADER_1589458301532_H
#define _CXGCODE_DEFAULTLOADER_1589458301532_H
#include "cxgcode/interface.h"
#include <QtCore/QObject>

namespace cxgcode
{
	class USettings;
	class CXGCODE_API DefaultLoader: public QObject
	{
	public:
		DefaultLoader(QObject* parent = nullptr);
		virtual ~DefaultLoader();

		void loadDefault(const QString& file, USettings* uSettings);
		void setConfigRoot(const QString& root);
	private:
		USettings* m_defRecommendSetting;
		QString m_configRoot;
	};
}
#endif // _CXGCODE_DEFAULTLOADER_1589458301532_H
