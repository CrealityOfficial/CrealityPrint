#include "msbase/utils/drill.h"

#include "cylindercollide.h"
#include <assert.h>
#include <fstream>
#include <memory>

namespace msbase
{
	trimesh::TriMesh* drill(trimesh::TriMesh* mesh, trimesh::TriMesh* cylinderMesh,
		ccglobal::Tracer* tracer, DrillDebugger* debugger)
	{
		if (tracer)
		{
			tracer->progress(0.1f);
		}
		if (!mesh || !cylinderMesh)
		{
			if (tracer)
				tracer->failed("mesh or cylinder mesh is empty.");
			
			return nullptr;
		}
		int vertexNum = mesh->vertices.size();
		int faceNum = mesh->faces.size();

		SAFE_TRACER(tracer, "drill start.  vertexNum [%d] , faceNum [%d].", vertexNum, faceNum);
		if (vertexNum == 0 || faceNum == 0 || cylinderMesh->vertices.size() == 0
			|| cylinderMesh->faces.size() == 0)
		{
			if (tracer)
				tracer->failed("mesh or cylinder (vertex, face) is empty.");
			return nullptr;
		}

		OptimizeCylinderCollide cylinderCollider(mesh, cylinderMesh, tracer, debugger);
		if (!cylinderCollider.valid())
		{
			if (tracer)
				tracer->failed("OptimizeCylinderCollide is not valid.");

			return nullptr;
		}
		if (tracer)
		{
			tracer->progress(0.2f);
		}
		return cylinderCollider.drill(tracer);
	}

	trimesh::TriMesh* drillCylinder(trimesh::TriMesh* mesh, const DrillParam& param, ccglobal::Tracer* tracer, DrillDebugger* debugger, bool useNewDrill)
	{
		if (!mesh)
		{
			if (tracer)
				tracer->failed("model mesh is empty.");

			return nullptr;
		}

		int vertexNum = mesh->vertices.size();
		int faceNum = mesh->faces.size();

		SAFE_TRACER(tracer, "drill start.  vertexNum [%d] , faceNum [%d].", vertexNum, faceNum);
		if (vertexNum == 0 || faceNum == 0)
		{
			if (tracer)
				tracer->failed("model mesh (vertex, face) is empty.");
			return nullptr;
		}

		OptimizeCylinderCollide cylinderCollider(mesh, param.cylinder_resolution, param.cylinder_radius, param.cylinder_depth, param.cylinder_startPos, param.cylinder_Dir, tracer, debugger);

		if (!cylinderCollider.valid())
		{
			if (tracer)
				tracer->failed("Depth Error or OptimizeCylinderCollide is not valid.");

			return nullptr;
		}

		if (useNewDrill)//new drill
		{
			return cylinderCollider.drilldrill(tracer);
		}

		return cylinderCollider.drill(tracer);

	}
	trimesh::TriMesh* drillCylinder(trimesh::TriMesh* mesh, std::vector<DrillParam>& params, ccglobal::Tracer* tracer, DrillDebugger* debugger, bool useNewDrill)
	{
		if (!mesh)
		{
			if (tracer)
				tracer->failed("model mesh is empty.");
			return nullptr;
		}
		if (params.size() == 0)
			return nullptr;

		auto f = [&useNewDrill](trimesh::TriMesh* mesh, const DrillParam& param)->trimesh::TriMesh * {
			if (!mesh)
				return nullptr;

			trimesh::TriMesh* out = drillCylinder(mesh, param, nullptr, nullptr, useNewDrill);
			return out;
		};

		trimesh::TriMesh* input = mesh;
		std::unique_ptr<trimesh::TriMesh> out;
		std::vector<DrillParam> _params = params;
		while (_params.size() > 0)
		{
			DrillParam param = _params.back();
			out.reset(f(input, param));
			if (!out)
				break;

			input = out.get();
			_params.pop_back();
		}

		return out.release();
	}
}