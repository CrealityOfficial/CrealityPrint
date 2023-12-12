#include "letter.h"

#include "ccglobal/log.h"

#ifdef _WIN32
#include "omp.h"
#endif

#include "internal/alg/trans.h"

#define FLOATERR 1e-8f
#define BLOCK 80000


namespace topomesh
{
	void concaveOrConvexOfFaces(MMeshT* mt, std::vector<int>& faces, bool concave,float deep)
	{
		mt->getMeshBoundaryFaces();
		trimesh::point ave_normal;
		for (int i = 0; i < faces.size(); i++)if (!mt->faces[faces[i]].IsD())
		{
			if (mt->faces[faces[i]].IsB()) continue;
			ave_normal += trimesh::trinorm(mt->faces[faces[i]].connect_vertex[0]->p, mt->faces[faces[i]].connect_vertex[1]->p, mt->faces[faces[i]].connect_vertex[2]->p);
			mt->faces[faces[i]].SetS();
			mt->faces[faces[i]].V0(0)->SetS();
			mt->faces[faces[i]].V0(1)->SetS();
			mt->faces[faces[i]].V0(2)->SetS();				
		}					
		ave_normal /= faces.size();		
		trimesh::normalize(ave_normal);	
	
		for (MMeshVertex& v : mt->vertices)if (!v.IsD() && v.IsS())
		{
			
			for (MMeshFace* f : v.connected_face)
				if (!f->IsS())
				{
					v.SetV(); break;
				}
		}
		
		if (concave)
			ave_normal = -ave_normal;
		for (MMeshVertex& v : mt->vertices)if (!v.IsD())
		{
			if (v.IsS())
			{
				if (!v.IsV())
					v.p += ave_normal*deep;					
				else
					splitPoint(mt, &v, ave_normal*deep);
					
			}
		}	
	}

