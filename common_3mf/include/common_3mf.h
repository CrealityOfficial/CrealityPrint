#ifndef _COMMON_3MF_H
#define _COMMON_3MF_H
#include "ccglobal/tracer.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include <string>
#include <memory>
#include <map>

namespace common_3mf
{
	typedef std::shared_ptr<trimesh::TriMesh> TriMeshPtr;
	
	enum PlateMode
	{
		Undef,
		SingleExtruder,
		MultiAsSingle,
		MultiExtruder
	};

	enum PlateType
	{
		ColorChange,
		PausePrint,
		ToolChange,
		Template,
		Custom,
		Unknown,
	};

	struct PlateCustomGCode
	{
		double      print_z;
		PlateType        type;
		int         extruder;
		std::string color;
		std::string extra;
	};

	struct Plate3MF
	{
		PlateMode mode = Undef;
		std::vector<PlateCustomGCode> gcodes;
		int       plateId;
		bool locked;
		std::string name;
		std::map<std::string, std::string> settings;
	};

	struct Model3MF
	{
		TriMeshPtr mesh;
		trimesh::xform xf;
		std::vector<std::string> colors;
		std::vector<std::string> seams;
		std::vector<std::string> supports;
		int id;
		std::string name;
		std::string subtype;
		int extruder;
		std::map<std::string, std::string> settings;
	};

	struct Group3MF
	{
		trimesh::xform xf;
		int id;
		std::string name;
		int extruder;
		std::vector<Model3MF> models;
		std::map<std::string, std::string> settings;
	};

	struct Scene3MF
	{
		int group_counts = 0;
		int model_counts = 0;
		std::vector<Group3MF> groups;
		std::vector<Plate3MF> plates;

		std::string application;

		std::string project_settings;
	};

	class Read3MFImpl;
	class Read3MF
	{
	public:
		Read3MF(const std::string& file_name);
		~Read3MF();

		bool read_all_3mf(Scene3MF& scene, ccglobal::Tracer* tracer = nullptr);
		bool extract_file(const std::string& zip_name, const std::string& dest_file_name);
	protected:
		Read3MFImpl* impl = nullptr;
	};

	class Write3MFImpl;
	class Write3MF
	{
	public:
		Write3MF(const std::string& file_name);
		~Write3MF();

		bool write_scene_2_3mf(const Scene3MF& scene, ccglobal::Tracer* tracer = nullptr);

	protected:
		Write3MFImpl* impl = nullptr;
	};

	void print_3mf_scene(const Scene3MF& scene, ccglobal::Tracer* tracer = nullptr);
}

#endif // _COMMON_3MF_H