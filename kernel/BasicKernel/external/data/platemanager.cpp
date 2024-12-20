#include "platemanager.h"

namespace creative_kernel
{
	PrintPlate::PrintPlate(QObject* parent)
		: QObject(parent)
		, m_id(-1)
	{

	}

	PrintPlate::~PrintPlate()
	{

	}

	int64_t PrintPlate::ID()
	{
		return m_id;
	}

	void PrintPlate::setPlateID(int64_t id)
	{
		m_id = id;
	}

	PlatesManager::PlatesManager(QObject* parent)
		: QObject(parent)
		, m_current_plate(nullptr)
	{

	}

	PlatesManager::~PlatesManager()
	{

	}

	void PlatesManager::appendPrintPlate(PrintPlate* plate)
	{

	}

	void PlatesManager::resizePrintPlates(int size)
	{

	}

	void PlatesManager::removePrintPlate(PrintPlate* plate)
	{

	}

	int PlatesManager::printPlatesCount()
	{
		return 1;
	}

	PrintPlate* PlatesManager::currentPrintPlate()
	{
		return m_current_plate;
	}
}