	void splitPoint(MMeshT* mt, MMeshVertex* v, trimesh::point ori)
	{
		mt->appendVertex(trimesh::point(v->p + ori));
		for (MMeshFace* f : v->connected_face)if (!f->IsD())
		{
			f->SetV();
			if (f->IsS())
			{
				f->V1(v)->SetA(); f->V2(v)->SetA();
				int vin = f->getVFindex(v);
				f->connect_vertex[vin] = &mt->vertices.back();
				mt->vertices.back().connected_face.push_back(f);
			}
		}

		for (MMeshFace* f : v->connected_face)if (!f->IsD())
		{
			std::vector<MMeshFace*>::iterator it;
			if (f->IsS())
				for (it = f->connect_face.begin(); it != f->connect_face.end();)
				{
					if ((*it)->IsV() && !(*it)->IsS())
						it = f->connect_face.erase(it);
					else
						it++;
				}
			if (!f->IsS())
				for (it = f->connect_face.begin(); it != f->connect_face.end(); )
				{
					if ((*it)->IsV() && (*it)->IsS())
						it = f->connect_face.erase(it);
					else
						it++;
				}
		}
		for(unsigned i=0;i<v->connected_vertex.size();i++)if(!v->connected_vertex[i]->IsD())
		{
			if (v->connected_vertex[i]->IsA(1))
			{			
				mt->appendFace(v->connected_vertex[i]->index, v->index, mt->vertices.size() - 1);			
			}
			if (v->connected_vertex[i]->IsA(1) || v->connected_vertex[i]->IsA(2))
				mt->vertices.back().connected_vertex.push_back(v->connected_vertex[i]);
		}

		for (MMeshFace* f : v->connected_face)if (!f->IsD())
		{
			f->ClearV();
			f->connect_vertex[0]->ClearA();
			f->connect_vertex[1]->ClearA();
			f->connect_vertex[2]->ClearA();
		}
	}

	
	void embedingAndCutting(MMeshT* mesh,const std::vector<std::vector<trimesh::vec2>>& lines,const std::vector<int>& facesIndex, bool is_close)
	{			
		auto crossProduct = [=](trimesh::vec2 p1, trimesh::vec2 p2) ->float {
			return p1.x * p2.y - p1.y * p2.x;
		};
#if 0
				
		for (int i = 0; i < lines.size(); i++)
		{
			for (int j = 0; j < lines[i].size(); j++)
			{
				for (MMeshFace& f : mesh->faces)
				{
					trimesh::vec2 v10 = trimesh::vec2(lines[i][j].x, lines[i][j].y) - trimesh::vec2(f.V0(0)->p.x, f.V0(0)->p.y);
					trimesh::vec2 v20 = trimesh::vec2(lines[i][j].x, lines[i][j].y) - trimesh::vec2(f.V0(1)->p.x, f.V0(1)->p.y);
					trimesh::vec2 v30 = trimesh::vec2(lines[i][j].x, lines[i][j].y) - trimesh::vec2(f.V0(2)->p.x, f.V0(2)->p.y);
					trimesh::vec2 v12 = trimesh::vec2(f.V0(1)->p.x, f.V0(1)->p.y) - trimesh::vec2(f.V0(0)->p.x, f.V0(0)->p.y);//1->2
					trimesh::vec2 v13 = trimesh::vec2(f.V0(2)->p.x, f.V0(2)->p.y) - trimesh::vec2(f.V0(0)->p.x, f.V0(0)->p.y);//0->2
					trimesh::vec2 v23 = trimesh::vec2(f.V0(2)->p.x, f.V0(2)->p.y) - trimesh::vec2(f.V0(1)->p.x, f.V0(1)->p.y);//2->3
					trimesh::vec2 v31 = trimesh::vec2(f.V0(0)->p.x, f.V0(0)->p.y) - trimesh::vec2(f.V0(2)->p.x, f.V0(2)->p.y);//3->1
					trimesh::point vv01 = f.V1(0)->p - f.V0(0)->p;
					trimesh::point vv02 = f.V2(0)->p - f.V0(0)->p;
					if ((crossProduct(v12, v10) >= 0 && crossProduct(v23, v20) >= 0 && crossProduct(v31, v30) >= 0) ||
						(crossProduct(v12, v10) < 0 && crossProduct(v23, v20) < 0 && crossProduct(v31, v30) < 0))
					{
						if(j==0)
							f.SetV();
						Eigen::Matrix2f e;
						e << v12.x, v13.x, v12.y, v13.y;
						Eigen::Vector2f b = { lines[i][j].x - f.V0(0)->p.x ,lines[i][j].y - f.V0(0)->p.y };
						Eigen::Vector2f x = e.fullPivLu().solve(b);
						mesh->appendVertex(trimesh::point(f.V0(0)->p + x.x() * vv01 + x.y() * vv02));						
						f.uv_coord.push_back(trimesh::vec4(-1, mesh->vertices.back().index, j, i));
					}
				}
			}
		}
		std::vector<trimesh::ivec2> edge;
		mesh->getEdge(edge);
		for (int i = 0; i < lines.size(); i++)
		{
			for (int j = 0; j < lines[i].size(); j++)
			{
				std::vector<std::pair<float, trimesh::ivec2>> corsspoint;
				mesh->calculateCrossPoint(edge, std::make_pair(trimesh::point(lines[i][j].x, lines[i][j].y, 0), trimesh::point(lines[i][(j + 1) % lines[i].size()].x, lines[i][(j + 1) % lines[i].size()].y, 0)), corsspoint);
				for (std::pair<float, trimesh::ivec2>& cp : corsspoint)
				{
					bool cover = false;
					for (MMeshFace* f : mesh->vertices[cp.second.x].connected_face)if (!f->IsD())
						f->SetA();
					for (MMeshFace* f : mesh->vertices[cp.second.y].connected_face)if (!f->IsD())
						f->SetA();
					for (MMeshFace* f : mesh->vertices[cp.second.x].connected_face)if (f->IsA(2) && !f->IsD())
					{						
						f->SetS();
						trimesh::point d = mesh->vertices[cp.second.y].p - mesh->vertices[cp.second.x].p;
						int index1 = f->getVFindex(&mesh->vertices[cp.second.x]);
						int index2 = f->getVFindex(&mesh->vertices[cp.second.y]);
						if (!cover)
						{
							mesh->appendVertex(trimesh::point(mesh->vertices[cp.second.x].p + cp.first * d)); cover = true;
						}
						if (f->V1(index1) == &mesh->vertices[cp.second.y])
							f->uv_coord.push_back(trimesh::vec4(index1, mesh->vertices.size() - 1, j, i));
						else
							f->uv_coord.push_back(trimesh::vec4(index2, mesh->vertices.size() - 1, j, i));
					}
					for (MMeshFace* f : mesh->vertices[cp.second.x].connected_face)if (!f->IsD())
						f->ClearA();
					for (MMeshFace* f : mesh->vertices[cp.second.y].connected_face)if (!f->IsD())
						f->ClearA();

				}
			}
		}
		
#else
		//---------new-----			
		for(int i = 0; i < lines.size(); i++)
		{
			for (int j = 0; j < lines[i].size(); j++)
			{			
				std::vector<trimesh::ivec3> push_lines;
				for (int fi : facesIndex)if (!mesh->faces[fi].IsD())
				{
					MMeshFace& f = mesh->faces[fi];
					/*trimesh::point n = (trimesh::point(f.V0(1)->p.x, f.V0(1)->p.y,0) - trimesh::point(f.V0(0)->p.x, f.V0(0)->p.y, 0)) % 
						(trimesh::point(f.V0(2)->p.x, f.V0(2)->p.y, 0) - trimesh::point(f.V0(0)->p.x, f.V0(0)->p.y, 0));*/
					for (int k = 0; k < f.inner_vertex.size(); k++)
					{
						if (f.inner_vertex[k].z == j && f.inner_vertex[k].w == i)
						{
							push_lines.push_back(trimesh::ivec3(std::min(f.V0(0)->index, f.V0(1)->index), std::max(f.V0(0)->index, f.V0(1)->index), f.inner_vertex[k].y));
						}
					}
					trimesh::vec2 v10 = trimesh::vec2(lines[i][j].x, lines[i][j].y)-trimesh::vec2(f.V0(0)->p.x, f.V0(0)->p.y);
					trimesh::vec2 v20 = trimesh::vec2(lines[i][j].x, lines[i][j].y)-trimesh::vec2(f.V0(1)->p.x, f.V0(1)->p.y);
					trimesh::vec2 v30 = trimesh::vec2(lines[i][j].x, lines[i][j].y)-trimesh::vec2(f.V0(2)->p.x, f.V0(2)->p.y);
					trimesh::vec2 v12 = trimesh::vec2(f.V0(1)->p.x, f.V0(1)->p.y) - trimesh::vec2(f.V0(0)->p.x, f.V0(0)->p.y);//1->2
					trimesh::vec2 v13 = trimesh::vec2(f.V0(2)->p.x, f.V0(2)->p.y) - trimesh::vec2(f.V0(0)->p.x, f.V0(0)->p.y);//0->2
					trimesh::vec2 v23 = trimesh::vec2(f.V0(2)->p.x, f.V0(2)->p.y) - trimesh::vec2(f.V0(1)->p.x, f.V0(1)->p.y);//2->3
					trimesh::vec2 v31 = trimesh::vec2(f.V0(0)->p.x, f.V0(0)->p.y) - trimesh::vec2(f.V0(2)->p.x, f.V0(2)->p.y);//3->1
					trimesh::point vv01 = f.V1(0)->p - f.V0(0)->p;
					trimesh::point vv02 = f.V2(0)->p - f.V0(0)->p;
					if ((crossProduct(v12, v10) >= 0 && crossProduct(v23, v20) >= 0 && crossProduct(v31, v30) >= 0) ||
						(crossProduct(v12, v10) < 0 && crossProduct(v23, v20) < 0 && crossProduct(v31, v30) < 0))
					{
						if (j == 0)
							f.SetV(); 					
						Eigen::Matrix2f e;
						e << v12.x, v13.x, v12.y, v13.y;
						Eigen::Vector2f b = { lines[i][j].x - f.V0(0)->p.x ,lines[i][j].y - f.V0(0)->p.y };
						Eigen::Vector2f x = e.fullPivLu().solve(b);
#ifdef _OPENMP
#pragma omp critical
#endif // _OPENMP
						{
							mesh->appendVertex(trimesh::point(f.V0(0)->p + x.x() * vv01 + x.y() * vv02)); 								
							mesh->vertices.back().inner.push_back(j+1);								
							//mesh->vertices.back().SetL();
						}
						f.uv_coord.push_back(trimesh::vec4(-1, mesh->vertices.back().index, j, i));
					}					
					std::vector<trimesh::ivec2> edge = {trimesh::ivec2(f.V0(0)->index,f.V1(0)->index),trimesh::ivec2(f.V1(0)->index,f.V2(0)->index) ,trimesh::ivec2(f.V2(0)->index,f.V0(0)->index) };
					std::vector<std::pair<float, trimesh::ivec2>> corsspoint;
					if(is_close)
						mesh->calculateCrossPoint(edge, std::make_pair(trimesh::point(lines[i][j].x, lines[i][j].y, 0), trimesh::point(lines[i][(j + 1) % lines[i].size()].x, lines[i][(j + 1) % lines[i].size()].y, 0)), corsspoint);
					else
					{
						if(j<lines[i].size()-1)
							mesh->calculateCrossPoint(edge, std::make_pair(trimesh::point(lines[i][j].x, lines[i][j].y, 0), trimesh::point(lines[i][j + 1].x, lines[i][j + 1].y, 0)), corsspoint);
					}
					std::vector<trimesh::vec4> dis;
					for (std::pair<float, trimesh::ivec2>& cp : corsspoint)
					{
						f.SetS();						
						int index = f.getVFindex(&mesh->vertices[cp.second.x]);
						bool pass = false;
						for(trimesh::ivec3& lp: push_lines)
							if (std::min(cp.second.x, cp.second.y) == lp.x && std::max(cp.second.x, cp.second.y) == lp.y)
							{								
								dis.push_back(trimesh::vec4(index, lp.z, j, i));
								pass = true; break;
							}
						if (pass)
							continue;
						trimesh::point d = mesh->vertices[cp.second.y].p - mesh->vertices[cp.second.x].p;
#ifdef _OPENMP
#pragma omp critical
#endif // _OPENMP
						{
							mesh->appendVertex(trimesh::point(mesh->vertices[cp.second.x].p + cp.first * d));							
							mesh->vertices.back().inner.push_back(j);																						
						}					
						dis.push_back(trimesh::vec4(index, mesh->vertices.size() - 1, j, i));
						push_lines.push_back(trimesh::ivec3(std::min(cp.second.x, cp.second.y), std::max(cp.second.x, cp.second.y), mesh->vertices.size() - 1));
					}
					if (dis.empty()) continue;
					else if (dis.size() == 1)
						f.uv_coord.push_back(dis[0]);
					else
					{
						float a = trimesh::distance2(lines[i][j],trimesh::vec2(mesh->vertices[dis[0].y].p.x, mesh->vertices[dis[0].y].p.y));
						float b = trimesh::distance2(lines[i][j], trimesh::vec2(mesh->vertices[dis[1].y].p.x, mesh->vertices[dis[1].y].p.y));
						if (a < b)
						{
							f.uv_coord.push_back(dis[0]); f.uv_coord.push_back(dis[1]);
						}
						else
						{
							f.uv_coord.push_back(dis[1]); f.uv_coord.push_back(dis[0]);
						}
					}
				}
			}
		}
#endif		
		
		
		int facesize = facesIndex.size();
		for (int i=0; i<facesize; i++)
		{
			int fi = facesIndex[i];
			if (!mesh->faces[fi].IsD() && (mesh->faces[fi].IsS() || mesh->faces[fi].IsV()))
			{
#if 0
				mesh->deleteFace(f);
				f.ClearS();
				if (!f.IsV())
					if (f.uv_coord.size() == 2)
					{
						if (f.V1(f.uv_coord[0].x)->index == f.V0(f.uv_coord[1].x)->index)
						{
							mesh->appendFace(f.V0(f.uv_coord[0].x)->index, f.uv_coord[0].y, f.V2(f.uv_coord[0].x)->index);
							mesh->appendFace(f.uv_coord[0].y, f.V0(f.uv_coord[1].x)->index, f.uv_coord[1].y);
							mesh->appendFace(f.uv_coord[0].y, f.uv_coord[1].y, f.V2(f.uv_coord[0].x)->index);
						}
						else
						{
							mesh->appendFace(f.V0(f.uv_coord[1].x)->index, f.uv_coord[1].y, f.V1(f.uv_coord[0].x)->index);
							mesh->appendFace(f.V0(f.uv_coord[0].x)->index, f.uv_coord[0].y, f.uv_coord[1].y);
							mesh->appendFace(f.V1(f.uv_coord[0].x)->index, f.uv_coord[0].y, f.uv_coord[1].y);
						}
					}
					else//----һ�������汻�������и�			
					{
						std::vector<std::pair<int, float>> faceVertexSque;
						for (int j = 0; j < 3; j++)
						{
							trimesh::point v1 = f.V1(j)->p - f.V0(j)->p;
							trimesh::point v2 = f.V2(j)->p - f.V0(j)->p;
							float a = std::acosf(trimesh::normalized(v1) ^ trimesh::normalized(v2));
							faceVertexSque.push_back(std::make_pair(f.V0(j)->index, a));
							std::vector<std::pair<int, float>> verVertexSque;
							for (int i = 0; i < f.uv_coord.size(); i++)
							{
								if (j == f.uv_coord[i].x)
								{
									verVertexSque.push_back(std::make_pair((int)f.uv_coord[i].y, M_PI));
								}
							}
							std::sort(verVertexSque.begin(), verVertexSque.end(), [&](std::pair<int, float> a, std::pair<int, float> b)->bool {
								float ad = trimesh::distance2(mesh->vertices[a.first].p, f.V0(j)->p);
								float bd = trimesh::distance2(mesh->vertices[b.first].p, f.V0(j)->p);
								return ad < bd;
								});
							faceVertexSque.insert(faceVertexSque.end(), verVertexSque.begin(), verVertexSque.end());
						}

						std::vector<std::pair<int, int>> lines;
						for (int i = 0; i < f.uv_coord.size(); i++)
							for (int j = i + 1; j < f.uv_coord.size(); j++)
							{
								if ((f.uv_coord[i].z == f.uv_coord[j].z) && (f.uv_coord[i].w == f.uv_coord[j].w))
									lines.push_back(std::make_pair(f.uv_coord[i].y, f.uv_coord[j].y));
							}
						for (int i = 0; i < faceVertexSque.size(); i++)
							mesh->vertices[faceVertexSque[i].first].inner.clear();
						for (int i = 0; i < lines.size(); i++)
						{
							for (int j = 0; j < faceVertexSque.size(); j++)
							{
								if (lines[i].first == faceVertexSque[j].first)
								{
									mesh->appendFace(lines[i].first, faceVertexSque[(j + 1) % faceVertexSque.size()].first, lines[i].second);
									mesh->vertices[lines[i].first].inner.push_back(mesh->faces.size() - 1);  //���±겻��index face index  ��һ�������±�
									mesh->vertices[faceVertexSque[(j + 1) % faceVertexSque.size()].first].inner.push_back(mesh->faces.size() - 1);
									mesh->vertices[lines[i].second].inner.push_back(mesh->faces.size() - 1);
								}
								if (lines[i].second == faceVertexSque[j].first)
								{
									mesh->appendFace(lines[i].first, lines[i].second, faceVertexSque[(j + 1) % faceVertexSque.size()].first);
									mesh->vertices[lines[i].first].inner.push_back(mesh->faces.size() - 1);
									mesh->vertices[faceVertexSque[(j + 1) % faceVertexSque.size()].first].inner.push_back(mesh->faces.size() - 1);
									mesh->vertices[lines[i].second].inner.push_back(mesh->faces.size() - 1);
								}
							}
						}
						std::vector<int> last_face;
						for (int i = 0; i < faceVertexSque.size(); i++)
						{
							float a = 0;
							MMeshVertex* v = &mesh->vertices[faceVertexSque[i].first];
							for (int j = 0; j < v->inner.size(); j++)
							{
								trimesh::point v01 = trimesh::normalized(mesh->faces[v->inner[j]].V1(v)->p - v->p);
								trimesh::point v02 = trimesh::normalized(mesh->faces[v->inner[j]].V2(v)->p - v->p);
								a += std::acosf(v01 ^ v02);
							}
							if (a < faceVertexSque[i].second - FLOATERR || a > faceVertexSque[i].second + FLOATERR)
								last_face.push_back(faceVertexSque[i].first);
						}
						if (last_face.size() == 3)
							mesh->appendFace(last_face[0], last_face[1], last_face[2]);
					}
				else//����			
				{
					if (f.inner_vertex.size() == 1)//ֻ��һ���� ֱ���ʷ�
					{
						std::vector<int> faceVertexSque;
						for (int i = 0; i < f.connect_vertex.size(); i++)
						{
							faceVertexSque.push_back(f.V0(i)->index);
							std::vector<int> verVertexSque;
							for (int j = 0; j < f.uv_coord.size(); j++)
							{
								if (f.uv_coord[j].x == i)
									verVertexSque.push_back(f.uv_coord[j].y);
							}
							std::sort(verVertexSque.begin(), verVertexSque.end(), [&](int a, int b)->bool {
								float ad = trimesh::distance2(mesh->vertices[a].p, f.V0(i)->p);
								float bd = trimesh::distance2(mesh->vertices[b].p, f.V0(i)->p);
								return ad < bd;
								});
							faceVertexSque.insert(faceVertexSque.end(), verVertexSque.begin(), verVertexSque.end());
						}

						for (int i = 0; i < faceVertexSque.size(); i++)
						{
							mesh->appendFace(faceVertexSque[i], faceVertexSque[(i + 1) % faceVertexSque.size()], f.inner_vertex[0]);
						}
					}
					else//�ڲ������
					{

					}
				}
				f.ClearV();

#else		

				MMeshFace& f = mesh->faces[fi];				
				if (f.uv_coord.size() == 1)
					continue;
				std::vector<int> facelines;
				int q = 0;
				for (int ii = 0; ii < f.uv_coord.size(); ii++)
				{
					if (ii == 0)
						facelines.push_back(f.uv_coord[ii].w);
					if (f.uv_coord[ii].w != facelines.back())
						facelines.push_back(f.uv_coord[ii].w);
					if (f.uv_coord[ii].x != -1)
						q++;
				}
				if (q % 2 != 0)
					continue;

				for (int ii = 0; ii < facelines.size(); ii++)
				{
					bool corss = false;
					for (int j = 0; j < f.uv_coord.size(); j++)
					{
						if (facelines[ii] == f.uv_coord[j].w)
							if (f.uv_coord[j].x != -1)
							{
								corss = true; break;
							}
					}
					if (!corss)
					{
						f.SetB(); break;
					}
				}
				if (f.IsB())
					continue;
				mesh->deleteFace(f);
				if (f.uv_coord.size() == 2)
				{
					if (f.V1(f.uv_coord[0].x)->index == f.V0(f.uv_coord[1].x)->index)
					{
#ifdef _OPENMP
#pragma omp critical
#endif // _OPENMP
						{
							mesh->appendFace(f.V0(f.uv_coord[0].x)->index, f.uv_coord[0].y, f.V2(f.uv_coord[0].x)->index);
							mesh->appendFace(f.uv_coord[0].y, f.V0(f.uv_coord[1].x)->index, f.uv_coord[1].y);
							mesh->appendFace(f.uv_coord[0].y, f.uv_coord[1].y, f.V2(f.uv_coord[0].x)->index);							
						}
					}
					else
					{
#ifdef _OPENMP
#pragma omp critical
#endif // _OPENMP
						{
							mesh->appendFace(f.V0(f.uv_coord[1].x)->index, f.uv_coord[1].y, f.V1(f.uv_coord[0].x)->index);
							mesh->appendFace(f.V0(f.uv_coord[0].x)->index, f.uv_coord[0].y, f.uv_coord[1].y);
							mesh->appendFace(f.V1(f.uv_coord[0].x)->index, f.uv_coord[0].y, f.uv_coord[1].y);							
						}
					}

					continue;
				}

				std::vector<int> faceVertexSque;
				for (int i = 0; i < f.connect_vertex.size(); i++)
				{
					f.connect_vertex[i]->SetV();
					faceVertexSque.push_back(f.connect_vertex[i]->index);
					std::vector<int> verVertexSque;
					for (int j = 0; j < f.uv_coord.size(); j++)
					{
						if (f.uv_coord[j].x == i)
							verVertexSque.push_back(f.uv_coord[j].y);
					}
					std::sort(verVertexSque.begin(), verVertexSque.end(), [&](int a, int b)->bool {
						float ad = trimesh::distance2(mesh->vertices[a].p, f.V0(i)->p);
						float bd = trimesh::distance2(mesh->vertices[b].p, f.V0(i)->p);
						return ad < bd;
						});
					faceVertexSque.insert(faceVertexSque.end(), verVertexSque.begin(), verVertexSque.end());
				}
				int l = 1;
				std::vector<std::vector<int>> innerPoint(f.uv_coord.size());
				std::vector<bool> curve(f.uv_coord.size(), false);
				std::vector<int>  lastIndex(f.uv_coord.size(), -1);
				if (!f.IsV())
				{
					int n = 0;
					for (int i = 0; i < f.uv_coord.size(); i++)
					{
						mesh->vertices[f.uv_coord[i].y].SetU(l);
						lastIndex[l] = f.uv_coord[i].y;
						if (f.uv_coord[i].x != -1)
							n++;
						else
						{
							curve[l] = true;
							innerPoint[l].push_back(f.uv_coord[i].y);
						}
						if (n == 2)
						{
							n = 0; l++;
						}
					}
				}
				else
				{
					int ln = -1;
					int n = 0;
					bool innerOrCorss = false;
					std::vector<std::vector<trimesh::ivec2>> mark_lines;
					for (int i = 0; i < f.uv_coord.size(); i++)
					{
						if (f.uv_coord[i].w == ln)
						{
							if (innerOrCorss)
							{
								mark_lines.back().push_back(trimesh::ivec2(f.uv_coord[i].x, f.uv_coord[i].y));
							}
							else
							{
								mesh->vertices[f.uv_coord[i].y].SetU(l);
								lastIndex[l] = f.uv_coord[i].y;
								if (f.uv_coord[i].x != -1)
									n++;
								else
								{
									curve[l] = true;
									innerPoint[l].push_back(f.uv_coord[i].y);
								}
								if (n == 2)
								{
									n = 0; l++;
								}
							}
						}
						else
						{
							ln = f.uv_coord[i].w;
							if (f.uv_coord[i].x == -1)
							{
								innerOrCorss = true;
								std::vector<trimesh::ivec2> line;
								mark_lines.push_back(line);
							}
							else
								innerOrCorss = false;
							i--;
						}
					}
					bool pass = false;
					n = 0;
					for (int i = 0; i < mark_lines.size(); i++)
					{
						for (int j = 0; j < mark_lines[i].size(); j++)
						{
							if (mark_lines[i][j].x != -1 && pass == false)
							{
								pass = true;
								continue;
							}
							if (pass)
							{
								mesh->vertices[mark_lines[i][j].y].SetU(l);
								lastIndex[l] = mark_lines[i][j].y;
								if (mark_lines[i][j].x != -1)
									n++;
								else
								{
									curve[l] = true;
									innerPoint[l].push_back(mark_lines[i][j].y);
								}
								if (n == 2)
								{
									n = 0; l++;
								}
							}
						}
						for (int j = 0; j < mark_lines[i].size(); j++)
						{
							mesh->vertices[mark_lines[i][j].y].SetU(l);
							lastIndex[l] = mark_lines[i][j].y;
							curve[l] = true;
							if (mark_lines[i][j].x != -1)
								break;
							innerPoint[l].push_back(mark_lines[i][j].y);
						}
						l++;
					}
				}
				std::vector<std::vector<int>> polygon;
				polygon.push_back(faceVertexSque);
				for (int u = 1; u < l; u++)
				{
					int polygon_size = polygon.size();
					for (int i = 0; i < polygon_size; i++)
					{
						int begin = -1, end = -1;
						for (int j = 0; j < polygon[i].size(); j++)
						{
							if (mesh->vertices[polygon[i][j]].IsU(u))
							{
								if (begin == -1)
									begin = j;
								else
								{
									end = j; break;
								}
							}
						}
						if (begin == -1 || end == -1) continue;
						std::vector<int> subpoly1;
						bool push1 = false;
						for (int j = begin; j <= end; j++)
						{
							subpoly1.push_back(polygon[i][j]);
							int getu = mesh->vertices[polygon[i][j]].GetU();
							if (!mesh->vertices[polygon[i][j]].IsV() && getu > u)
								push1 = true;
						}
						if (curve[u])
						{
							if (subpoly1[subpoly1.size() - 1] == lastIndex[u])
								for (int j = innerPoint[u].size() - 1; j > -1; j--)
									subpoly1.push_back(innerPoint[u][j]);
							else
								subpoly1.insert(subpoly1.end(), innerPoint[u].begin(), innerPoint[u].end());
						}
						if (push1)
						{
							polygon.push_back(subpoly1);
						}
						else {
							trimesh::point n = (trimesh::point(f.V0(1)->p.x, f.V0(1)->p.y, 0) - trimesh::point(f.V0(0)->p.x, f.V0(0)->p.y, 0)) %
								(trimesh::point(f.V0(2)->p.x, f.V0(2)->p.y, 0) - trimesh::point(f.V0(0)->p.x, f.V0(0)->p.y, 0));
							if (subpoly1.size() == 3)
							{
#ifdef _OPENMP
#pragma omp critical
#endif // _OPENMP
								{
									
									mesh->appendFace(subpoly1[0], subpoly1[1], subpoly1[2]);								
								}
							}
							else
							{
								//fillTriangle(mesh, subpoly1);										
								if (n.z <= 0)
									fillTriangleForTraverse(mesh, subpoly1, true);
								else
									fillTriangleForTraverse(mesh, subpoly1);
							}
						}

						std::vector<int> subpoly2;
						bool push2 = false;
						for (int j = end; j <= begin + polygon[i].size(); j++)
						{
							subpoly2.push_back(polygon[i][j % polygon[i].size()]);
							int getu = mesh->vertices[polygon[i][j % polygon[i].size()]].GetU();
							if (!mesh->vertices[polygon[i][j % polygon[i].size()]].IsV() && getu > u)
								push2 = true;
						}
						if (curve[u])
						{
							if (subpoly2[subpoly2.size() - 1] == lastIndex[u])
								for (int j = innerPoint[u].size() - 1; j > -1; j--)
									subpoly2.push_back(innerPoint[u][j]);
							else
								subpoly2.insert(subpoly2.end(), innerPoint[u].begin(), innerPoint[u].end());
						}
						if (push2)
						{
							polygon.push_back(subpoly2);
						}
						else {
							trimesh::point n = (trimesh::point(f.V0(1)->p.x, f.V0(1)->p.y, 0) - trimesh::point(f.V0(0)->p.x, f.V0(0)->p.y, 0)) %
								(trimesh::point(f.V0(2)->p.x, f.V0(2)->p.y, 0) - trimesh::point(f.V0(0)->p.x, f.V0(0)->p.y, 0));
							if (subpoly2.size() == 3)
							{
#ifdef _OPENMP
#pragma omp critical
#endif // _OPENMP
								{									
										mesh->appendFace(subpoly2[0], subpoly2[1], subpoly2[2]);
								}
							}
							else
							{
								//fillTriangle(mesh, subpoly2);								
								
								if (n.z <= 0)
									fillTriangleForTraverse(mesh, subpoly2, true);
								else
									fillTriangleForTraverse(mesh, subpoly2);
							}
						}

						polygon.erase(polygon.begin() + i);
						break;

					}
				}
				for (int i = 0; i < faceVertexSque.size(); i++)							
					mesh->vertices[faceVertexSque[i]].ClearV();			
				for (int i = 0; i < f.uv_coord.size(); i++)
					mesh->vertices[f.uv_coord[i].y].ClearU();
#endif
			}
		}
		std::vector<int> nextfaceid;		
		for (int i = 0; i < facesize; i++)
		{
			int fi = facesIndex[i];
			if (!mesh->faces[fi].IsD() && mesh->faces[fi].IsB())
			{
				MMeshFace& f = mesh->faces[fi];
				mesh->deleteFace(f);
				trimesh::point c = (f.V0(0)->p + f.V1(0)->p + f.V2(0)->p) / 3.0f;
#ifdef _OPENMP
#pragma omp critical
#endif // _OPENMP
				{
					mesh->appendVertex(c);
					mesh->appendFace(f.V0(0)->index, f.V1(0)->index, mesh->vertices.size() - 1);
				}
				for (int j = 0; j < f.uv_coord.size(); j++)
				{
					if (f.uv_coord[j].x == 0)
						mesh->faces.back().inner_vertex.push_back(trimesh::vec4(0, f.uv_coord[j].y, f.uv_coord[j].z, f.uv_coord[j].w));
				}
				nextfaceid.push_back(mesh->faces.size() - 1);
#ifdef _OPENMP
#endif // _OPENMP
#pragma omp critical
				mesh->appendFace(f.V1(0)->index, f.V2(0)->index, mesh->vertices.size() - 1);
				for (int j = 0; j < f.uv_coord.size(); j++)
				{
					if (f.uv_coord[j].x == 1)
						mesh->faces.back().inner_vertex.push_back(trimesh::vec4(0, f.uv_coord[j].y, f.uv_coord[j].z, f.uv_coord[j].w));
				}
				nextfaceid.push_back(mesh->faces.size() - 1);
#ifdef _OPENMP
#endif // _OPENMP
#pragma omp critical
				mesh->appendFace(f.V2(0)->index, f.V0(0)->index, mesh->vertices.size() - 1);
				for (int j = 0; j < f.uv_coord.size(); j++)
				{
					if (f.uv_coord[j].x == 2)
						mesh->faces.back().inner_vertex.push_back(trimesh::vec4(0, f.uv_coord[j].y, f.uv_coord[j].z, f.uv_coord[j].w));
				}
				nextfaceid.push_back(mesh->faces.size() - 1);		
				
			}
		}
		if (!nextfaceid.empty())
			return embedingAndCutting(mesh,lines, nextfaceid);
		
	}

