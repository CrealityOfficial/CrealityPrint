#ifndef _NULLSPACE_CADLoaderWRAPPER_1590982007351_H
#define _NULLSPACE_CADLoaderWRAPPER_1590982007351_H
#include "cxkernel/cxkernelinterface.h"
#include "cxkernel/data/modelndata.h"
#include "qtusercore/module/cxhandlerbase.h"

namespace creative_kernel
{
	class CADLoader : public QObject, public qtuser_core::CXHandleBase
	{
		Q_OBJECT
	public:
		CADLoader(QObject* parent = nullptr);
		virtual ~CADLoader();

		void load(const QStringList& fileNames);
	public:
		QString filter() override;
		void handle(const QString& fileName) override;
		void handle(const QStringList& fileNames) override;

		void setModelNDataProcessor(cxkernel::ModelNDataProcessor* processor);

	protected:
		cxkernel::ModelNDataProcessor* m_processor;
	};
}
#endif // _NULLSPACE_CADLoaderWRAPPER_1590982007351_H
