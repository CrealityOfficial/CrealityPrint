#ifndef CXKERNEL_MODELNDATA_1681019989200_H
#define CXKERNEL_MODELNDATA_1681019989200_H
#include "cxkernel/cxkernelinterface.h"
#include "cxkernel/data/attribute.h"
#include "cxkernel/data/header.h"

namespace common_3mf
{
	struct Scene3MF;
}
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

		int defaultColor { 0 };
		std::vector<std::string> colors; //for paint color
		std::vector<std::string> seams; //for paint seam
		std::vector<std::string> supports; //for paint support
		ModelNDataType type = ModelNDataType::mdt_none;

		trimesh::xform mxform;  // used when loading 3mf
		int objectId3mf{ 0 };  // used when loading 3mf
		trimesh::xform componentxform;  // 3mf object component transform
	};

	class ModelNData;
	typedef std::shared_ptr<ModelNData> ModelNDataPtr;
	class CXKERNEL_API ModelNData
	{
	public:
		ModelNData();
		~ModelNData();

		ModelNDataPtr clone();

		int primitiveNum();
		trimesh::box3 calculateBox(const trimesh::fxform& matrix = trimesh::fxform::identity());
		trimesh::box3 localBox();
		float localZ();

		void calculateFaces();
		void resetHull();

		void adaptSmallBox(const trimesh::box3& box);
		void adaptBigBox(const trimesh::box3& box);

		void convex(const trimesh::fxform& matrix, std::vector<trimesh::vec3>& datas);
		bool traitTriangle(int faceID, std::vector<trimesh::vec3>& position, const trimesh::fxform& matrix, bool offset = false);
		TriMeshPtr createGlobalMesh(const trimesh::fxform& matrix);
		bool traitTriangleEx(int faceID, std::vector<trimesh::vec3>& position, trimesh::vec3& normal, const trimesh::fxform& matrix, float offsetValue, bool offset = false);

		TriMeshPtr mesh;
		TriMeshPtr hull;

		std::vector<KernelHullFace> faces;
		QMap<int, trimesh::vec3> colorMap;
		QSet<int> colorIndexs;
		std::vector<std::string> colors; //for paint color
		int defaultColor;
		std::vector<int> spreadFaces; //for paint color spread face
		int spreadFaceCount;
		std::vector<std::string> seams; //for paint seam
		std::vector<std::string> supports; //for paint support
		trimesh::vec3 offset;
		ModelCreateInput input;
	};

	class ModelNDataProcessor
	{
	public:
		virtual ~ModelNDataProcessor() {}
		virtual void process(ModelNDataPtr data) = 0;
#ifndef CXKERNEL_DISABLE_3MF
		virtual void process(common_3mf::Scene3MF data) = 0;
#endif
		virtual void modelMeshLoadStarted(int iMeshNum) {};
		virtual void onMeshLoadFail() {};
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