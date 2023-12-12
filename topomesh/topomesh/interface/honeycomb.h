#ifndef TOPOMESH_HONEYCOMB_1692753685514_H
#define TOPOMESH_HONEYCOMB_1692753685514_H
#include "topomesh/interface/idata.h"

namespace topomesh
{
    enum class ErrorCode :int {
        GENERATE_NORMAL = 0,
        NO_MODEL_DATA = 1,
        BOUNDARY_CHECK_FAIL = 2,
        NO_INTERSECT_HEXAGON = 3,
        MAX_ERROR = 4,
    };

	struct CombParam
	{
		float width = 2.0f;
		float diameter = 6.0f;
		float combShell = 2.0f;

		bool holeConnect = true;
		float holeHeight = 1.0f;
		float holeDiameter = 2.5f;
		float holeGap = 1.0f;
		std::vector<std::vector<int>> faces;
        int mode = 0; ///< 0 is shell, 1 is backfill.
	};

    TOPOMESH_API std::shared_ptr<trimesh::TriMesh> honeyCombGenerate(trimesh::TriMesh* trimesh, ErrorCode& code, const CombParam& honeyparams = CombParam(),
        ccglobal::Tracer* tracer = nullptr);

	/// <summary>
	/// 0: parameters qualified.  
	/// 1: combShell is not qualified.
    /// 2: diameter is not qualified.
    /// 3: width is not qualified.
    /// 4: holeDiameter is not qualified.
    /// 5: holeGap is not qualified.
    /// 6: holeHeight is not qualified.
    /// 7: mode is not qualified.
    /// 8: faces of backfill is not qualified.
	/// </summary>
	/// <param name="param"></param>
	/// <returns></returns>
	TOPOMESH_API int checkParam(const CombParam& param);

	TOPOMESH_API  void SelectBorderFacesOfInterface(trimesh::TriMesh* mesh, int indicate, std::vector<int>& out);
	TOPOMESH_API  void LastFacesOfInterface(trimesh::TriMesh* mesh, const std::vector<int>& in, std::vector<std::vector<int>>& out, int threshold, std::vector<int>& other_shells);
}

#endif // TOPOMESH_HONEYCOMB_1692753685514_H