#include "subdivision.h"
#include "trimesh2/TriMesh_algo.h"

#include <set>

namespace topomesh {
	void SimpleMidSubdivision(MMeshT* mesh, std::vector<int>& faceindexs)
	{
		for (int fi : faceindexs)
		{
			trimesh::point c = (mesh->faces[fi].V0(0)->p + mesh->faces[fi].V1(0)->p + mesh->faces[fi].V2(0)->p)/3.0;
			mesh->appendVertex(c);
			mesh->deleteFace(fi);
			mesh->appendFace(mesh->faces[fi].V0(0)->index, mesh->faces[fi].V0(1)->index, mesh->vertices.back().index);
			mesh->appendFace(mesh->faces[fi].V0(1)->index, mesh->faces[fi].V0(2)->index, mesh->vertices.back().index);
			mesh->appendFace(mesh->faces[fi].V0(2)->index, mesh->faces[fi].V0(0)->index, mesh->vertices.back().index);
		}
	}

	void loopSubdivision(MMeshT* mesh, std::vector<int>& faceindexs, std::vector<std::tuple<int, trimesh::point>>& vertex,
		std::vector<std::tuple<int, trimesh::ivec3>>& face_vertex, bool is_move , int iteration)
	{
		auto bate = [&](int n)->float {
			float alpth = 3.0f / 8.0f + 1.f / 4.f * std::cos(2.0f * M_PI / (n * 1.0f));
			return (5.0f / 8.0f - alpth * alpth)/(n*1.0f);
		};		
		if (mesh->half_edge.empty()) mesh->init_halfedge();
		std::set<int> vertex_container;
		for (int& i : faceindexs)
		{
			mesh->faces[i].SetS();
			mesh->faces[i].V0(0)->SetS(); vertex_container.insert(mesh->faces[i].V0(0)->index);
			mesh->faces[i].V1(0)->SetS(); vertex_container.insert(mesh->faces[i].V1(0)->index);
			mesh->faces[i].V2(0)->SetS(); vertex_container.insert(mesh->faces[i].V2(0)->index);
		}
		for (int& i : faceindexs)
		{
			if (mesh->faces[i].f_mhe->opposite == nullptr || !mesh->faces[i].f_mhe->opposite->indication_face->IsS())
			{
				mesh->faces[i].f_mhe->SetB(); mesh->faces[i].f_mhe->edge_vertex.first->SetB(); mesh->faces[i].f_mhe->edge_vertex.second->SetB();
			}
			if (mesh->faces[i].f_mhe->next->opposite==nullptr||!mesh->faces[i].f_mhe->next->opposite->indication_face->IsS())
			{
				mesh->faces[i].f_mhe->next->SetB(); mesh->faces[i].f_mhe->next->edge_vertex.first->SetB(); mesh->faces[i].f_mhe->next->edge_vertex.second->SetB();
			}
			if (mesh->faces[i].f_mhe->next->next->opposite==nullptr||!mesh->faces[i].f_mhe->next->next->opposite->indication_face->IsS())
			{
				mesh->faces[i].f_mhe->next->next->SetB(); mesh->faces[i].f_mhe->next->next->edge_vertex.first->SetB(); mesh->faces[i].f_mhe->next->next->edge_vertex.second->SetB();
			}
		}
		for (int& i : faceindexs)
		{
			mesh->deleteFace(i);
			MMeshHalfEdge* halfedge_ptr=mesh->faces[i].f_mhe;
			std::vector<MMeshVertex*> newVertex;
			do
			{
				if (halfedge_ptr->IsS())
				{
					newVertex.push_back(&mesh->vertices[halfedge_ptr->attritube]);
					halfedge_ptr = halfedge_ptr->next;
					continue;
				}
				halfedge_ptr->SetS();
				halfedge_ptr->opposite->SetS();
				if (halfedge_ptr->IsB())
				{
					mesh->appendVertex((halfedge_ptr->edge_vertex.first->p + halfedge_ptr->edge_vertex.second->p) / 2.0);
					mesh->vertices.back().SetB();
					vertex.push_back(std::make_tuple(mesh->vertices.back().index, mesh->vertices.back().p));
					mesh->deleteFace(halfedge_ptr->opposite->indication_face->index);
					mesh->appendFace(mesh->vertices.size() - 1, halfedge_ptr->opposite->edge_vertex.second->index, halfedge_ptr->opposite->next->edge_vertex.second->index);
					face_vertex.push_back(std::make_tuple(mesh->faces.back().index,
						trimesh::ivec3(mesh->vertices.size() - 1, halfedge_ptr->opposite->edge_vertex.second->index, halfedge_ptr->opposite->next->edge_vertex.second->index)));
					mesh->appendFace(mesh->vertices.size() - 1, halfedge_ptr->opposite->next->next->edge_vertex.first->index, halfedge_ptr->opposite->next->next->edge_vertex.second->index);
					face_vertex.push_back(std::make_tuple(mesh->faces.back().index,
						trimesh::ivec3(mesh->vertices.size() - 1, halfedge_ptr->opposite->next->next->edge_vertex.first->index, halfedge_ptr->opposite->next->next->edge_vertex.second->index)));
				}
				else
				{
					if (is_move)
					{
						trimesh::point np = 3.0 * (halfedge_ptr->edge_vertex.first->p + halfedge_ptr->edge_vertex.second->p) / 8.0;
						trimesh::point tp = 1.0 * (halfedge_ptr->next->edge_vertex.second->p + halfedge_ptr->opposite->next->edge_vertex.second->p) / 8.0;
						mesh->appendVertex(np + tp);
						vertex.push_back(std::make_tuple(mesh->vertices.back().index, mesh->vertices.back().p));
					}
					else
					{
						mesh->appendVertex((halfedge_ptr->edge_vertex.first->p + halfedge_ptr->edge_vertex.second->p) / 2.0);
						vertex.push_back(std::make_tuple(mesh->vertices.back().index, mesh->vertices.back().p));
					}
					
				}
				halfedge_ptr->attritube = mesh->vertices.size() - 1;
				halfedge_ptr->opposite->attritube=mesh->vertices.size() - 1;
				newVertex.push_back(&mesh->vertices[mesh->vertices.size() - 1]);
				halfedge_ptr = halfedge_ptr->next;
			} while (halfedge_ptr!=mesh->faces[i].f_mhe);		
			//trimesh::point normal = trimesh::normalized((mesh->faces[i].V1(0)->p - mesh->faces[i].V0(0)->p) % (mesh->faces[i].V2(0)->p - mesh->faces[i].V0(0)->p));
			mesh->appendFace(mesh->faces[i].V0(0)->index, newVertex[0]->index, newVertex[2]->index);
			face_vertex.push_back(std::make_tuple(mesh->faces.back().index, trimesh::ivec3(mesh->faces[i].V0(0)->index, newVertex[0]->index, newVertex[2]->index)));
			mesh->appendFace(mesh->faces[i].V1(0)->index, newVertex[1]->index, newVertex[0]->index);
			face_vertex.push_back(std::make_tuple(mesh->faces.back().index, trimesh::ivec3(mesh->faces[i].V1(0)->index, newVertex[1]->index, newVertex[0]->index)));
			mesh->appendFace(mesh->faces[i].V2(0)->index, newVertex[2]->index, newVertex[1]->index);
			face_vertex.push_back(std::make_tuple(mesh->faces.back().index, trimesh::ivec3(mesh->faces[i].V2(0)->index, newVertex[2]->index, newVertex[1]->index)));
			mesh->appendFace(newVertex[0]->index, newVertex[1]->index, newVertex[2]->index);
			face_vertex.push_back(std::make_tuple(mesh->faces.back().index, trimesh::ivec3(newVertex[0]->index, newVertex[1]->index, newVertex[2]->index)));
		}
		std::set<int>::iterator it;
		if (is_move)
		{
			for (it = vertex_container.begin(); it != vertex_container.end(); it++)
			{
				MMeshVertex& v = mesh->vertices[*it];
				trimesh::point cp;
				int ln = v.connected_vertex.size();
				float b = bate(ln);
				trimesh::point vv_p;
				for (MMeshVertex* vv : v.connected_vertex)
				{
					vv_p += vv->p;
					if (vv->IsB())
						cp += vv->p;
				}
				if (!v.IsB())
				{
					v.p = (1 - (float)ln * b) * v.p + b * vv_p;
				}
				else
				{
					v.p = 3.f / 4.f * v.p + 1.f / 8.f * cp;
				}
			}
		}
		for (it = vertex_container.begin(); it != vertex_container.end(); it++)
		{
			MMeshVertex& v = mesh->vertices[*it];
			for (MMeshVertex* vv : v.connected_vertex)
				vv->ClearALL();			
		}
	}

	void sqrt3Subdivision(MMeshT* mesh, std::vector<int>& faceindexs)
	{

	}
}