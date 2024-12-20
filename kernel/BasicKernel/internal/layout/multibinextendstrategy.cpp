#include "multibinextendstrategy.h"
#include "interface/printerinterface.h"
#include "qtuser3d/trimesh2/conv.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{
		// ====  another multiBin layout strategy ====

	MultiBinExtendStrategy::MultiBinExtendStrategy(int priorBin)
		: nestplacer::BinExtendStrategy()
		, m_hasWipeTowerFlag(false)
		, m_priorBinId(priorBin)
	{
		m_box0 = getPrinter(0)->box();
		m_estimateBoxSizes = printersCount();
		m_estimateBoxs = estimateGlobalBoxBeenLayout(getPrinter(0)->box(), m_estimateBoxSizes);
	}

	MultiBinExtendStrategy::~MultiBinExtendStrategy()
	{
	}

	// this func returns the box of the index layout printer, 
	// "index" is likely bigger than total printers count of current scene, when needing to generate a new bin
	trimesh::box3 MultiBinExtendStrategy::bounding(int index) const
	{
		if (index < 0)
		{
			// this case should never happen
			return qtuser_3d::qBox32box3(m_box0);
		}

		int firstUnlockIdx = getPriorPrinterIndex(index);

		Printer* firstUnlockPrinter = getPrinter(firstUnlockIdx);
		if (!firstUnlockPrinter)
		{
			//// need to generate new printer
			if (firstUnlockIdx >= 0 && firstUnlockIdx < m_estimateBoxs.size())
				return qtuser_3d::qBox32box3(m_estimateBoxs[firstUnlockIdx]);

			return qtuser_3d::qBox32box3(m_box0);

		}

		qtuser_3d::Box3D printerBox = getPrinter(firstUnlockIdx)->globalBox();
		return qtuser_3d::qBox32box3(printerBox);

	}

	// this func returns the box in which the fixItem resides 
	trimesh::box3 MultiBinExtendStrategy::fixBounding(int fixBinId) const
	{
		//Q_ASSERT(fixBinId >= 0 && fixBinId < printersCount());
		if (fixBinId < 0 || fixBinId >= printersCount())
		{
			return qtuser_3d::qBox32box3(m_box0);
		}

		int firstUnlockIdx = 0;

		firstUnlockIdx = getPriorPrinterIndex(fixBinId);

		//Q_ASSERT(firstUnlockIdx >= 0 && firstUnlockIdx < printersCount());
		if (firstUnlockIdx < 0 || firstUnlockIdx >= printersCount())
		{
			return qtuser_3d::qBox32box3(m_box0);
		}

		qtuser_3d::Box3D printerBox = getPrinter(firstUnlockIdx)->globalBox();
		return qtuser_3d::qBox32box3(printerBox);
	}

	// --
	std::vector<int> MultiBinExtendStrategy::getPriorIndexArray() const
	{
		int lockedPrintersNum = getLockedPrintersNum();

		std::vector<int> numbers;
		//int printerCount = printersCount() - lockedPrintersNum;
		int printerCount = m_estimateBoxSizes - lockedPrintersNum;
		if (printerCount <= 0)
		{
			// all printers could possibly be locked
			printerCount = printersCount() + 1;
			numbers.push_back(printerCount - 1);
		}
		else
		{
			numbers.reserve(printerCount);

			for (int i = 0; i < m_estimateBoxSizes; ++i)
			{
				if (i < printersCount())
				{
					if (!getPrinter(i)->lock())
					{
						numbers.emplace_back(i);
					}
				}
				else
				{
					numbers.emplace_back(i);
				}

			}
		}


		auto iter = std::find(numbers.begin(), numbers.end(), m_priorBinId);
		if (iter != numbers.end()) {
			std::rotate(numbers.begin(), iter, iter + 1);
		}

		return numbers;
	}

	int MultiBinExtendStrategy::getPriorPrinterIndex(int resultIndex) const
	{
		std::vector<int> numbers = getPriorIndexArray();

		if (resultIndex >= 0 && resultIndex < numbers.size())
			return numbers[resultIndex];

		if (resultIndex >= numbers.size())
			return 0;

		qWarning() << Q_FUNC_INFO << "resultIndex : " << resultIndex << "  out of range";

		return 0;
	}

	void MultiBinExtendStrategy::setEstimateBoxSize(int boxSize)
	{
		m_estimateBoxSizes = boxSize;

		m_estimateBoxs = estimateGlobalBoxBeenLayout(getPrinter(0)->box(), m_estimateBoxSizes);
	}

	//====  another multiBin layout strategy ====
}