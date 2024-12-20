#ifndef CREATIVE_KERNEL_RAWDATA_1681019989200_H
#define CREATIVE_KERNEL_RAWDATA_1681019989200_H
#include "cxkernel/data/meshdata.h"
#include "data/shareddatamanager/shareddataid.h"
#include "data/kernelenum.h"
#include "us/usettings.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QMatrix4x4>

class FontMeshWrapper;

namespace creative_kernel
{
	struct MeshCreateInput
	{
		TriMeshPtr mesh;

		QString fileName;    // only for load
		QString name;
		QString description;

		int defaultColor{ 0 };
		std::vector<std::string> colors; //for paint color
		std::vector<std::string> seams; //for paint seam
		std::vector<std::string> supports; //for paint support
	
		ModelNType type{ ModelNType::normal_part };
		int64_t group{ -1 };
	};

	BASICKERNEL_API void makeSureDataValid(TriMeshPtr mesh,
		std::vector<std::string>& colors,
		std::vector<std::string>& seams,
		std::vector<std::string>& supports);

	struct ModelData {
		cxkernel::MeshDataPtr mesh;
		std::vector<std::string> colors;
		std::vector<std::string> supports;
		std::vector<std::string> seams;
		int defaultColor = -1;

		QString name;
	};
	typedef std::shared_ptr<ModelData> ModelDataPtr;

	BASICKERNEL_API ModelDataPtr createModelData(TriMeshPtr mesh,
		const std::vector<std::string>& colors,
		const std::vector<std::string>& seams,
		const std::vector<std::string>& supports,
		int color_index,
		bool toCenter = true);

	BASICKERNEL_API cxkernel::MeshDataPtr createMeshDataPtr(TriMeshPtr mesh, bool toCenter = true);

	struct ModelCreateData {
		SharedDataID dataID;
		QString name;
		int64_t fixed_id { -1 };
		int64_t model_id = -1;
		int model_part_index = -1;
		trimesh::xform xf = trimesh::xform(0.0);
		us::USettings* usettings = nullptr;
		std::vector<double> layerHeightProfile;
		std::string reliefDescription;

	};
	struct GroupCreateData {
		QList<ModelCreateData> models;

		//group
		QString name;
		trimesh::xform xf = trimesh::xform(0.0);
		int64_t model_group_id = -1;
		us::USettings* usettings = nullptr;
		int defaultColorIndex {-1};
		std::vector<trimesh::vec3> outlinePath;
		trimesh::dvec3   outlinePathInitPos;
		std::vector<double> layerHeightProfile;

		void reset() { model_group_id = -1; models.clear(); name.clear(); outlinePath.clear(); }
	};

	struct SceneCreateData {
		QList<GroupCreateData> add_groups;
		QList<GroupCreateData> remove_groups;
	};
}

#endif // CREATIVE_KERNEL_DATASERIAL_1681019989200_H