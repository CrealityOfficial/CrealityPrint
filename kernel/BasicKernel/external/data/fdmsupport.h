#ifndef CREATIVE_KERNEL_FDMSUPPORT_1595470868902_H
#define CREATIVE_KERNEL_FDMSUPPORT_1595470868902_H
#include "basickernelexport.h"
#include "data/fdmsupportparam.h"
#include "qtuser3d/module/chunkbufferuser.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API FDMSupport : public QObject, public qtuser_3d::ChunkBufferUser
	{
		Q_OBJECT
	public:
		FDMSupport(QObject* parent = nullptr);
		virtual ~FDMSupport();

		void setParam(FDMSupportParam& param);
		FDMSupportParam& param();
	protected:
		int updatePosition(QByteArray& data) override;
		void updateBuffer(float* buffer) override;
	protected:
		FDMSupportParam m_param;
	};
}
#endif // CREATIVE_KERNEL_FDMSUPPORT_1595470868902_H