	bool intersectionTriangle(MMeshT* mt, trimesh::point p, trimesh::point normal)
	{
		/*if (!mt->is_FaceNormals()) mt->getFacesNormals();*/
		for (MMeshFace& f : mt->faces)
		{
			/*float a = f.normal ^ normal;
			if (a > 0) continue;*/
			trimesh::point v01 = f.V0(1)->p - f.V0(0)->p;
			trimesh::point v02 = f.V0(2)->p - f.V0(0)->p;
			trimesh::point v0p = p - f.V0(0)->p;

			trimesh::point pe = normal % v02;
			trimesh::point pd = v0p % v01;
			float pq = 1.0 / std::abs(pe ^ v01);
			float t = pq * (pd ^ v02);
			float u = pq * (pe ^ v0p);
			float v = pq * (pd ^ normal);
			if (u > 0 && v > 0 && (u + v) < 1)
			{
				/*mt->appendVertex(trimesh::point(f.V0(0)->p + u * v01 + v * v02));
				f.SetV();*/		
				mt->deleteFace(f);
				return true;
			}
		}
		return false;
	}
	

	trimesh::point getWorldPoint(const CameraParam& camera, trimesh::ivec2 p)
	{
		trimesh::point screenCenter = camera.pos + camera.n * camera.dir;
		trimesh::point left = camera.dir % camera.up;
		trimesh::normalize(left);
		std::pair<float, float> screenhw;
		getScreenWidthAndHeight(camera, screenhw);
		float x_ratio = (float)p.x / (float)camera.ScreenSize.y - 0.5f;
		float y_ratio = (float)p.y / (float)camera.ScreenSize.x - 0.5f;
		return trimesh::point(screenCenter - camera.up * y_ratio * screenhw.first + left * x_ratio * screenhw.second);
	}

