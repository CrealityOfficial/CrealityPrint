#include "util.h"
#include <assert.h>

#include "BRepTools.hxx"
#include "BRep_Tool.hxx"

#include "BRepMesh_IncrementalMesh.hxx"
#include "TopExp_Explorer.hxx"
#include "TopoDS_Face.hxx"
#include "TopoDS.hxx"

#include "ccglobal/log.h"

namespace cadcore
{
	trimesh::TriMesh* Shape_Triangulation(const TopoDS_Shape& shape, Standard_Real deflection)
	{
		if (!BRepTools::Triangulation(shape, deflection))
		{
			if (!BRepMesh_IncrementalMesh(shape, deflection, false, std::min(0.1, deflection * 5 + 0.005), true).IsDone())
			{
				return nullptr;
			}
		}

		trimesh::TriMesh* mesh = new trimesh::TriMesh();

		int vertex_id_start = 0;
		for (TopExp_Explorer aFaceExp(shape, TopAbs_FACE); aFaceExp.More(); aFaceExp.Next())
		{
			TopoDS_Face aFace = TopoDS::Face(aFaceExp.Current());

			if (aFace.IsNull())
			{
				continue;
			}

			LOGI("Shape_Triangulation processing face: %d", aFace.HashCode(1000));

			TopLoc_Location aLoc;
			Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(aFace, aLoc);

			if (triangulation.IsNull())
			{
				continue;
			}

			Standard_Boolean face_reversed = (TopAbs_REVERSED == aFace.Orientation());

			vertex_id_start = mesh->vertices.size();

			// set verteces
			for (Standard_Integer i = 0; i < triangulation->NbNodes(); i++)
			{
				const gp_Pnt& pnt = triangulation->Node(i + 1);
				mesh->vertices.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
			}

			// set triangles
			for (Standard_Integer i = 0; i < triangulation->NbTriangles(); i++)
			{
				Standard_Integer n1 = -1, n2 = -1, n3 = -1;

				triangulation->Triangle(i + 1).Get(n1, n2, n3);

				n1 += vertex_id_start - 1;
				n2 += vertex_id_start - 1;
				n3 += vertex_id_start - 1;

				if (face_reversed)
				{
					mesh->faces.emplace_back(n3, n2, n1);
				}
				else
				{
					mesh->faces.emplace_back(n1, n2, n3);
				}
			}
		}

		return mesh;
	}
}