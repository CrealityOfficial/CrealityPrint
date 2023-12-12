#ifndef CXKERNEL_MODELNDATASERIAL_1691655235363_H
#define CXKERNEL_MODELNDATASERIAL_1691655235363_H
#include "cxkernel/data/modelndata.h"
#include "ccglobal/serial.h"

namespace cxkernel
{
	class CXKERNEL_API ModelNDataSerial : public ccglobal::Serializeable
	{
	public:
		ModelNDataSerial();
		virtual ~ModelNDataSerial();

		void setData(ModelNDataPtr data);
		ModelNDataPtr getData();

		void load(const QString& fileName, ccglobal::Tracer* tracer);
		void save(const QString& fileName, ccglobal::Tracer* tracer);

		int version() override;
		bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
		bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;
	protected:
		ModelNDataPtr m_data;
	};
}

#endif // CXKERNEL_MODELNDATASERIAL_1691655235363_H