	void polygonInnerFaces(MMeshT* mt, std::vector<std::vector<std::vector<trimesh::vec2>>>& poly, std::vector<int>& infaceIndex, std::vector<int>& outfaceIndex)
	{
		for (int fi : infaceIndex)if (!mt->faces[fi].IsD())
		{
			MMeshFace& f = mt->faces[fi];
			/*double are = mt->det(fi);
			if (are < 0.00000001f)
			{
				outfaceIndex.push_back(fi);
				continue;
			}*/
			trimesh::point c = (f.V0(0)->p + f.V0(1)->p + f.V0(2)->p) / 3.0;
			int rayCorssPoint = 0;
			int equalPoint = 0;
			for (int i = 0; i < poly.size(); i++)
			{
				for (int j = 0; j < poly[i].size(); j++)
				{
					for (int k = 0; k < poly[i][j].size(); k++)
					{
						if (std::abs(poly[i][j][(k + 1) % poly[i][j].size()].y - poly[i][j][k].y) < FLOATERR) continue;
						if (c.y < std::min(poly[i][j][k].y, poly[i][j][(k + 1) % poly[i][j].size()].y)) continue;
						if (c.y > std::max(poly[i][j][k].y, poly[i][j][(k + 1) % poly[i][j].size()].y)) continue;
						double x = (c.y - poly[i][j][k].y) * (poly[i][j][(k + 1) % poly[i][j].size()].x - poly[i][j][k].x) / (poly[i][j][(k + 1) % poly[i][j].size()].y - poly[i][j][k].y) + poly[i][j][k].x;
						if (x >= c.x)
						{
							rayCorssPoint++;
							if (poly[i][j][k].y == c.y || poly[i][j][(k + 1) % poly[i][j].size()].y == c.y)
							{
								equalPoint++;
							}
						}
					}
				}
				rayCorssPoint -= (equalPoint / 2);
				if ((rayCorssPoint % 2) == 1)
				{
					outfaceIndex.push_back(f.index);
					//mt->deleteFace(f);
				}
			}
		}
	}

