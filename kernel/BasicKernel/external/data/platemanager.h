#ifndef CREATIVE_KERNEL_PLATEMANAGER_1592037009137_H
#define CREATIVE_KERNEL_PLATEMANAGER_1592037009137_H
#include "basickernelexport.h"

namespace creative_kernel
{
	class BASIC_KERNEL_API PrintPlate : public QObject
	{
		Q_OBJECT
	public:
		PrintPlate(QObject* parent = nullptr);
		virtual ~PrintPlate();

		int64_t ID();
		void setPlateID(int64_t id);
	protected:
		int64_t m_id;
	};

	class BASIC_KERNEL_API PlatesManager : public QObject
	{
		Q_OBJECT
	public:
		PlatesManager(QObject* parent = nullptr);
		virtual ~PlatesManager();

		void appendPrintPlate(PrintPlate* plate);
		void resizePrintPlates(int size);
		void removePrintPlate(PrintPlate* plate);
		int printPlatesCount();

		PrintPlate* currentPrintPlate();
	protected:
		QList<PrintPlate*> m_plates;
		PrintPlate* m_current_plate;
	};
}
#endif // CREATIVE_KERNEL_PLATEMANAGER_1592037009137_H