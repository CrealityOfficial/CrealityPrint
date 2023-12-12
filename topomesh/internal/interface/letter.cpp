#include "topomesh/interface/letter.h"
#include "msbase/utils/trimeshserial.h"
#include "internal/alg/letter.h"
#include "ccglobal/profile.h"

namespace topomesh
{
	class LetterInput : public ccglobal::Serializeable
	{
	public:
		trimesh::TriMesh mesh;
		LetterParam param;
		SimpleCamera camera;
		std::vector<TriPolygons> polys;

		LetterInput() {

		}
		~LetterInput() {

		}

		int version() override {
			return 0;
		}
		bool save(std::fstream& out, ccglobal::Tracer* tracer) override {
			ccglobal::cxndSaveT(out, param.concave);
			ccglobal::cxndSaveT(out, param.deep);
			ccglobal::cxndSaveT(out, camera);
			msbase::saveTrimesh(out, mesh);
			
			int size = (int)polys.size();
			ccglobal::cxndSaveT(out, size);
			for (int i = 0; i < size; ++i)
				msbase::savePolys(out, polys.at(i));

			return true;
		}

		bool load(std::fstream& in, int ver, ccglobal::Tracer* tracer) override {
			if (ver == 0)
			{
				ccglobal::cxndLoadT(in, param.concave);
				ccglobal::cxndLoadT(in, param.deep);
				ccglobal::cxndLoadT(in, camera);
				msbase::loadTrimesh(in, mesh);
				
				int size = 0;
				ccglobal::cxndLoadT(in, size);
				if (size > 0)
				{
					polys.resize(size);
					for (int i = 0; i < size; ++i)
						msbase::loadPolys(in, polys.at(i));
				}
				return true;
			}
			return false;
		}
	};

	trimesh::TriMesh* letterFromFile(const std::string& fileName, LetterDebugger* debugger, ccglobal::Tracer* tracer)
	{
		LetterInput input;
		if (ccglobal::cxndLoad(input, fileName, tracer))
		{
			LOGE("letterFromFile load error [%s]", fileName.c_str());
			return nullptr;
		}

		return topomesh::letter(&input.mesh, input.camera, input.param, input.polys, debugger, tracer);
	}

	trimesh::TriMesh* letter(trimesh::TriMesh* mesh, const SimpleCamera& camera,
		const LetterParam& param, const std::vector<TriPolygons>& polygons,
		LetterDebugger* debugger, ccglobal::Tracer* tracer) 
	{
		if (param.cacheInput && mesh)
		{
			LetterInput input;
			input.mesh = *mesh;
			input.camera = camera;
			input.param = param;
			input.polys = polygons;

			ccglobal::cxndSave(input, param.fileName);
		}

		bool letterOpState = true;
		SYSTEM_TICK("letter");
		trimesh::TriMesh* result = letter(mesh, camera, param, polygons, letterOpState, debugger, tracer);
		SYSTEM_TICK("letter");
		if (!letterOpState && result)
		{
			delete result;
			result = nullptr;
		}

		return result;
	}
}