	void loadCameraParam(CameraParam& camera)
	{
		camera.dir = trimesh::normalized(camera.lookAt - camera.pos);
		trimesh::point undir = -1.0f * camera.dir;
		camera.right = trimesh::normalized(camera.dir % camera.up);
		trimesh::normalize(camera.up);
	}

	trimesh::TriMesh* letter(trimesh::TriMesh* mesh, const SimpleCamera& camera, const LetterParam& Letter, const std::vector<TriPolygons>& polygons, bool& letterOpState,
		LetterDebugger* debugger, ccglobal::Tracer* tracer)
	{								
		trimesh::TriMesh* newmesh = new trimesh::TriMesh();
		*newmesh = *mesh;		
		if (newmesh->faces.empty()|| polygons.empty())
		{
			letterOpState = false;
			return newmesh;
		}

		size_t process=0;			
		newmesh->need_adjacentfaces();
		if (tracer)
		{			
			process += 0.1f;
			tracer->progress(process);
		}
		newmesh->need_neighbors();
		if (tracer)
		{
			process += 0.1f;
			tracer->progress(process);
		}
		CameraParam cp;
		cp.lookAt = camera.center;
		cp.pos = camera.pos;
		cp.up = camera.up;
		cp.n = camera.n; cp.f = camera.f;
		cp.fov = camera.fov; cp.aspect = camera.aspect;

		/*cp.lookAt = trimesh::point(-1.10312, 0.172629, -0.0166779);
		cp.pos = trimesh::point(-2.05359, -0.781168, 27.2706);
		cp.up = trimesh::point(0.705012, 0.707481, 0.0492861);
		cp.n = 16.7296; cp.f = 3037.91;
		cp.fov = 16.9342; cp.aspect = 1.92965;*/

		LOGD("[Letter] camera center %f, %f, %f", camera.center.x, camera.center.y, camera.center.z);
		LOGD("[Letter] camera pos %f, %f, %f", camera.pos.x, camera.pos.y, camera.pos.z);
		LOGD("[Letter] camera up %f, %f, %f", camera.up.x, camera.up.y, camera.up.z);
		LOGD("[Letter] camera n %f,camera f %f,camera fov %f,camera aspect %f", camera.n, camera.f, camera.fov,camera.aspect);
		
		loadCameraParam(cp);
		if (tracer)
		{
			process += 0.02f;
			tracer->progress(process);
		}
		Eigen::Matrix4f viewMatrix;
		Eigen::Matrix4f projectionMatrix;
		getViewMatrixAndProjectionMatrix(cp, viewMatrix, projectionMatrix);	
		if (tracer)
		{
			process += 0.03f;
			tracer->progress(process);
		}				

		std::vector<std::vector<std::vector<trimesh::vec2>>> poly;
		poly.resize(polygons.size());		
		for (int i = 0; i < polygons.size(); i++) {
			for (int j = 0; j < polygons[i].size(); j++)
			{
				std::vector<trimesh::vec2> line;
				for (int k = 0; k < polygons[i][j].size(); k++)
				{
					if (k != polygons[i][j].size() - 1 && polygons[i][j][k] != polygons[i][j][k + 1])
					{
						line.push_back(trimesh::vec2(polygons[i][j][k].x, polygons[i][j][k].y));
					}
				}
				if (polygons[i][j][0] != polygons[i][j][polygons[i][j].size() - 1])
					line.push_back(trimesh::vec2(polygons[i][j][polygons[i][j].size() - 1].x, polygons[i][j][polygons[i][j].size() - 1].y));
				poly[i].push_back(line);
			}
		}
		if (tracer)
		{
			process += 0.02f;
			tracer->progress(process);
		}
		TransformationMesh(newmesh, viewMatrix, projectionMatrix);		
		if (tracer)
		{
			process += 0.03f;
			tracer->progress(process);
		}

		std::vector<int> faceindex;
		float threshold = 0.6f;
		getMeshFaces(newmesh, poly, cp, faceindex, threshold);
		if(tracer)
			tracer->progress(0.35);
		if (faceindex.empty())
		{
			letterOpState = false;
			return newmesh;
		}
		std::map<int, int> vmap;
		std::map<int, int> fmap;		
		MMeshT mt(newmesh,faceindex,vmap,fmap);

		if (tracer)
			tracer->progress(0.37);
		faceindex.clear();
		getDisCoverFaces(&mt, faceindex, fmap);
		
		if (tracer)
			tracer->progress(0.42);
		fmap.clear();
		vmap.clear();
		MMeshT mt2(newmesh, faceindex, vmap, fmap);	
		//MMeshT mt2(newmesh);
		mt2.set_VFadjacent(true);
		mt2.set_VVadjacent(true);
		mt2.set_FFadjacent(true);
		faceindex.clear();	
		if (tracer)
			tracer->progress(0.45);

		if (polygons.size() >= 8 && mt2.faces.size() > 800)
		{
			std::vector<std::vector<int>> faceindexs;			
			std::vector<std::vector<trimesh::vec2>> totalpoly;
			simpleCutting(&mt2, poly, faceindexs);	
#ifdef _OPENMP
#pragma omp parallel for shared(mt2) 
#endif // _OPENMP
			for (int i = 0; i < poly.size(); i++)
			{							
				embedingAndCutting(&mt2, poly[i], faceindexs[i]);						
			}
			for (int i = 0; i < poly.size(); i++)
				for (int j = 0; j < poly[i].size(); j++)				
					totalpoly.push_back(poly[i][j]);
			std::vector<int> facesindex;
			std::vector<int> outfacesIndexs;
			for (int i = 0; i < mt2.faces.size(); i++)
				facesindex.push_back(i);
			polygonInnerFaces(&mt2, poly, facesindex, outfacesIndexs);
			unTransformationMesh(&mt2, viewMatrix, projectionMatrix);
			unTransformationMesh(newmesh, viewMatrix, projectionMatrix);			
			concaveOrConvexOfFaces(&mt2, outfacesIndexs, Letter.concave, Letter.deep);
			mapping(&mt2, newmesh, vmap, fmap,true);			
		}
		else
		{
			for (int i = 0; i < mt2.faces.size(); i++)
				faceindex.push_back(i);
			std::vector<std::vector<trimesh::vec2>> totalpoly;
			for(int i=0;i<poly.size();i++)
				for (int j = 0; j < poly[i].size(); j++)
				{
					totalpoly.push_back(poly[i][j]);					
				}
			mt2.set_VFadjacent(true);
			mt2.set_VVadjacent(true);
			mt2.set_FFadjacent(true);
			embedingAndCutting(&mt2, totalpoly, faceindex);	
			if (tracer)
				tracer->progress(0.85);
			faceindex.clear();
			for (int i = 0; i < mt2.faces.size(); i++)
				faceindex.push_back(i);
			std::vector<int> outfacesIndex;
			polygonInnerFaces(&mt2, poly, faceindex, outfacesIndex);
			if (tracer)
				tracer->progress(0.9);
			unTransformationMesh(&mt2, viewMatrix, projectionMatrix);
			unTransformationMesh(newmesh, viewMatrix, projectionMatrix);
			for (int i = 0; i < mt.vertices.size(); i++)
				mt.vertices[i].ClearS();
			concaveOrConvexOfFaces(&mt2, outfacesIndex, Letter.concave, Letter.deep);
			if (tracer)
				tracer->progress(0.95);
			mapping(&mt2, newmesh, vmap, fmap);
			if (tracer)
				tracer->progress(1.0);
		}								
		/*trimesh::TriMesh* savemesh = new trimesh::TriMesh();
		mt2.mmesh2trimesh(savemesh);
		savemesh->write("savemesh.ply");
		newmesh->write("visualizationmesh.ply");*/	
		letterOpState = true;
		return newmesh;
	}

