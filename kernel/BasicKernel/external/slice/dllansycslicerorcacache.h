#ifndef _NULLSPACE_DLLANSYCSLICERORCA_CACHE_1591781523824_H
#define _NULLSPACE_DLLANSYCSLICERORCA_CACHE_1591781523824_H
#include "slice/ansycslicer.h"
#include "internal/multi_printer/printer.h"

namespace creative_kernel
{
	class OrcaCacheSlice : public QObject
	{
	public:
		OrcaCacheSlice(QObject* parent = nullptr);
		virtual ~OrcaCacheSlice();

		void cacheSlice(Printer* printer, SettingsPointer G, const QList<SettingsPointer>& Es,
			const std::vector<crslice2::ThumbnailData>& thumbnails, const crslice2::PlateInfo& plate_infos,
			qtuser_core::ProgressorTracer* tracer, SliceAttain* attain = nullptr);
	};
}
#endif // _NULLSPACE_DLLANSYCSLICERORCA_CACHE_1591781523824_H
