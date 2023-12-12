#include "cutting.h"
#include "internal/data/mmesht.h"
#include "internal/alg/letter.h"
#include "internal/alg/fillhoneycombs.h"
#include "internal/alg/trans.h"

namespace topomesh
{

	bool ModleCutting(const std::vector<trimesh::TriMesh*>& inMesh, std::vector<trimesh::TriMesh*>& outMesh, const SimpleCamera& camera,
		const TriPolygon& paths, ccglobal::Tracer* tracer)
	{
		if (paths.empty()) return false;			
		CameraParam cp;
		cp.lookAt = camera.center;
		cp.pos = camera.pos;
		cp.up = camera.up;
		cp.n = camera.n; cp.f = camera.f;
		cp.fov = camera.fov; cp.aspect = camera.aspect;
		loadCameraParam(cp);
		Eigen::Matrix4f viewMatrix;
		Eigen::Matrix4f projectionMatrix;
		getViewMatrixAndProjectionMatrix(cp, viewMatrix, projectionMatrix);		
		std::vector<MMeshT> meshts;
		for (int i = 0; i < inMesh.size(); i++)
		{
			inMesh[i]->need_adjacentfaces();
			inMesh[i]->need_neighbors();			
			TransformationMesh(inMesh[i], viewMatrix, projectionMatrix);
			if (JudgeMeshIsVaild(inMesh[i]))
			{
				meshts.push_back(inMesh[i]);
			}
		}
		std::vector<std::vector<trimesh::vec2>> lines(1);
		for (int i = 0; i < paths.size(); i++)
		{
			lines[0].push_back(trimesh::vec2(paths[i].x, paths[i].y));
		}
		
		std::vector<MMeshT> newMeshs;
		for (int i = 0; i < meshts.size(); i++)
		{
			std::vector<int> faceindex;
			for (int fi = 0; fi < meshts[i].faces.size(); fi++)
			{							
				faceindex.emplace_back(fi);
			}
			embedingAndCutting(&meshts[i], lines, faceindex);				
			faceindex.clear();
			for (int fi = 0; fi < meshts[i].faces.size(); fi++)
			{
				if(!meshts[i].faces[fi].IsD())
					faceindex.emplace_back(fi);
			}
			std::vector<int> outfacesIndex;
			std::vector<std::vector<std::vector<trimesh::vec2>>> poly(1);
			poly.emplace_back(lines);
			polygonInnerFaces(&meshts[i], poly, faceindex, outfacesIndex);
			std::vector<int> otherfaces;
			std::set_difference(faceindex.begin(),faceindex.end(),outfacesIndex.begin(),outfacesIndex.end(),std::back_inserter(otherfaces));
		
			MMeshT mt(&meshts[i], outfacesIndex);
			MMeshT mt1(&meshts[i], otherfaces);	
			
			ConnectMeshFace(&mt);
			ConnectMeshFace(&mt1);
			trimesh::TriMesh* trimesh = new trimesh::TriMesh();
			trimesh::TriMesh* trimesh1 = new trimesh::TriMesh();
			mt.mmesh2trimesh(trimesh);
			mt1.mmesh2trimesh(trimesh1);
			unTransformationMesh(trimesh, viewMatrix, projectionMatrix);
			unTransformationMesh(trimesh1, viewMatrix, projectionMatrix);
			outMesh.push_back(trimesh);
			outMesh.push_back(trimesh1);
			trimesh->write("trimesh.ply");
			trimesh1->write("trimesh1.ply");
		}
		if (meshts.empty()) return false;

		return true;
	}

	//------nlogn---
	bool JudgeCloseOfPath(const TriPolygon& paths)
	{
		for (int i = 0; i < paths.size() - 1; i++)
		{
			trimesh::point l1b = trimesh::point(paths[i].x, paths[i].y, 0);
			trimesh::point l1e = trimesh::point(paths[i + 1].x, paths[i + 1].y, 0);
			trimesh::point l1 = l1e - l1b;
			for (int j = i + 1; j < paths.size() - 1; j++)
			{
				trimesh::point l2b = trimesh::point(paths[j].x, paths[j].y, 0);
				trimesh::point l2e = trimesh::point(paths[j + 1].x, paths[j + 1].y, 0);
				trimesh::point l2 = l2e - l2b;
				trimesh::point n1 = l1b - l2b;
				trimesh::point n2 = l1b - l2e;
				trimesh::point n3 = l1e - l2b;
				if (((l1 % -n1) ^ (l1 % -n2)) >= 0 || ((l2 % n1) ^ (l2 % n3)) >= 0) continue;
				trimesh::point m = l1 % l2;
				trimesh::point n = n1 % l1;
				float t = (m.x != 0 && n.x != 0) ? n.x / m.x : (m.y != 0 && n.y != 0) ? n.y / m.y : (m.z != 0 && n.z != 0) ? n.z / m.z : -1;
				if (t > 0 && t < 1)
				{
					return true;
				}
			}
		}
		return false;
	}

