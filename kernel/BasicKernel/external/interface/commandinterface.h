#ifndef COMMAND_OPERATION_H
#define COMMAND_OPERATION_H
#include "basickernelexport.h"
#include "data/kernelenum.h"
#include <QtCore/QString>

namespace creative_kernel
{
	class ModelN;
	BASIC_KERNEL_API int cmdSaveAs(const QString& fileName);
	BASIC_KERNEL_API void cmdSaveSelectStl(const QString& filePath, QList<QString>& saveNames);

	BASIC_KERNEL_API int cmdClone(int numb);
	BASIC_KERNEL_API int cmdClone(QList<creative_kernel::ModelN*> copyModels);
	int doClone(const QList<creative_kernel::ModelN*>& selections, const int& num = 1);

	//sensor analytics
	BASIC_KERNEL_API void sensorAnlyticsTrace(const QString& eventType, const QString& eventAction);

	BASIC_KERNEL_API void openUserCourseLocation();
	BASIC_KERNEL_API void openLogLocation();
	BASIC_KERNEL_API void openModelLocation();
	BASIC_KERNEL_API void openUserFeedbackWebsite();
	BASIC_KERNEL_API void openCalibrationTutorialWebsite();
	BASIC_KERNEL_API void openFileLocation(const QString& file);
	BASIC_KERNEL_API void openUrl(const QString& web);
	class UIVisualTracer;
	BASIC_KERNEL_API void addUIVisualTracer(UIVisualTracer* tracer,QObject* base);
	BASIC_KERNEL_API void removeUIVisualTracer(UIVisualTracer* tracer);
	BASIC_KERNEL_API void changeTheme(ThemeCategory theme);
	BASIC_KERNEL_API void changeLanguage(MultiLanguage language);
	BASIC_KERNEL_API MultiLanguage currentLanguage();
	BASIC_KERNEL_API ThemeCategory currentTheme();

	BASIC_KERNEL_API QString generateSceneName();

	BASIC_KERNEL_API void setKernelPhase(KernelPhaseType phase);
}

#endif //