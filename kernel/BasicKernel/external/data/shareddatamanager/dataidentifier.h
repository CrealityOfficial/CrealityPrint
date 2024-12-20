#ifndef CREATIVE_KERNEL_DATAIDENTIFIER_1681019989200_H
#define CREATIVE_KERNEL_DATAIDENTIFIER_1681019989200_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "ccglobal/serial.h"
#include <QObject>

namespace creative_kernel
{
	class BASIC_KERNEL_API DataIdentifier : public QObject
	{
	public:
		enum DataType 
		{
			None = 0,
			Mesh,
			Spread
		};

		DataIdentifier();
		virtual ~DataIdentifier();

		void generate(const trimesh::TriMesh* mesh);
		void generate(const std::vector<std::string>& spreadData);

		int size();

		bool operator==(const DataIdentifier& other) const;
		bool operator!=(const DataIdentifier& other) const;


	private:
		QByteArray formatTriMesh(const trimesh::TriMesh* mesh);
		QByteArray formatSpreadData(const std::vector<std::string>& spreadData);

	private:
		DataType m_type { None };
		int m_size { 0 };
		QByteArray m_identifier;

	};


}

#endif // CREATIVE_KERNEL_DATAIDENTIFIER_1681019989200_H