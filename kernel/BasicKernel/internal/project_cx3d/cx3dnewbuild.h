#ifndef CREATIVE_KERNEL_CX3DNEWBUILD_H
#define CREATIVE_KERNEL_CX3DNEWBUILD_H
#include "qtusercore/module/cxhandlerbase.h"
namespace creative_kernel
{
	class CX3DNewBuild : public QObject, public qtuser_core::CXHandleBase
	{
		Q_OBJECT
	public:
		CX3DNewBuild(QObject* parent = nullptr);
		virtual ~CX3DNewBuild();

	public:
		QString filter() override;
		void handle(const QString& fileName) override;
		void handle(const QStringList& fileNames) override;

	};
}

#endif // CREATIVE_KERNEL_CX3DNEWBUILD_H