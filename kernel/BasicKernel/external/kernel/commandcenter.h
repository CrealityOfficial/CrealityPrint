#ifndef CREATIVE_KERNEL_COMMANDCENTER_1672888329478_H
#define CREATIVE_KERNEL_COMMANDCENTER_1672888329478_H
#include <QtCore/QObject>

namespace creative_kernel
{
	class CommandCenter : public QObject
	{
		Q_OBJECT
	public:
		CommandCenter(QObject* parent = nullptr);
		virtual ~CommandCenter();

		Q_INVOKABLE void openUserCourseLocation();
		Q_INVOKABLE void openUserFeedbackWebsite();
		Q_INVOKABLE void openCalibrationTutorialWebsite();
		Q_INVOKABLE void openUseTutorialWebsite();
		Q_INVOKABLE void openLogLocation();
		Q_INVOKABLE void openModelLocation();
		Q_INVOKABLE void openUrl(const QString& web);
		Q_INVOKABLE void openCloudTutorialWeb(const QString& name);

		void openFileLocation(const QString& file);
		QString generateSceneName();
	};
}

#endif // CREATIVE_KERNEL_COMMANDCENTER_1672888329478_H