#include "crslice2/crslice.h"
#include "wrapper/orcaslicewrapper.h"

#include "ccglobal/log.h"
#include "libslic3r/GCode/GCodeProcessor.hpp"
#include "libslic3r/I18N.hpp"

namespace crslice2
{
	CrSlice::CrSlice()
		:sliceResult({0})
	{

	}

	CrSlice::~CrSlice()
	{

	}

	void CrSlice::sliceFromScene(CrScenePtr scene, ccglobal::Tracer* tracer)
	{
		if (!scene)
		{
			LOGM("CrSlice::sliceFromSceneOrca empty scene.");
			return;
		}

		Slic3r::GCodeProcessorResult gcodeProcessResult;
		gcodeProcessResult.reset();

		orca_slice_impl(scene, tracer, &gcodeProcessResult);

		// check slice warning details
		if (gcodeProcessResult.conflict_result.has_value())
		{
			static std::string text;

			std::string objName1 = gcodeProcessResult.conflict_result.value()._objName1;
			std::string objName2 = gcodeProcessResult.conflict_result.value()._objName2;
			double      height = gcodeProcessResult.conflict_result.value()._height;
			int  layer = gcodeProcessResult.conflict_result.value().layer + 1;  // "+1" to align with zslider display layer value
			size_t sliceObj2Id = gcodeProcessResult.conflict_result.value()._sliceObject2Id;

			text = (boost::format(_u8L("Conflicts of gcode paths have been found at layer# %d, #height$ %.2f mm.$ Please separate the conflicted objects further@%s")) % layer % height % objName1).str();

			int64_t sceneObjId = scene->getSceneObjectIdBySliceObjId(sliceObj2Id);
			extraSliceWarningDetails["Path_Conflict"] = std::make_pair(text, sceneObjId);
		}

		if (tracer && tracer->extraMessageSize() > 0)
		{
			std::map< std::string, std::pair<std::string, size_t> > warningInfo = tracer->getExtraRecordMessage();
			auto itr = warningInfo.begin();
			for (; itr != warningInfo.end(); itr++)
			{
				std::pair pairVal = itr->second;
				size_t sliceObjId = pairVal.second;
				int64_t sceneObjId = scene->getSceneObjectIdBySliceObjId(sliceObjId);

				extraSliceWarningDetails[itr->first] = std::make_pair(itr->second.first, sceneObjId);
			}
		}
	}

	CRSLICE2_API std::vector<double> getLayerHeightProfileAdaptive(crslice2::SettingsPtr settings, std::vector<TriMeshPtr> triMesh, float quality)
	{
		return orca_layer_height_profile_adaptive(settings, triMesh, quality);
	}

	CRSLICE2_API std::vector<double> smooth_height_profile(crslice2::SettingsPtr settings, std::vector<TriMeshPtr> triMesh,
		const std::vector<double>& layer, unsigned int radius, bool keep_min)
	{
		return orca_smooth_height_profile(settings, triMesh, layer, radius, keep_min);
	}

	CRSLICE2_API std::vector<double> generateObjectLayers(crslice2::SettingsPtr settings, std::vector<TriMeshPtr> triMesh,
		const std::vector<double>& profile)
	{
		return orca_generate_object_layers(settings, triMesh, profile);
	}

	CRSLICE2_API std::vector<double> updateObjectLayers(crslice2::SettingsPtr settings, std::vector<TriMeshPtr> triMesh, const std::vector<double>& profile)
	{
		return orca_update_layer_height_profile(settings, triMesh, profile);
	}

	void orcaSliceFromFile(const std::string& file, const std::string& out, ccglobal::Tracer* tracer)
	{
		orca_slice_fromfile_impl(file, out, tracer);
	}
}