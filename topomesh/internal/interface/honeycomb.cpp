#include "topomesh/interface/honeycomb.h"
#include "internal/alg/fillhoneycombs.h"

namespace topomesh
{
	std::shared_ptr<trimesh::TriMesh> honeyCombGenerate(trimesh::TriMesh* trimesh, ErrorCode& code, const CombParam& honeyparams,
		ccglobal::Tracer* tracer)
	{
        HoneyCombParam params;
        params.honeyCombRadius = honeyparams.diameter / 2.0f;
        params.nestWidth = honeyparams.width;
        params.shellThickness = honeyparams.combShell;
        params.cheight = honeyparams.holeHeight;
        params.delta = honeyparams.holeGap;
        params.ratio = honeyparams.holeDiameter / (honeyparams.diameter / 2.0f);
        params.holeConnect = honeyparams.holeConnect;
        params.mode = honeyparams.mode;
        params.faces = honeyparams.faces;
        int value = 0;
        std::shared_ptr<trimesh::TriMesh> mesh = GenerateHoneyCombs(trimesh, value, params, tracer);
        code = ErrorCode(value);
		return mesh;
	}

    int checkParam(const CombParam& param)
    {
        if (param.combShell < 0.5f || param.combShell > 10.f) {
            return 1;
        }else if (param.diameter < 1.0f || param.diameter > 10.0f) {
            return 2;
        } else if (param.width < 1.0f || param.width > param.diameter) {
            return 3;
        } else if (param.holeDiameter < 1.0f || param.holeDiameter > param.diameter / 2.0f - 0.5f) {
            return 4;
        } else if (param.holeGap < 1.0f || param.holeGap > 10.0f) {
            return 5;
        } else if (param.holeHeight < 1.0f || param.holeHeight > 3.0f) {
            return 6;
        }
        if (param.mode == 0 || param.mode == 1) {
            if (param.mode) {
                if (param.faces.size() < 2) {
                    return 8;
                } else if (param.faces[0].empty() || param.faces[1].empty()) {
                    return 8;
                }
            }
        } else {
            return 7;
        }
        return 0;
    }


    void SelectBorderFacesOfInterface(trimesh::TriMesh* mesh, int indicate, std::vector<int>& out)
    {
        topomesh::SelectBorderFaces(mesh,indicate,out);
    }

    void LastFacesOfInterface(trimesh::TriMesh* mesh, const std::vector<int>& in, std::vector<std::vector<int>>& out, int threshold, std::vector<int>& other_shells)
    {
        topomesh::LastFaces(mesh, in, out, threshold, other_shells);
    }
}