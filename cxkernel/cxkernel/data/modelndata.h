#ifndef CXKERNEL_MODELNDATA_1681019989200_H
#define CXKERNEL_MODELNDATA_1681019989200_H
#include "cxkernel/cxkernelinterface.h"
#include "cxkernel/data/attribute.h"
#include "cxkernel/data/header.h"

namespace cxkernel
{
	struct KernelHullFace {
		TriMeshPtr mesh = std::make_shared<trimesh::TriMesh>();
		trimesh::vec3 normal;
		float hullarea = 0.0f;
	};

	enum class ModelNDataType
	{
		mdt_none,
		mdt_file,
		mdt_cxbin,
		mdt_algrithm,
		mdt_project
	};

	struct ModelCreateInput
	{
		TriMeshPtr mesh;

		QString fileName;    // only for load
		QString name;
		QString description;
		std::vector<std::string> colors; //for paint color
		std::vector<std::string> seams; //for paint seam
		std::vector<std::string> supports; //for paint support
		ModelNDataType type = ModelNDataType::mdt_none;
	};

	class CXKERNEL_API ModelNData
	{
	public:
		ModelNData();
		~ModelNData();

		int primitiveNum();
		void updateRenderData();
		void updateRenderDataForced();
		void updateIndexRenderData();
		trimesh::box3 calculateBox(const trimesh::fxform& matrix = trimesh::fxform::identity());
		trimesh::box3 localBox();
		float localZ();

		void calculateFaces();
		void resetHull();

		void convex(const trimesh::fxform& matrix, std::vector<trimesh::vec3>& datas);
		bool traitTriangle(int faceID, std::vector<trimesh::vec3>& position, const trimesh::fxform& matrix, bool offset = false);
		TriMeshPtr createGlobalMesh(const trimesh::fxform& matrix);
		bool traitTriangleEx(int faceID, std::vector<trimesh::vec3>& position, trimesh::vec3& normal, const trimesh::fxform& matrix, float offsetValue, bool offset = false);

		TriMeshPtr mesh;
		TriMeshPtr hull;
		cxkernel::GeometryData renderData;
		std::vector<KernelHullFace> faces;
		std::vector<std::string> colors; //for paint color
		std::vector<std::string> seams; //for paint seam
		std::vector<std::string> supports; //for paint support
		trimesh::vec3 offset;
		ModelCreateInput input;
	};

	typedef std::shared_ptr<ModelNData> ModelNDataPtr;

	class ModelNDataProcessor
	{
	public:
		virtual ~ModelNDataProcessor() {}
		virtual void process(ModelNDataPtr data) = 0;
	};

	struct ModelNDataCreateParam
	{
		bool dumplicate = true;
		bool toCenter = true;
		bool indexRender = true;
	};
	
	CXKERNEL_API ModelNDataPtr createModelNData(ModelCreateInput input,
													   ccglobal::Tracer* tracer = nullptr,
													   const ModelNDataCreateParam& param = ModelNDataCreateParam()
													   );

	CXKERNEL_API ModelNDataPtr createModelNData(TriMeshPtr mesh, const QString& name,
		ModelNDataType type, bool toCenter = false, ccglobal::Tracer* tracer = nullptr);
}

#endif // CXKERNEL_MODELNDATA_1681019989200_H