	bool JudgeMeshIsVaild(const trimesh::TriMesh* inMesh)
	{
		for (int i = 0; i < inMesh->vertices.size(); i++)
		{
			if (inMesh->vertices[i].x > -1 && inMesh->vertices[i].x < 1 && inMesh->vertices[i].y < 1 && inMesh->vertices[i].y>-1)
				return true;
		}
		return false;
	}


	void setMarkOfCorssPoint(std::vector<MMeshT*>& meshs, const TriPolygon& paths, const std::vector<std::pair<int, int>>& corssPoint)
	{
		for (int i = 0; i < meshs.size(); i++)
		{
			for (int fi = 0; fi < meshs[i]->faces.size(); fi++)
			{
				for (int j = 0; j < corssPoint.size(); j++)
				{
					if (meshs[i]->faces[fi].innerFace(paths[corssPoint[j].first]) || meshs[i]->faces[fi].innerFace(paths[corssPoint[j].second]))
					{
						meshs[i]->faces[fi].SetL(); break;
					}
				}
			}
		}
	}


	void splitMesh(MMeshT* mesh, std::vector<MMeshT>& outmesh, std::vector<int>& order)
	{
		//std::vector<std::vector<int>> ConverFaces(2);
		for (int i = 0; i < mesh->faces.size(); i++)
		{
			mesh->faces[i].ClearS(); mesh->faces[i].ClearV();
			/*MMeshFace& f = mesh->faces[i];
			if (!f.IsD())
			{
				trimesh::point n = (f.V0(1)->p - f.V0(0)->p) % (f.V0(2)->p - f.V0(0)->p);
				if (n.z > 0)
					ConverFaces[0].push_back(i);
				else
				{
					ConverFaces[1].push_back(i); mesh->faces[i].SetV();
				}
			}*/
		}

		/*for (int c = 0; c < ConverFaces.size(); c++)
		{
			std::queue<int> queue;
			int fn = 0;
			while (fn < ConverFaces[c].size())
			{
				int index = -1;
				for (int i = 0; i < ConverFaces[c].size(); i++)
				{
					if (!mesh->faces[ConverFaces[c][i]].IsS())
					{
						index = i; break;
					}
				}
				if (index == -1)
					break;
				queue.push(index);
				mesh->faces[index].SetS();
				std::vector<int> faceindex;
				faceindex.push_back(index);
				fn++;
				while (!queue.empty())
				{
					for (int i = 0; i < mesh->faces[queue.front()].connect_face.size(); i++)if (!mesh->faces[queue.front()].connect_face[i]->IsD())
					{
						if (c == 1)
						{
							if (mesh->faces[queue.front()].connect_face[i]->IsV() && !mesh->faces[queue.front()].connect_face[i]->IsS())
							{
								bool pass = true;
								if (mesh->faces[queue.front()].IsB() && mesh->faces[queue.front()].connect_face[i]->IsB())
								{
									std::pair<int, int> edge = mesh->faces[queue.front()].connect_face[i]->commonEdge(&mesh->faces[queue.front()]);
									std::vector<int>::iterator it1 = std::find(order[c].begin(), order[c].end(), edge.first);
									std::vector<int>::iterator it2 = std::find(order[c].begin(), order[c].end(), edge.second);
									if (it1 != order[c].end() && it2 != order[c].end())
									{
										int distance1 = std::distance(order[c].begin(), it1);
										int distance2 = std::distance(order[c].begin(), it2);
										if (std::abs(distance1 - distance2) == 1)
											pass = false;
									}
								}
								if (pass)
								{
									queue.push(mesh->faces[queue.front()].connect_face[i]->index);
									mesh->faces[queue.front()].connect_face[i]->SetS();
									faceindex.push_back(mesh->faces[queue.front()].connect_face[i]->index); fn++;
								}
							}
						}
						else
						{

						}
					}
				}
			}
		}*/
		std::queue<int> queue;
		int fn = 0; int it = 0;

		while (fn < mesh->FN())
		{
			int index = -1;
			for (int i = 0; i < mesh->faces.size(); i++)
				if (!mesh->faces[i].IsS() && !mesh->faces[i].IsD())
				{
					index = i; break;
				}
			if (index == -1)
				break;
			queue.push(index);
			mesh->faces[index].SetS();
			std::vector<int> faceindex;
			faceindex.push_back(index);
			fn++;
			while (!queue.empty())
			{
				for (int i = 0; i < mesh->faces[queue.front()].connect_face.size(); i++)if (!mesh->faces[queue.front()].connect_face[i]->IsD())
				{
					if (!mesh->faces[queue.front()].connect_face[i]->IsS())
					{
						bool pass = true;
						if (mesh->faces[queue.front()].IsB() && mesh->faces[queue.front()].connect_face[i]->IsB())
						{
							std::pair<int, int> edge = mesh->faces[queue.front()].connect_face[i]->commonEdge(&mesh->faces[queue.front()]);
							std::vector<int>::iterator it1 = std::find(order.begin(), order.end(), edge.first);
							std::vector<int>::iterator it2 = std::find(order.begin(), order.end(), edge.second);
							if (it1 != order.end() && it2 != order.end())
							{
								int distance1 = std::distance(order.begin(), it1);
								int distance2 = std::distance(order.begin(), it2);
								if (std::abs(distance1 - distance2) == 1 || (std::min(distance1, distance2) == 0 && std::max(distance1, distance2) == order.size() - 1))
									pass = false;
							}
						}
						if (pass)
						{
							queue.push(mesh->faces[queue.front()].connect_face[i]->index);
							mesh->faces[queue.front()].connect_face[i]->SetS();
							faceindex.push_back(mesh->faces[queue.front()].connect_face[i]->index); fn++;
						}
					}
				}
				queue.pop();
			}

			MMeshT mt(mesh, faceindex);
			auto&& mtrr = std::move(mt);
			outmesh.push_back(std::move(mtrr));
			it++;
		}
		if (it == 1)
			outmesh.clear();

	}

