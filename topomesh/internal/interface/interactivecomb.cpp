#include "topomesh/interface/interactivecomb.h"
#include "internal/alg/fillhoneycombs.h"

#include "msbase/mesh/get.h"
#include "msbase/mesh/checker.h"
#include "msbase/data/conv.h"
#include "trimesh2/TriMesh_algo.h"

#include <assert.h>

namespace topomesh
{
	struct InteractiveCombImpl
	{
		InteractiveCombImpl(TopoTriMeshPtr mesh)
			:debugger(nullptr)
		{
			raw = mesh;
			raw->need_across_edge();

			msbase::calculateFaceNormalOrAreas(raw.get(), normals, &areas);

			modXf = trimesh::fxform::identity();
		}

		trimesh::vec3 findBottomDirection(std::vector<int>& faces, float threadHold = 0.95f)
		{
			msbase::checkLargetPlanar(raw.get(), normals, areas, threadHold, faces);

			trimesh::vec3 normal(0.0f, 0.0f, 0.0f);
			for (const auto& f : faces) {
				normal += normals[f] * areas[f];
			}
			return trimesh::normalized(normal);
		}

		void modifyMesh(trimesh::TriMesh* mesh, const trimesh::vec3& dir, bool invert)
		{
			if (invert)
			{
				trimesh::xform xf = trimesh::inv(modXf);
				trimesh::apply_xform(mesh, xf);
			}
			else
			{
				trimesh::xform q = trimesh::xform::rot_into(dir, trimesh::vec3(0.0f, 0.0f, -1.0f));
				trimesh::apply_xform(mesh, q);
				mesh->clear_bbox();
				mesh->need_bbox();

				trimesh::xform t = trimesh::xform::trans(- mesh->bbox.min);
				trimesh::apply_xform(mesh, t);
				modXf = t * q;
			}
		}

		void flatBottomSurface(const std::vector<int>& bottomfaces)
		{
			std::vector<trimesh::TriMesh::Face> faces = raw->across_edge;
			const auto& minz = raw->bbox.min.z;
			for (const auto& f : bottomfaces) {
				const auto& neighbor = faces[f];
				for (int j = 0; j < 3; ++j) {
					if(neighbor[j] >= 0)
						raw->vertices[neighbor[j]].z = minz;
				}
			}
		}

		TopoTriMeshPtr shell(const HoneyCombParam& param, ccglobal::Tracer* tracer)
		{
            assert(param.axisDir == trimesh::vec3(0.0f, 0.0f, 0.0f));

            std::vector<int> bottomFaces;
            trimesh::vec3 dir = findBottomDirection(bottomFaces);

			modifyMesh(raw.get(), dir, false);

			//第2步，平移至xoy平面后底面整平
			flatBottomSurface(bottomFaces);

			//第3步，生成底面六角网格
			honeyLetterOpt letterOpts;
            letterOpts.bottom.resize(bottomFaces.size());
            letterOpts.bottom.assign(bottomFaces.begin(), bottomFaces.end());
            std::vector<int> honeyFaces;
            honeyFaces.reserve(raw->faces.size());
            for (int i = 0; i < raw->faces.size(); ++i) {
                honeyFaces.emplace_back(i);
            }
            std::sort(bottomFaces.begin(), bottomFaces.end());
            std::vector<int> otherFaces(honeyFaces.size() - bottomFaces.size());
            std::set_difference(honeyFaces.begin(), honeyFaces.end(), bottomFaces.begin(), bottomFaces.end(), otherFaces.begin());
            letterOpts.others = std::move(otherFaces);
   
            //GenerateBottomHexagons(cmesh, param, letterOpts);
            TopoTriMeshPtr newmesh = SetHoneyCombHeight(raw.get(), param, letterOpts);
			
            JointBotMesh(raw.get(), newmesh.get(), bottomFaces, param.mode);

			modifyMesh(newmesh.get(), trimesh::vec3(), true);
            return newmesh;
		}

		TopoTriMeshPtr backfill(const HoneyCombParam& param, ccglobal::Tracer* tracer)
		{
			return nullptr;
		}

		TopoTriMeshPtr raw;
		InteractiveCombDebugger* debugger;

		std::vector<trimesh::vec3> normals;
		std::vector<float> areas;

		//modify xf
		trimesh::xform modXf;
	};


	InteractiveComb::InteractiveComb(TopoTriMeshPtr mesh)
		:impl(new InteractiveCombImpl(mesh))
	{
		assert(mesh && mesh->faces.size() > 0);
	}

	InteractiveComb::~InteractiveComb()
	{
		delete impl;
	}

	void InteractiveComb::setDebugger(InteractiveCombDebugger* debugger)
	{
		impl->debugger = debugger;
	}

	TopoTriMeshPtr InteractiveComb::generate(const CombParam& honeyparams, ccglobal::Tracer* tracer)
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

		if (honeyparams.mode == 0) {
			return impl->shell(params, tracer);
		}

		return impl->backfill(params, tracer);
	}

	std::vector<int> InteractiveComb::checkLagestPlaner()
	{
		std::vector<int> faces;
		impl->findBottomDirection(faces);
		return faces;
	}
}