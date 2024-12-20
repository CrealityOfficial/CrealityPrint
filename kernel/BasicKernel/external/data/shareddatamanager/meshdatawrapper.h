#ifndef CREATIVE_KERNEL_MODELNDATAWRAPPER_1681019989200_H
#define CREATIVE_KERNEL_MODELNDATAWRAPPER_1681019989200_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "dataserial.h"
#include "cxkernel/data/meshdata.h"

namespace creative_kernel
{
	class SharedDataManager;
	class BASIC_KERNEL_API MeshDataWrapper : public DataSerial
	{
		friend class SharedDataManager;
	public:
		virtual ~MeshDataWrapper();

	public:
		MeshDataWrapper(int id, cxkernel::MeshDataPtr data);

		virtual bool load();
		virtual bool store();
		virtual void generateIdentifier();
		virtual void clear();

		cxkernel::MeshDataPtr data();

		void checkMesh();
		bool needRepair();

	private:
		cxkernel::MeshDataPtr m_data;
		bool m_needRepair;
	};

}

#endif // CREATIVE_KERNEL_MODELNDATAWRAPPER_1681019989200_H