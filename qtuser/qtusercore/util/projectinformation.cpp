#include "projectinformation.h"
#include <QtCore/QDebug>

namespace qtuser_core
{
	ProjectInformation::ProjectInformation()
	{

	}

	ProjectInformation::~ProjectInformation()
	{

	}

	const Version& ProjectInformation::version() const
	{
		return m_version;
	}

	void ProjectInformation::setVersion(const Version& version)
	{
		m_version = version;
	}

	QString ProjectInformation::name()
	{
		return m_projectName;
	}

	void ProjectInformation::setName(const QString& name)
	{
		m_projectName = name;
	}

	void ProjectInformation::print()
	{
		qInfo() << "ProjectName : " << m_projectName;
		qInfo() << "Version : " << m_version.versionFullName();
	}
}