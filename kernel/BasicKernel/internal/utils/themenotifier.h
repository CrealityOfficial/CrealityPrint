#ifndef CREATIVE_KERNEL_THEMENOTIFIER_1670475882513_H
#define CREATIVE_KERNEL_THEMENOTIFIER_1670475882513_H
#include <QtCore/QObject>
#include "data/interface.h"

namespace creative_kernel
{
	class ThemeNotifier : public QObject, public UIVisualTracer
	{
	public:
		ThemeNotifier(QObject* parent = nullptr);
		virtual ~ThemeNotifier();

		void onThemeChanged(ThemeCategory category) override;
		void onLanguageChanged(MultiLanguage language) override;
	};
}

#endif // CREATIVE_KERNEL_THEMENOTIFIER_1670475882513_H