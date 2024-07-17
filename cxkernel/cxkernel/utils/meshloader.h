#ifndef _NULLSPACE_MESHLOADERWRAPPER_1590982007351_H
#define _NULLSPACE_MESHLOADERWRAPPER_1590982007351_H

#include <list>
#include <set>

#include "cxkernel/cxkernelinterface.h"
#include "cxkernel/data/modelndata.h"
#include "qtusercore/module/cxhandlerbase.h"
#include "cxkernel/utils/meshjob.h"

namespace cxkernel
{
	class MeshLoader : public QObject, public qtuser_core::CXHandleBase, public MeshJobObserver
	{
		Q_OBJECT
	public:
		MeshLoader(QObject* parent = nullptr);
		virtual ~MeshLoader();

		void load(const QStringList& fileNames);
		void setParam(const ModelNDataCreateParam& param);
	public:
		QString filter() override;
		void handle(const QString& fileName) override;
		void handle(const QStringList& fileNames) override;

		void addModelFromCreateInput(const ModelCreateInput& input);
		void setModelNDataProcessor(ModelNDataProcessor* processor);

	protected:
		void onFinished(MeshJob* job) override;

	protected:
		std::list<QStringList> m_tasks;
		std::set<MeshJob*> m_jobs;
		ModelNDataProcessor* m_processor;
		ModelNDataCreateParam m_param;
	};
}
#endif // _NULLSPACE_MESHLOADERWRAPPER_1590982007351_H