	void fillTriangle(MMeshT* mesh, std::vector<int>& vindex)
	{	
		int size = vindex.size();
		if (size == 0) return;		
		if (size == 3)
		{
#ifdef _OPENMP
#pragma omp critical
#endif // _OPENMP
			{
				mesh->appendFace(vindex[0], vindex[1], vindex[2]);			
			}
			return;
		}		
		int index = -1;
		for (int i = 0; i < size; i++)
		{			
			float b = trimesh::point((mesh->vertices[vindex[(i + 1) % size]].p - mesh->vertices[vindex[i]].p)%(mesh->vertices[vindex[(i + size - 1) % size]].p- mesh->vertices[vindex[i]].p)).z;			
			/*if (b <= 0)
				continue;*/
			float a = trimesh::normalized(trimesh::point(mesh->vertices[vindex[(i + 1) % size]].p - mesh->vertices[vindex[i]].p))^ trimesh::normalized(trimesh::point(mesh->vertices[vindex[(i + size - 1) % size]].p - mesh->vertices[vindex[i]].p));			
			if (a <=-0.9993f||b<=0)
				continue;
			bool pass = false;
			std::vector<trimesh::vec2> triangle = { trimesh::vec2(mesh->vertices[vindex[i]].p.x,mesh->vertices[vindex[i]].p.y),trimesh::vec2(mesh->vertices[vindex[(i + 1) % size]].p.x, mesh->vertices[vindex[(i + 1) % size]].p.y),
			trimesh::vec2(mesh->vertices[vindex[(i + size - 1) % size]].p.x, mesh->vertices[vindex[(i + size - 1) % size]].p.y) };
			for (int j = 0; j < size; j++)
			{
				if (j != i && j != ((i + 1) % size) && j != ((i + size - 1) % size))
				{										
					int c = 0; float e = 1; bool eq = false;
					for (int k = 0; k < triangle.size(); k++)
					{
						if (std::abs(triangle[k].y - triangle[(k + 1) % 3].y) < FLOATERR) continue;
						if (mesh->vertices[vindex[j]].p.y < std::min(triangle[k].y, triangle[(k + 1) % 3].y))continue;
						if (mesh->vertices[vindex[j]].p.y > std::max(triangle[k].y, triangle[(k + 1) % 3].y)) continue;
						if (triangle[k].y == mesh->vertices[vindex[j]].p.y || triangle[(k + 1) % 3].y == mesh->vertices[vindex[j]].p.y)
						{			
							eq = true;
							trimesh::point p1 = trimesh::point(triangle[(k + 1) % 3].x, triangle[(k + 1) % 3].y, 0) - trimesh::point(triangle[k].x, triangle[k].y, 0);
							trimesh::point p2 = mesh->vertices[vindex[j]].p - trimesh::point(triangle[k].x, triangle[k].y, 0);
							e = e * (p1 % p2).z;
						}
						double x = (mesh->vertices[vindex[j]].p.y - triangle[k].y) * (triangle[(k + 1) % 3].x - triangle[k].x) / (triangle[(k + 1) % 3].y - triangle[k].y) + triangle[k].x;
						double cha = x - mesh->vertices[vindex[j]].p.x;
						if (x- mesh->vertices[vindex[j]].p.x >= 0)
							c++;						
					}
					
					if (c == 1)
					{
						pass = true;
						break;
					}
					else if (c == 2)
					{
						if (eq)
						{
							if (e > 0)
							{
								pass = true; break;
							}
						}
					}
				}
			}
			if (pass)
				continue;
			else
			{	
#ifdef _OPENMP
#pragma omp critical
#endif // _OPENMP
				{
					mesh->appendFace(vindex[i], vindex[(i + 1) % size], vindex[(i + size - 1) % size]);				
					index = i;		
				}
				break;
			}
		}
		if (index != -1)
		{			
			vindex.erase(vindex.begin() + index);
			return fillTriangle(mesh, vindex);
		}				
	}

	void getMeshFaces(MMeshT* mesh, const std::vector<std::vector<trimesh::vec2>>& polygons, const CameraParam& camera, std::vector<int>& faces)
	{
		trimesh::vec2 topleft(1, -1);
		trimesh::vec2 botright(-1, 1);
		for(int i=0;i< polygons.size();i++)
			for (int j = 0; j < polygons[i].size(); j++)
			{
				if (polygons[i][j].x < topleft.x)
					topleft.x = polygons[i][j].x;
				if (polygons[i][j].x > botright.x)
					botright.x = polygons[i][j].x;
				if(polygons[i][j].y > topleft.y)
					topleft.y = polygons[i][j].y;
				if (polygons[i][j].y < botright.y)
					botright.y = polygons[i][j].y;
			}
		trimesh::vec2 topright(botright.x, topleft.y);
		trimesh::vec2 botleft(topleft.x, botright.y);
		std::vector<trimesh::vec2> rect={ topleft ,botright ,topright ,botleft };		
		for (MMeshFace& f : mesh->faces)if (!f.IsD())
		{
			trimesh::point v01 = f.V0(1)->p - f.V0(0)->p;
			trimesh::point v02 = f.V0(2)->p - f.V0(0)->p;
			trimesh::point normal = v01 % v02;
			float a = normal ^ camera.dir;
			if (a > 0) continue;
			bool rectInnerFace = false;
			float min_x=1.0, min_y=1.0, max_x=-1.0, max_y=-1.0;
			std::vector<trimesh::vec2> triangle;
			for (int i = 0; i < 3; i++)
			{
				trimesh::vec2 v = trimesh::vec2(f.V0(i)->p.x, f.V0(i)->p.y);
				triangle.push_back(v);
				if (v.x<botright.x && v.x>topleft.x && v.y<topleft.y && v.y>botright.y)
				{					
					faces.push_back(f.index); rectInnerFace = true; break;
				}
				if (v.x < min_x)
					min_x = v.x;
				if (v.x > max_x)
					max_x = v.x;
				if (v.y < min_y)
					min_y = v.y;
				if (v.y > max_y)
					max_y = v.y;				
			}
			if (min_x > botright.x || max_x<topleft.x || min_y>topleft.y || max_y < botright.y)
				rectInnerFace = true;

			if (rectInnerFace)
				continue;
			else
			{			
				for (int i = 0; i < rect.size(); i++)
				{
					int c = 0; float e = 1; bool eq = false;
					for (int j = 0; j < 3; j++)
					{
						if (std::abs(triangle[j].y - triangle[(j + 1) % 3].y) < FLOATERR) continue;
						if (rect[i].y < std::min(triangle[j].y, triangle[(j + 1) % 3].y))continue;
						if (rect[i].y > std::max(triangle[j].y, triangle[(j + 1) % 3].y)) continue;
						if (triangle[j].y == rect[i].y || triangle[(j + 1) % 3].y == rect[i].y)
						{
							eq = true;
							trimesh::point p1 = trimesh::point(triangle[(j + 1) % 3].x, triangle[(j + 1) % 3].y, 0) - trimesh::point(triangle[j].x, triangle[j].y, 0);
							trimesh::point p2 =trimesh::point(rect[i].x,rect[i].y,0) - trimesh::point(triangle[j].x, triangle[j].y, 0);
							e = e * (p1 % p2).z;
						}
						double x = (rect[i].y - triangle[j].y) * (triangle[(j + 1) % 3].x - triangle[j].x) / (triangle[(j + 1) % 3].y - triangle[j].y) + triangle[j].x;
						if (x > rect[i].x)
							c++;
					}
					if (c == 1)
					{			
						
						faces.push_back(f.index);
						break;
					}
					else if (c == 2)
					{
						if (eq)
						{
							if (e > 0)
							{								
								faces.push_back(f.index); break;
							}
						}
					}
				}				
			}
		}
	}

