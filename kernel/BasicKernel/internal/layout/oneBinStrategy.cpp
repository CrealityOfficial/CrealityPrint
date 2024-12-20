#include "oneBinStrategy.h"
#include "interface/printerinterface.h"
#include "qtuser3d/trimesh2/conv.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{

	OneBinStrategy::OneBinStrategy(int binIndex)
		: nestplacer::BinExtendStrategy()
		, m_binIndex(binIndex)
	{
		m_box = getPrinter(binIndex)->globalBox();
	}

	OneBinStrategy::~OneBinStrategy()
	{
	}

	// this func returns the box of the index layout printer, 
	// "index" is likely bigger than total printers count of current scene, when needing to generate a new bin
	trimesh::box3 OneBinStrategy::bounding(int index) const
	{
		return qtuser_3d::qBox32box3(m_box);
	}

	// this func returns the box in which the fixItem resides 
	trimesh::box3 OneBinStrategy::fixBounding(int fixBinId) const
	{
		return qtuser_3d::qBox32box3(m_box);
	}

}