	void getPathOrder(MMeshT* mesh, const TriPolygon& paths, int start, std::vector<std::vector<int>>& order)
	{
		order.resize(2);
		std::vector<std::vector<int>> container(2);
		//std::vector<std::vector<int>> outContainer(2);
		for (int i = start; i < mesh->vertices.size(); i++)
		{
			if (!mesh->vertices[i].IsS())
				container[0].push_back(i);
			else
				container[1].push_back(i);
			for (int j = 0; j < mesh->vertices[i].connected_face.size(); j++)
				mesh->vertices[i].connected_face[j]->SetB();
		}
		for (int c = 0; c < container.size(); c++)
		{
			int n = 0;
			std::vector<int> temp_container;
			for (int i = 0; i < container[c].size(); i++)
			{
				if (mesh->vertices[container[c][i]].inner[0] != n)
				{
					std::sort(temp_container.begin(), temp_container.end(), [&](int a, int b) ->bool {
						float l1 = trimesh::distance2(trimesh::vec2(mesh->vertices[a].p.x, mesh->vertices[a].p.y), trimesh::vec2(paths[n].x, paths[n].y));
						float l2 = trimesh::distance2(trimesh::vec2(mesh->vertices[b].p.x, mesh->vertices[b].p.y), trimesh::vec2(paths[n].x, paths[n].y));
						return l1 < l2;
						});
					for (int j = 0; j < temp_container.size(); j++)
						order[c].push_back(temp_container[j]);
					temp_container.clear();
					n = mesh->vertices[container[c][i]].inner[0];
				}
				if (mesh->vertices[container[c][i]].inner[0] == n)
				{
					temp_container.push_back(container[c][i]);
				}
				if (i == container[c].size() - 1)
				{
					std::sort(temp_container.begin(), temp_container.end(), [&](int a, int b) ->bool {
						float l1 = trimesh::distance2(trimesh::vec2(mesh->vertices[a].p.x, mesh->vertices[a].p.y), trimesh::vec2(paths[n].x, paths[n].y));
						float l2 = trimesh::distance2(trimesh::vec2(mesh->vertices[b].p.x, mesh->vertices[b].p.y), trimesh::vec2(paths[n].x, paths[n].y));
						return l1 < l2;
						});
					for (int j = 0; j < temp_container.size(); j++)
						order[c].push_back(temp_container[j]);
					temp_container.clear();
				}
			}
		}
		//std::reverse(outContainer[1].begin(),outContainer[1].end());
		////for (int i = 0; i < outContainer.size(); i++)
		//	for (int j = 0; j < outContainer[0].size(); j++)
		//		order.push_back(outContainer[0][j]);
	}

	void ConnectMeshFace(MMeshT* mesh)
	{		
		std::vector<int> userVertex;
		for (int vi = 0; vi < mesh->vertices.size(); vi++)
		{
			int u = mesh->vertices[vi].GetU();
			if (u > 0&&mesh->vertices[vi].IsL())
				userVertex.push_back(u);
		}
		std::sort(userVertex.begin(), userVertex.end());
		auto last = std::unique(userVertex.begin(), userVertex.end());
		userVertex.erase(last, userVertex.end());
		std::vector<std::vector<int>> ConnectVertex(userVertex.size());
		for (int i = 0; i < userVertex.size(); i++)
			for (int vi = 0; vi < mesh->vertices.size(); vi++)
				if (mesh->vertices[vi].GetU() == userVertex[i]&&mesh->vertices[vi].IsL())
					ConnectVertex[i].push_back(vi);
		
		for (int i = 0; i < ConnectVertex.size(); i++)
		{
			std::sort(ConnectVertex[i].begin(), ConnectVertex[i].end(), [&](int a,int b)->bool {
				return mesh->vertices[a].p.z < mesh->vertices[b].p.z;
				});
		}
		for (int i = 0; i < ConnectVertex.size(); i++)
		{
			std::vector<int> connect;
			std::vector<int> element= ConnectVertex[i];
			std::vector<int> next_element = ConnectVertex[(i + 1) % ConnectVertex.size()];
			std::reverse(next_element.begin(), next_element.end());
			connect.insert(connect.end(), element.begin(), element.end());
			connect.insert(connect.end(), next_element.begin(), next_element.end());
			fillTriangleForTraverse(mesh, connect);
		}
		
	}
	

}