	void getMeshFaces(trimesh::TriMesh* mesh, const std::vector<std::vector<std::vector<trimesh::vec2>>>& polygons, 
		const CameraParam& camera, std::vector<int>& faces,float threshold)
	{
		trimesh::vec2 topleft(1, -1);
		trimesh::vec2 botright(-1, 1);
		for (int i = 0; i < polygons[0].size(); i++)
			for (int j = 0; j < polygons[0][i].size(); j++)
			{
				if (polygons[0][i][j].x < topleft.x)
					topleft.x = polygons[0][i][j].x;
				if (polygons[0][i][j].y > topleft.y)
					topleft.y = polygons[0][i][j].y;
			}
		topleft.x -= 0.001f; topleft.y += 0.001f;
		for(int i=0;i<polygons.back().size();i++)
			for (int j = 0; j < polygons.back()[i].size(); j++)
			{
				if (polygons.back()[i][j].x > botright.x)
					botright.x = polygons.back()[i][j].x;
				if (polygons.back()[i][j].y < botright.y)
					botright.y = polygons.back()[i][j].y;
			}
		botright.x += 0.001f; botright.y -= 0.001f;
		trimesh::vec2 topright(botright.x, topleft.y);
		trimesh::vec2 botleft(topleft.x, botright.y);
		std::vector<trimesh::vec2> rect = { topleft ,botright ,topright ,botleft };
		
		for(int fi=0;fi<mesh->faces.size();fi++)		
		{		
			trimesh::point normal =mesh->trinorm(fi);															
			if (normal.z < 0) continue;		
			trimesh::normalize(normal);
			float unangle = normal ^ trimesh::point(0, 0, 1);
			if (unangle < threshold) continue;
			bool rectInnerFace = false;
			float min_x = 1.0, min_y = 1.0, max_x = -1.0, max_y = -1.0;
			std::vector<trimesh::vec2> triangle;
			for (int i = 0; i < 3; i++)
			{
				trimesh::vec2 v = trimesh::vec2(mesh->vertices[mesh->faces[fi][i]].x, mesh->vertices[mesh->faces[fi][i]].y);
				triangle.push_back(v);
				if (v.x<botright.x && v.x>topleft.x && v.y<topleft.y && v.y>botright.y)
				{
					faces.push_back(fi); rectInnerFace = true; break;
				}
				if (v.x < min_x)
					min_x = v.x;
				if (v.x > max_x)
					max_x = v.x;
				if (v.y < min_y)
					min_y = v.y;
				if (v.y > max_y)
					max_y = v.y;
			}
			if (min_x > botright.x || max_x<topleft.x || min_y>topleft.y || max_y < botright.y)
				rectInnerFace = true;

			if (rectInnerFace)
				continue;
			else
			{
				faces.push_back(fi);
			}
		}
	}

	void mapping(MMeshT* mesh, trimesh::TriMesh* trimesh, std::map<int, int>& vmap, std::map<int, int>& fmap, bool is_thread)
	{					
		for (int i = vmap.size(); i < mesh->vertices.size(); i++)
		{
			trimesh->vertices.push_back(mesh->vertices[i].p);
			vmap[trimesh->vertices.size()-1] = i;
		}
		std::map<int, int> unvmap;
		for (auto nv : vmap)
			unvmap[nv.second] = nv.first;
		for (int i = 0; i < mesh->vertices.size(); i++)
		{
			if (mesh->vertices[i].IsS() && !mesh->vertices[i].IsV())
				trimesh->vertices[unvmap[i]] = mesh->vertices[i].p;
		}
		for (int i = fmap.size(); i < mesh->faces.size(); i++)
		{
			if (!mesh->faces[i].IsD())
			{
				trimesh->faces.push_back(trimesh::TriMesh::Face(unvmap[mesh->faces[i].connect_vertex[0]->index],
					unvmap[mesh->faces[i].connect_vertex[1]->index],
					unvmap[mesh->faces[i].connect_vertex[2]->index]));
			}
		}
		for (std::map<int, int>::reverse_iterator it = fmap.rbegin(); it != fmap.rend(); ++it)
			if (mesh->faces[it->first].IsD())
				trimesh->faces.erase(trimesh->faces.begin() + it->second);

		if (is_thread)
		{
			fillholes(trimesh);
		}
	}

	void getDisCoverFaces(MMeshT* mesh, std::vector<int>& faces, std::map<int, int>& fmap)
	{
		if (mesh->faces.empty()) return;
		float minz = std::numeric_limits<float>::max();
		int index = -1;
		for (int i = 0; i < mesh->faces.size(); i++)
		{
			float z = (mesh->faces[i].V0(0)->p.z + mesh->faces[i].V0(1)->p.z + mesh->faces[i].V0(2)->p.z)/3.0f;
			if (z < minz)
			{
				minz = z; index = i;
			}
		}
		std::queue<int> queue;
		queue.push(index);
		mesh->faces[index].SetS();
		while (!queue.empty())
		{
			for (int i = 0; i < mesh->faces[queue.front()].connect_face.size(); i++)
			{
				if (!mesh->faces[queue.front()].connect_face[i]->IsS())
				{
					queue.push(mesh->faces[queue.front()].connect_face[i]->index);
					mesh->faces[queue.front()].connect_face[i]->SetS();
				}
			}
			queue.pop();
		}
		for (int i = 0; i < mesh->faces.size(); i++)
		{
			if (mesh->faces[i].IsS())
			{
				faces.push_back(fmap[i]);
			}
			mesh->faces[i].ClearS();
		}
	}

	void simpleCutting(MMeshT* mesh, const std::vector<std::vector<std::vector<trimesh::vec2>>>& polygons, std::vector<std::vector<int>>& faceindexs)
	{
		faceindexs.resize(polygons.size());
		std::vector<float> tangent(polygons.size()-1,1);
		for (int i = 1; i < polygons.size(); i++)	
			for (int j = 0; j < polygons[i].size(); j++)			
				for (int k = 0; k < polygons[i][j].size(); k++)				
					if (tangent[i-1] > polygons[i][j][k].x)
						tangent[i-1] = polygons[i][j][k].x-0.001f;
			
		for (int i = 0; i < mesh->faces.size(); i++)
		{
			float maxx = -1.0f;
			float minx = 1.0f;
			for (int k = 0; k < 3; k++)
			{
				if (mesh->faces[i].V0(k)->p.x < minx)
					minx = mesh->faces[i].V0(k)->p.x;
				if(mesh->faces[i].V0(k)->p.x > maxx)
					maxx = mesh->faces[i].V0(k)->p.x;
			}
			for (int j = 0; j < tangent.size(); j++)
			{
				if (j == 0)
				{
					if (maxx <= tangent[j])
					{
						faceindexs[j].push_back(i); mesh->faces[i].SetS(); break;
					}
				}
				else
				{
					if (maxx<= tangent[j]&&minx>=tangent[j-1])
					{
						faceindexs[j].push_back(i);  mesh->faces[i].SetS(); break;
					}
				}
				if (j == tangent.size() - 1)
				{
					if (minx > tangent[j])
					{
						faceindexs[j + 1].push_back(i); mesh->faces[i].SetS(); break;
					}
				}				
			}
		}
		//mesh->getMeshBoundaryFaces();
		for (int i = 0; i < tangent.size(); i++)
		{
			std::vector<trimesh::ivec3> push_edge;			
			int size = mesh->faces.size();
			for (int j = 0; j < size; j++)
			{
				if (!mesh->faces[j].IsD()&&!mesh->faces[j].IsS())
				{
					MMeshFace& f = mesh->faces[j];
					std::vector<trimesh::ivec2> edge = { trimesh::ivec2(f.V0(0)->index,f.V1(0)->index),trimesh::ivec2(f.V1(0)->index,f.V2(0)->index) ,trimesh::ivec2(f.V2(0)->index,f.V0(0)->index) };
					std::vector<std::pair<float, trimesh::ivec2>> corsspoint;
					mesh->calculateCrossPoint(edge, std::make_pair(trimesh::point(tangent[i], 10000 , 0), trimesh::point(tangent[i], -10000, 0)), corsspoint);
					for (std::pair<float, trimesh::ivec2>& cp : corsspoint)
					{				
						bool pass = false;
						int index = f.getVFindex(&mesh->vertices[cp.second.x]);
						for (trimesh::ivec3& lp : push_edge)
							if (std::min(cp.second.x, cp.second.y) == lp.x && std::max(cp.second.x, cp.second.y) == lp.y)
							{
								f.uv_coord.push_back(trimesh::vec4(index, lp.z, 0, i));								
								pass = true; break;
							}
						if (pass)
							continue;
						trimesh::point d = mesh->vertices[cp.second.y].p - mesh->vertices[cp.second.x].p;				
						mesh->appendVertex(trimesh::point(mesh->vertices[cp.second.x].p + cp.first * d));												
						f.uv_coord.push_back(trimesh::vec4(index, mesh->vertices.size() - 1, 0, i));
						push_edge.push_back(trimesh::ivec3(std::min(cp.second.x, cp.second.y), std::max(cp.second.x, cp.second.y), mesh->vertices.size() - 1));
					}
					if (f.uv_coord.size() == 2)
					{						
						mesh->deleteFace(f);																	
						if (f.V1(f.uv_coord[0].x)->index == f.V0(f.uv_coord[1].x)->index)
						{
							mesh->appendFace(f.V0(f.uv_coord[0].x)->index, f.uv_coord[0].y, f.V2(f.uv_coord[0].x)->index);
							mesh->appendFace(f.uv_coord[0].y, f.V0(f.uv_coord[1].x)->index, f.uv_coord[1].y); 
							mesh->appendFace(f.uv_coord[0].y, f.uv_coord[1].y, f.V2(f.uv_coord[0].x)->index); 
						}
						else
						{
							mesh->appendFace(f.V0(f.uv_coord[1].x)->index, f.uv_coord[1].y, f.V1(f.uv_coord[0].x)->index);
							mesh->appendFace(f.V0(f.uv_coord[0].x)->index, f.uv_coord[0].y, f.uv_coord[1].y);
							mesh->appendFace(f.V1(f.uv_coord[0].x)->index, f.uv_coord[0].y, f.uv_coord[1].y); 
						}
					}
				}
			}	
			
		}
		for (int i = 0; i < mesh->faces.size(); i++)
		{			
			if (!mesh->faces[i].IsD() && !mesh->faces[i].IsS())
			{
				float c = (mesh->faces[i].V0(0)->p.x + mesh->faces[i].V0(1)->p.x + mesh->faces[i].V0(2)->p.x) / 3.0f;
				for (int j = 0; j < tangent.size(); j++)
				{
					if (j == 0)
					{
						if (c <= tangent[j])
						{
							faceindexs[j].push_back(i); break;
						}
					}
					else
					{
						if (c <= tangent[j] && c >= tangent[j - 1])
						{
							faceindexs[j].push_back(i);  break;
						}
					}
					if (j == tangent.size() - 1)
					{
						if (c > tangent[j])
						{
							faceindexs[j + 1].push_back(i); break;
						}
					}
				}
			}			
			mesh->faces[i].ClearS();
		}
	}

