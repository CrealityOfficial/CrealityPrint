#include "commandcenter.h"

#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>

#include "interface/appsettinginterface.h"
#include "interface/spaceinterface.h"
#include "interface/machineinterface.h"
#include "data/modeln.h"

namespace creative_kernel
{
	CommandCenter::CommandCenter(QObject* parent)
		: QObject(parent)
	{

	}

	CommandCenter::~CommandCenter()
	{

	}

	void CommandCenter::openUserCourseLocation()
	{
		QFileInfo info(userCoursePath());
		QDesktopServices::openUrl(QUrl::fromLocalFile(info.path()));
	}

	void CommandCenter::openUserFeedbackWebsite()
	{
		QString urlStr = userFeedbackWebsite();
		QDesktopServices::openUrl(QUrl(urlStr));
	}

	void CommandCenter::openCalibrationTutorialWebsite()
	{
		QString urlStr = calibrationTutorialWebsite();
		QDesktopServices::openUrl(QUrl(urlStr));
	}

	void CommandCenter::openLogLocation()
	{
		QFileInfo info(LOG_PATH);
		QDesktopServices::openUrl(QUrl::fromLocalFile(info.path()));
	}

	void CommandCenter::openModelLocation()
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(MODEL_PATH));
	}

	void CommandCenter::openUrl(const QString& web)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(web));
	}

	void CommandCenter::openCloudTutorialWeb(const QString& name)
	{
		openUrl(cloudTutorialWeb(name));
	}

	void CommandCenter::openFileLocation(const QString& file)
	{
		QFileInfo info(file);
		QDesktopServices::openUrl(QUrl::fromLocalFile(info.absolutePath()));
	}

	QString CommandCenter::generateSceneName()
	{
		QString name = currentMachineName();
		QList<ModelN*> models = modelns();
		for (ModelN* model : models)
		{
			QFileInfo info(model->objectName());
			name += QString("-%1").arg(info.baseName());
		}
		return name;
	}
}