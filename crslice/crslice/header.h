#ifndef CRSLICE_HEADER_INTERFACE
#define CRSLICE_HEADER_INTERFACE
#include "crslice/interface.h"
#include <memory>

#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"
#include "trimesh2/TriMesh_algo.h"
#include "crslice/gcode/header.h"

#include <fstream>
#include <unordered_map>
#include "ccglobal/tracer.h"

typedef std::shared_ptr<trimesh::TriMesh> TriMeshPtr;

#include "crslice/settings.h"
typedef std::shared_ptr<crslice::Settings> SettingsPtr;

namespace crslice
{
	class PathData :public gcode::GcodeTracer
	{
	public:
		void tick(const std::string& tag) override {};
		void getPathData(const trimesh::vec3 point, float e, int type) override {};
		void getPathDataG2G3(const trimesh::vec3 point, float i, float j, float e, int type, bool isG2 = true) override {};
		void setParam(gcode::GCodeParseInfo& pathParam)override {};
		void setLayer(int layer) override {};
		void setLayers(int layer) override {};
		void setSpeed(float s) override {};
		void setTEMP(float temp) override {};
		void setExtruder(int nr) override {};
		void setTime(float time) override {};
		void setFan(float fan) override {};
		void setZ(float z, float h = -1) override {};
		void setE(float e) override {};
		void getNotPath() override {};
		void set_data_gcodelayer(int layer, const std::string& gcodelayer) override {};

		//for cloud : preview image
		void onSupports(int layerIdx, float z, float thickness, const std::vector<std::vector<trimesh::vec3>>& paths) override {};
		void setSceneBox(const trimesh::box3& box) override {};
	};
}
#endif // CRSLICE_HEADER_INTERFACE