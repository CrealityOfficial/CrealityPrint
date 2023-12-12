#ifndef _NULLSPACE_MESHLOADERWRAPPER_1590982007351_H
#define _NULLSPACE_MESHLOADERWRAPPER_1590982007351_H
#include "cxkernel/cxkernelinterface.h"
#include "cxkernel/data/modelndata.h"
#include "qtusercore/module/cxhandlerbase.h"

namespace cxkernel
{
	class MeshLoader : public QObject, public qtuser_core::CXHandleBase
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
		ModelNDataProcessor* m_processor;

		ModelNDataCreateParam m_param;
	};
}
#endif // _NULLSPACE_MESHLOADERWRAPPER_1590982007351_H
