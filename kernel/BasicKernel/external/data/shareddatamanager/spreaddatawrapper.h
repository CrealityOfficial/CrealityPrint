#ifndef CREATIVE_KERNEL_SPREADDATAWRAPPER_1681019989200_H
#define CREATIVE_KERNEL_SPREADDATAWRAPPER_1681019989200_H
#include "data/header.h"
#include "data/kernelenum.h"
#include "dataserial.h"

namespace creative_kernel
{
	class SharedDataManager;
	class BASIC_KERNEL_API SpreadDataWrapper : public DataSerial
	{
		friend class SharedDataManager;
	public:
		virtual ~SpreadDataWrapper();

	private:
		SpreadDataWrapper(int id, const std::vector<std::string>& data, const QString& dataName);

		virtual bool load();
		virtual bool store();
		virtual void generateIdentifier();
		virtual void clear();

		std::vector<std::string> data();

		void setUsedIndex(const QSet<int>& usedIndexes);
		bool hasIndex(int index);
		bool hasIndexMoreThan(int index);

		void discardIndex(int index);	//删除细分数据中指定的索引
		void discardIndexMoreThan(int index); //删除大于指定数值的索引

	private:
		std::vector<std::string> m_data;
		QSet<int> m_usedIndexes;


	};
}

#endif // CREATIVE_KERNEL_SPREADDATAWRAPPER_1681019989200_H