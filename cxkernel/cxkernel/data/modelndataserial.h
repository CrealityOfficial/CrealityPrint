#ifndef CXKERNEL_MODELNDATASERIAL_1691655235363_H
#define CXKERNEL_MODELNDATASERIAL_1691655235363_H
#include "cxkernel/data/modelndata.h"
#include "cxkernel/data/meshdata.h"
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

		void setMeshData(cxkernel::MeshDataPtr meshData);
		void setColorData(const std::vector<std::string>& colors);
		void setSeamData(const std::vector<std::string>& seams);
		void setSupportData(const std::vector<std::string>& supports);

		void load(const QString& fileName, ccglobal::Tracer* tracer);
		void save(const QString& fileName, ccglobal::Tracer* tracer);

		int version() override;
		bool save(std::fstream& out, ccglobal::Tracer* tracer) override;
		bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override;
	protected:
		ModelNDataPtr m_data;
		cxkernel::MeshDataPtr m_meshData;
		std::vector<std::string> m_colors;
		std::vector<std::string> m_seams;
		std::vector<std::string> m_supports;
	};
}

#endif // CXKERNEL_MODELNDATASERIAL_1691655235363_H