#ifndef QTUSER_CORE_PROJECTINFORMATION_1609402097688_H
#define QTUSER_CORE_PROJECTINFORMATION_1609402097688_H
#include "qtusercore/qtusercoreexport.h"
#include "qtusercore/util/version.h"

namespace qtuser_core
{
	class QTUSER_CORE_API ProjectInformation
	{
	public:
		ProjectInformation();
		~ProjectInformation();

		const Version& version() const;
		void setVersion(const Version& version);

		QString name();
		void setName(const QString& name);

		void print();
	protected:
		Version m_version;
		QString m_projectName;
	};
}

#endif // QTUSER_CORE_PROJECTINFORMATION_1609402097688_H
