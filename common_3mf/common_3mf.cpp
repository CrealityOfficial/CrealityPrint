#include "common_3mf.h"
#include "readwrite.h"
#include "ccglobal/log.h"

namespace common_3mf
{
	Read3MF::Read3MF(const std::string& file_name)
		:impl(new Read3MFImpl(file_name))
	{

	}

	Read3MF::~Read3MF()
	{
		delete impl;
	}

	bool Read3MF::read_all_3mf(Scene3MF& scene, ccglobal::Tracer* tracer)
	{
		return impl->read_all_3mf(scene, tracer);
	}

	bool Read3MF::extract_file(const std::string& zip_name, const std::string& dest_file_name)
	{
		return impl->extract_file(zip_name, dest_file_name);
	}

	Write3MF::Write3MF(const std::string& file_name)
		:impl(new Write3MFImpl(file_name))
	{

	}

	Write3MF::~Write3MF()
	{
		delete impl;
	}

	bool Write3MF::write_scene_2_3mf(const Scene3MF& scene, ccglobal::Tracer* tracer)
	{
		return impl->write_scene_2_3mf(scene, tracer);
	}

	void print_3mf_scene(const Scene3MF& scene, ccglobal::Tracer* tracer)
	{
		LOGI("3mf contains [%d] plate", (int)scene.plates.size());
		LOGI("3mf scene content, [%d] groups, [%d] models", scene.group_counts, scene.model_counts);
		for (const Group3MF& group : scene.groups)
		{
			LOGI("group [%s] [%d]", group.name.c_str(), group.id);
			for (const Model3MF& model : group.models)
			{
				LOGI("model [%s] [%d]", model.name.c_str(), model.id);
			}
		}
	}
}