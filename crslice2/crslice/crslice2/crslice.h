#ifndef CRSLICE_SLICE_H_2
#define CRSLICE_SLICE_H_2
#include "crslice2/interface.h"
#include "crslice2/crscene.h"
#include <vector>

namespace crslice2
{
    struct SliceResult
    {
        unsigned long int print_time; // Ԥ����ӡ��ʱ����λ����
        double filament_len; // Ԥ���������ģ���λ����
        double filament_volume; // Ԥ��������������λ��g
        unsigned long int layer_count;  // ��Ƭ����
        double x;   // ��Ƭx�ߴ�
        double y;   // ��Ƭy�ߴ�
        double z;   // ��Ƭz�ߴ�
    };

	class CRSLICE2_API CrSlice
	{
	public:
		CrSlice();
		~CrSlice();

		void sliceFromScene(CrScenePtr scene, ccglobal::Tracer* tracer = nullptr);

        SliceResult sliceResult;

        std::map<std::string, std::pair<std::string,int64_t>> extraSliceWarningDetails;

	};

    CRSLICE2_API std::vector<double> getLayerHeightProfileAdaptive(crslice2::SettingsPtr settings, std::vector<TriMeshPtr> triMesh, float quality);
    CRSLICE2_API std::vector<double> smooth_height_profile(crslice2::SettingsPtr settings, std::vector<TriMeshPtr> triMesh,
        const std::vector<double>& profile, unsigned int radius, bool keep_min);
    CRSLICE2_API std::vector<double> generateObjectLayers(crslice2::SettingsPtr settings, std::vector<TriMeshPtr> triMesh,
        const std::vector<double>& profile);
    CRSLICE2_API std::vector<double> updateObjectLayers(crslice2::SettingsPtr settings, std::vector<TriMeshPtr> triMesh,
        const std::vector<double>& profile);
    CRSLICE2_API void orcaSliceFromFile(const std::string& file, const std::string& out, ccglobal::Tracer* tracer = nullptr);
}
#endif  // MSIMPLIFY_SIMPLIFY_H
