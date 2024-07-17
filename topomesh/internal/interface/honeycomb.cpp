#include "topomesh/interface/honeycomb.h"
#include "internal/alg/fillhoneycombs.h"
#include "msbase/utils/trimeshserial.h"
#include "ccglobal/profile.h"
namespace topomesh
{
    class HoneyCombInput : public ccglobal::Serializeable {
    public:
        trimesh::TriMesh mesh;
        CombParam param;
        int code = 0;

        HoneyCombInput()
        {

        }
        ~HoneyCombInput()
        {

        }

        int version() override
        {
            return 0;
        }
        bool save(std::fstream& out, ccglobal::Tracer* tracer) override
        {
            ccglobal::cxndSaveT(out, param.width);
            ccglobal::cxndSaveT(out, param.diameter);
            ccglobal::cxndSaveT(out, param.combShell);

            ccglobal::cxndSaveT(out, param.holeConnect);
            ccglobal::cxndSaveT(out, param.holeHeight);
            ccglobal::cxndSaveT(out, param.holeDiameter);
            ccglobal::cxndSaveT(out, param.holeGap);
            ccglobal::cxndSaveT(out, param.mode);

            ccglobal::cxndSaveT(out, code);
            msbase::saveTrimesh(out, mesh);
            msbase::saveFacePatchs(out, param.faces);
            return true;
        }

        bool load(std::fstream & in, int ver, ccglobal::Tracer * tracer) override
        {
            if (ver == 0) {
                ccglobal::cxndLoadT(in, param.width);
                ccglobal::cxndLoadT(in, param.diameter);
                ccglobal::cxndLoadT(in, param.combShell);

                ccglobal::cxndLoadT(in, param.holeConnect);
                ccglobal::cxndLoadT(in, param.holeHeight);
                ccglobal::cxndLoadT(in, param.holeDiameter);
                ccglobal::cxndLoadT(in, param.holeGap);
                ccglobal::cxndLoadT(in, param.mode);

                ccglobal::cxndLoadT(in, code);
                msbase::loadTrimesh(in, mesh);
                msbase::loadFacePatchs(in, param.faces);
                return true;
            }
            return false;
        }
    };

    std::shared_ptr<trimesh::TriMesh> honeyGenerateFromFile(const std::string & fileName, ccglobal::Tracer * tracer)
    {
        HoneyCombInput input;
        if (!ccglobal::cxndLoad(input, fileName, tracer)) {
            LOGE("honeyCombFromFile load error [%s]", fileName.c_str());
            return nullptr;
        }
        ErrorCode value = (ErrorCode)input.code;
        return topomesh::honeyCombGenerate(&input.mesh, value, input.param, tracer);
    }

	std::shared_ptr<trimesh::TriMesh> honeyCombGenerate(trimesh::TriMesh* trimesh, ErrorCode& code, const CombParam& honeyparams,
		ccglobal::Tracer* tracer)
	{
        if (!honeyparams.fileName.empty() && trimesh) {
            HoneyCombInput input;
            input.mesh = *trimesh;
            input.param = honeyparams;
            input.code = (int)code;

            ccglobal::cxndSave(input, honeyparams.fileName);
        }

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
        SYSTEM_TICK("honeyComb");
        std::shared_ptr<trimesh::TriMesh> mesh = GenerateHoneyCombs(trimesh, value, params, tracer);
        SYSTEM_TICK("honeyComb");
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