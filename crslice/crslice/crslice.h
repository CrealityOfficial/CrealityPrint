#ifndef CRSLICE_SLICE_H
#define CRSLICE_SLICE_H
#include "crslice/interface.h"
#include "crslice/crscene.h"

namespace crslice
{
    struct SliceResult
    {
        unsigned long int print_time; // 预估打印耗时，单位：秒
        double filament_len; // 预估材料消耗，单位：米
        double filament_volume; // 预估材料重量，单位：g
        unsigned long int layer_count;  // 切片层数
        double x;   // 切片x尺寸
        double y;   // 切片y尺寸
        double z;   // 切片z尺寸
    };

	class CRSLICE_API CrSlice
	{
	public:
		CrSlice();
		~CrSlice();

		void sliceFromScene(CrScenePtr scene, ccglobal::Tracer* tracer = nullptr);

        SliceResult sliceResult;
	};
}
#endif  // MSIMPLIFY_SIMPLIFY_H
