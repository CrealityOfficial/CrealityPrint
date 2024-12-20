#include "appsettinginterface.h"
#include "kernel/kernel.h"
#include "qtusercore/util/applicationconfiguration.h"

namespace creative_kernel
{
	void registerBundle(const QString& bundle)
	{
		getKernel()->appConfiguration()->registerBundle(bundle);
	}

	QColor getSettingColor(const QString& bundle, const QString& key, const QString& group)
	{
		return getKernel()->appConfiguration()->color(bundle, key, group);
	}

	QVector4D getSettingVector4D(const QString& bundle, const QString& key, const QString& group)
	{
		return getKernel()->appConfiguration()->vector4(bundle, key, group);
	}

	QVariantList getSettingVariantList(const QString& bundle, const QString& key, const QString& group)
	{
		return getKernel()->appConfiguration()->varlist(bundle, key, group);
	}

	QString getSettingString(const QString& bundle, const QString& key, const QString& group)
	{
		return getKernel()->appConfiguration()->string(bundle, key, group);
	}

	QString version()
	{
		return getKernel()->globalConst()->version();
	}

	QString userCoursePath()
	{
		return getKernel()->globalConst()->userCoursePath();
	}

	QString userFeedbackWebsite()
	{
		return getKernel()->globalConst()->userFeedbackWebsite();
	}

	QString calibrationTutorialWebsite()
	{
		return getKernel()->globalConst()->calibrationTutorialWebsite();
	}
	QString useTutorialWebsite()
	{
		return getKernel()->globalConst()->useTutorialWebsite();
	}
	QString multiColorGuide()
	{
		return getKernel()->globalConst()->multiColorGuide();
	}
	QString officialWebsite()
	{
		return getKernel()->globalConst()->officialWebsite();
	}

	QString cloudTutorialWeb(const QString& name)
	{
		return getKernel()->globalConst()->cloudTutorialWeb(name);
	}

	QString languageName(MultiLanguage language)
	{
		return getKernel()->globalConst()->languageName(language);
	}

	QString languageTsFile(MultiLanguage language)
	{
		return getKernel()->globalConst()->languageTsFile(language);
	}

	MultiLanguage tsFile2Language(const QString& tsFile)
	{
		return getKernel()->globalConst()->tsFile2Language(tsFile);
	}

	QString themeName(ThemeCategory theme)
	{
		return getKernel()->globalConst()->themeName(theme);
	}

	QString writableLocation(const QString& subDir, const QString& subSubDir)
	{
		return getKernel()->globalConst()->writableLocation(subDir, subSubDir);
	}

	QString getResourcePath(ResourcesType resource)
	{
		return getKernel()->globalConst()->getResourcePath(resource);
	}

	QString getEnginePathPrefix()
	{
		return getKernel()->globalConst()->getEnginePathPrefix();
	}

	EngineType getEngineType()
	{
		return getKernel()->globalConst()->getEngineType();
	}
	QString getEngineVersion()
	{
		return getKernel()->globalConst()->getEngineVersion();
	}
}