	void fillholes(trimesh::TriMesh* mesh)
	{
		mesh->clear_adjacentfaces();
		mesh->clear_neighbors();
		std::vector<int> boundary;
		for (int i = 0; i < mesh->vertices.size(); i++)
		{
			if (mesh->is_bdy(i))
			{
				//std::cout << " i :" << i << "\n";
				boundary.push_back(i);
			}
		}
		if (boundary.empty()) return;
		std::vector<std::vector<int>> holes;
		while (!boundary.empty())
		{
			std::queue<int> ring;
			ring.push(boundary[0]);
			std::vector<int> hole = { boundary[0] };
			while (!ring.empty())
			{
				for (int i = 0; i < mesh->neighbors[ring.front()].size(); i++)
				{
					int index = mesh->neighbors[ring.front()][i];
					if (mesh->is_bdy(index))
					{
						bool pass = false;
						for (int j = 0; j < hole.size(); j++)
						{
							if (hole[j] == index)
								pass = true;
						}
						if (pass)
							continue;
						ring.push(index); hole.push_back(index);
					}
				}
				ring.pop();
			}
			holes.push_back(hole);
			for (int i = 0; i < boundary.size(); i++)
			{
				for (int j = 0; j < hole.size(); j++)
				{
					if (boundary[i] == hole[j])
					{
						boundary.erase(boundary.begin() + i); i--;
						break;
					}
				}
			}
		}

		for (int i = 0; i < holes.size(); i++)
		{
			for (int j = 0; j < holes[i].size() - 2; j++)
			{
				mesh->faces.push_back(trimesh::TriMesh::Face(holes[i][0], holes[i][j + 1], holes[i][j + 2]));
			}
		}
	}

	void fillTriangleForTraverse(MMeshT* mesh, std::vector<int>& vindex, bool is_rollback)
	{				
		if (is_rollback)
			std::reverse(vindex.begin(), vindex.end());
		while (!vindex.empty())
		{
			int before_size = vindex.size();
			for (int i = 0; i < vindex.size(); i++)
			{
				int size = vindex.size();
				if (size == 3)
				{
					if(is_rollback)
						mesh->appendFace(vindex[0], vindex[2], vindex[1]);
					else					
						mesh->appendFace(vindex[0], vindex[1], vindex[2]);
					vindex.clear();
					break;
				}
				float b = trimesh::point((mesh->vertices[vindex[(i + 1) % size]].p - mesh->vertices[vindex[i]].p) % (mesh->vertices[vindex[(i + size - 1) % size]].p - mesh->vertices[vindex[i]].p)).z;
				float a = trimesh::normalized(trimesh::point(mesh->vertices[vindex[(i + 1) % size]].p - mesh->vertices[vindex[i]].p)) ^ trimesh::normalized(trimesh::point(mesh->vertices[vindex[(i + size - 1) % size]].p - mesh->vertices[vindex[i]].p));
				//if (a <= -1.0f || b < 0.0f)
				if(a<=-0.9999f||b<0.f)
					continue;
				std::vector<trimesh::vec2> triangle = { trimesh::vec2(mesh->vertices[vindex[i]].p.x,mesh->vertices[vindex[i]].p.y),trimesh::vec2(mesh->vertices[vindex[(i + 1) % size]].p.x, mesh->vertices[vindex[(i + 1) % size]].p.y),
				trimesh::vec2(mesh->vertices[vindex[(i + size - 1) % size]].p.x, mesh->vertices[vindex[(i + size - 1) % size]].p.y) };
				bool pass = false;
				for (int j = 0; j < size; j++)
				{
					int RightrayCorssPoint = 0; int LeftrayCrossPoint = 0;
					if (j != i && j != ((i + 1) % size) && j != ((i + size - 1) % size))
					{
						for (int k = 0; k < triangle.size(); k++)
						{
							double ty_abs = std::abs(triangle[k].y - triangle[(k + 1) % 3].y);
							double yy_abs = std::abs(triangle[k].y - mesh->vertices[vindex[j]].p.y);
							if (ty_abs < 10 * FLOATERR && yy_abs < 10 * FLOATERR&&
								mesh->vertices[vindex[j]].p.x>std::min(triangle[k].x, triangle[(k + 1) % 3].x)&&
								mesh->vertices[vindex[j]].p.x<std::max(triangle[k].x, triangle[(k + 1) % 3].x)) { pass = true; break; }
							if (std::abs(triangle[k].y - triangle[(k + 1) % 3].y) < FLOATERR) continue;
							if (mesh->vertices[vindex[j]].p.y < std::min(triangle[k].y, triangle[(k + 1) % 3].y))continue;
							if (mesh->vertices[vindex[j]].p.y > std::max(triangle[k].y, triangle[(k + 1) % 3].y)) continue;
							double x = (mesh->vertices[vindex[j]].p.y - triangle[k].y) * (triangle[(k + 1) % 3].x - triangle[k].x) / (triangle[(k + 1) % 3].y - triangle[k].y) + triangle[k].x;
							double x_abs = std::abs(x - mesh->vertices[vindex[j]].p.x);
							if (x_abs < 10*FLOATERR) { pass = true; break; }
							if (x - mesh->vertices[vindex[j]].p.x <= 0)
							{
								RightrayCorssPoint++;
							}
							else if (x - mesh->vertices[vindex[j]].p.x >= 0)
							{
								LeftrayCrossPoint++;
							}																						
						}
					}
					if (RightrayCorssPoint > 0 && LeftrayCrossPoint > 0)
					{
						pass = true; break;
					}
				}
				if (pass)
					continue;
				else {
					if(is_rollback)
						mesh->appendFace(vindex[i], vindex[(i + size - 1) % size], vindex[(i + 1) % size]);
					else					
						mesh->appendFace(vindex[i], vindex[(i + 1) % size], vindex[(i + size - 1) % size]);
					vindex.erase(vindex.begin() + i);
					i--;
				}
			}
			if (before_size == vindex.size())
			{
				//std::cout << "while :" << vindex.size() <<"\n";
				float k= mesh->vertices[vindex[0]].p.y / mesh->vertices[vindex[0]].p.x;
				std::vector<int> triangle(vindex.size(), 0);
				triangle[0] = 1;
				for (int i = 1; i < vindex.size(); i++)
				{
					if (k - (mesh->vertices[vindex[i]].p.y / mesh->vertices[vindex[i]].p.x) < 1e-7)					
						triangle[i] = 1;				
				}
				for (int i = 1; i < triangle.size()-1; i++)
				{
					if (triangle[i] == 1 && triangle[i + 1] == 1)
						mesh->appendFace(vindex[0], vindex[i], vindex[i + 1]);
				}
				break;
			}
		}
	}
}