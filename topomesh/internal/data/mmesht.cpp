#include "mmesht.h"
#include "unordered_map"

namespace topomesh
{
	MMeshT::MMeshT(trimesh::TriMesh* currentMesh)
	{		
		currentMesh->need_adjacentfaces();
		currentMesh->need_neighbors();
		currentMesh->need_across_edge();		
		rememory(currentMesh->vertices.size(), currentMesh->faces.size());
		if (currentMesh->normals.size() > 0)
			this->VertexNormals = true;
		int vn = 0;
		for (trimesh::point &apoint : currentMesh->vertices)
		{
			this->vertices.push_back(apoint);
			this->vertices.back().index = vn;
			if (this->is_VertexNormals())
			{
				this->vertices.back().normal = currentMesh->normals.at(vn);
			}
			vn++;
		}
		this->vn = vn;				
		if (currentMesh->neighbors.size() > 0)
		{
			this->VVadjacent = true;
			for (int i = 0; i < this->vn; i++)
			{
				for (int j = 0; j < currentMesh->neighbors[i].size(); j++)
					this->vertices[i].connected_vertex.push_back(&this->vertices[currentMesh->neighbors[i][j]]);
			}
		}
		
		int fn = 0;
		for (trimesh::TriMesh::Face f : currentMesh->faces)
		{
			this->faces.push_back(f);
			this->faces.back().index = fn;
			for (int i = 0; i < 3; i++)
				this->faces.back().connect_vertex.push_back(&this->vertices[f[i]]);
			fn++;
		}
		this->fn = fn;
		if (currentMesh->adjacentfaces.size() > 0)
		{
			//this->FFadjacent = true;
			this->VFadjacent = true;
			for (int i = 0; i < this->vn; i++)
			{
				for (int j = 0; j < currentMesh->adjacentfaces[i].size(); j++)
					this->vertices[i].connected_face.push_back(&this->faces[currentMesh->adjacentfaces[i][j]]);
			}
			/*for (int n = 0; n < this->faces.size(); n++)
			{
				for (int i = 0; i < 3; i++)
					for (int j = 0; j < this->faces[n].connect_vertex[i]->connected_face.size(); j++)
						this->faces[n].connect_vertex[i]->connected_face[j]->SetA();

				for (int i = 0; i < 3; i++)
					for (int j = 0; j < this->faces[n].connect_vertex[i]->connected_face.size(); j++)
						if (this->faces[n].connect_vertex[i]->connected_face[j]->IsA(2) && !this->faces[n].connect_vertex[i]->connected_face[j]->IsS())
						{
							this->faces[n].connect_vertex[i]->connected_face[j]->SetS();
							this->faces[n].connect_face.push_back(this->faces[n].connect_vertex[i]->connected_face[j]);
						}

				for (int i = 0; i < 3; i++)
					for (int j = 0; j < this->faces[n].connect_vertex[i]->connected_face.size(); j++)
					{
						this->faces[n].connect_vertex[i]->connected_face[j]->ClearA();
						this->faces[n].connect_vertex[i]->connected_face[j]->ClearS();
					}
			}*/
		}
		if (currentMesh->across_edge.size() > 0)
		{
			this->FFadjacent = true;
			for (int i = 0; i < this->fn; i++)
			{
				for (int ffi = 0; ffi < currentMesh->across_edge[i].size(); ffi++)
				{
					if(currentMesh->across_edge[i][ffi] !=-1)
						this->faces[i].connect_face.push_back(&this->faces[currentMesh->across_edge[i][ffi]]);
				}
			}
		}
	}


	MMeshT::MMeshT(trimesh::TriMesh* currentMesh, std::vector<int>& faces, std::map<int, int>& vmap, std::map<int, int>& fmap)
	{
		/*if (faces.size() < 4096)
			this->faces.reserve(8192 * 10);
		else
			this->faces.reserve((unsigned)(faces.size() * 3.5));
		if (faces.size()*3 < 4096)
			this->vertices.reserve(8192 * 10);
		else
			this->vertices.reserve((unsigned)(faces.size() * 8.5));*/
		rememory(faces.size()*2, faces.size());

		std::vector<int> vertexIndex;
		for (int i = 0; i < faces.size(); i++)
			for (int j = 0; j < 3; j++)
				vertexIndex.push_back(currentMesh->faces[faces[i]][j]);
		
		std::sort(vertexIndex.begin(), vertexIndex.end());
		for (int i = 0; i < vertexIndex.size(); i++)
		{
			if (i != 0 && vertexIndex[i] == vertexIndex[i - 1])
			{
				vertexIndex.erase(vertexIndex.begin() + i); i--;
			}
		}
		for (int i = 0; i < vertexIndex.size(); i++)
		{
			this->vertices.push_back(currentMesh->vertices[vertexIndex[i]]);
			this->vertices.back().index = i;
			vmap[vertexIndex[i]] = i;
		}
		this->vn = vertexIndex.size();
		this->faces.resize(faces.size());
		for (int i = 0; i < this->faces.size(); i++)
		{
			fmap[i] = faces[i];
			this->faces[i].index = i;
			for (int j = 0; j < 3; j++)
				this->faces[i].connect_vertex.push_back(&this->vertices[vmap[currentMesh->faces[faces[i]][j]]]);
			for (int j = 0; j < 3; j++)
				this->faces[i].connect_vertex[j]->connected_face.push_back(&this->faces[i]);			
		}	
		for (int i = 0; i < this->vertices.size(); i++)
		{
			for (MMeshFace* f : this->vertices[i].connected_face)
			{
				for (int j = 0; j < 3; j++)
				{
					if (!f->V0(j)->IsL()&&f->V0(j)->index!=i)
					{
						this->vertices[i].connected_vertex.push_back(f->V0(j)); f->V0(j)->SetL();
					}
				}				
			}
			for (MMeshVertex* v : this->vertices[i].connected_vertex)
				v->ClearL();
		}
		for (int i = 0; i < this->faces.size(); i++)
		{
			for (int j = 0; j < 3; j++)
				for (int k = 0; k < this->faces[i].connect_vertex[j]->connected_face.size(); k++)
					this->faces[i].connect_vertex[j]->connected_face[k]->SetA();

			for (int j = 0; j < 3; j++)
				for (int k = 0; k < this->faces[i].connect_vertex[j]->connected_face.size(); k++)
					if (this->faces[i].connect_vertex[j]->connected_face[k]->IsA(2) && !this->faces[i].connect_vertex[j]->connected_face[k]->IsS())
					{
						this->faces[i].connect_vertex[j]->connected_face[k]->SetS();
						this->faces[i].connect_face.push_back(this->faces[i].connect_vertex[j]->connected_face[k]);
					}
			for (int j = 0; j < 3; j++)
				for (int k = 0; k < this->faces[i].connect_vertex[j]->connected_face.size(); k++)
				{
					this->faces[i].connect_vertex[j]->connected_face[k]->ClearS();
					this->faces[i].connect_vertex[j]->connected_face[k]->ClearA();
				}
		}
	}

	MMeshT::MMeshT(MMeshT* mt, std::vector<int>& faceindex)
	{
		/*if (faceindex.size() < 4096)
		{
			this->vertices.reserve(4096 * 6);
			this->faces.reserve(4096 * 2);
		}
		else
		{
			this->vertices.reserve(faceindex.size() * 6);
			this->faces.reserve(faceindex.size() * 2);
		}*/
		rememory(faceindex.size()*3, faceindex.size());
		std::map<int, int> vv;
		for (int i = 0; i < faceindex.size(); i++)
		{
			mt->faces[faceindex[i]].SetS();
			for (int vi = 0; vi < mt->faces[faceindex[i]].connect_vertex.size(); vi++)
			{
				if (!mt->faces[faceindex[i]].V0(vi)->IsS())
				{
					this->appendVertex(mt->faces[faceindex[i]].V0(vi)->p);
					vv[mt->faces[faceindex[i]].V0(vi)->index] = this->vertices.back().index;
					mt->faces[faceindex[i]].V0(vi)->SetS();
				}
			}
		}
		std::map<int, int> ff;
		for (int i = 0; i < faceindex.size(); i++)
		{
			this->appendFace(vv[mt->faces[faceindex[i]].V0(0)->index], vv[mt->faces[faceindex[i]].V0(1)->index], vv[mt->faces[faceindex[i]].V0(2)->index]);
			ff[faceindex[i]] = this->faces.size() - 1;
		}
		if (mt->is_VVadjacent())
		{	
			this->set_VVadjacent(true);
			bool is_vf = mt->is_VFadjacent();
			if (is_vf) this->set_VFadjacent(true);
			for (int vi = 0; vi < mt->vertices.size(); vi++)
			{
				MMeshVertex& v = mt->vertices[vi];
				if (v.IsS())
				{
					for (MMeshVertex* vv_ptr : v.connected_vertex)
					{
						if (vv_ptr->IsS())
							this->vertices[vv[vi]].connected_vertex.push_back(&this->vertices[vv[vv_ptr->index]]);
					}
					if (is_vf)
					{
						for (MMeshFace* vf : v.connected_face)
						{
							if (vf->IsS())
								this->vertices[vv[vi]].connected_face.push_back(&this->faces[ff[vf->index]]);
						}
					}
				}
			}			
		}
		if (mt->is_FFadjacent())
		{
			this->set_FFadjacent(true);
			for (int i = 0; i < faceindex.size(); i++)
			{
				MMeshFace& f = mt->faces[faceindex[i]];
				for (MMeshFace* fc : f.connect_face)
				{
					if (fc->IsS())
						this->faces[ff[f.index]].connect_face.push_back(&this->faces[ff[fc->index]]);
				}
			}
		}
		
		for (int i = 0; i < faceindex.size(); i++)
		{
			mt->faces[faceindex[i]].ClearS();
			mt->faces[faceindex[i]].V0(0)->ClearS();
			mt->faces[faceindex[i]].V0(1)->ClearS();
			mt->faces[faceindex[i]].V0(2)->ClearS();
		}
	}

	MMeshT::MMeshT(const MMeshT& mt)
	{
		//std::cout << "" << "\n";
	}

	void MMeshT::rememory(int v_size,int f_size)
	{
		auto g = [](int x, int a, int b, int c)->float {
			return (float)a * std::exp(-(std::pow(x - b, 2)*1.0f / 2.0f * std::pow(c, 2)));
		};
		if (v_size < 8096 || f_size < 8086)
		{
			this->vertices.reserve(80960);
			this->faces.reserve(80960);
			return;
		}
		if (v_size < 200000 || f_size < 200000)
		{
			this->vertices.reserve(409600);
			this->faces.reserve(409600);
			return;
		}
		/*int begin_size = 8096;
		int scale = 10;
		int vu = std::pow(10, (int)std::log10(v_size));
		int fu = std::pow(10, (int)std::log10(f_size));
		float new_v_size = g(v_size, scale, begin_size, vu);
		new_v_size = new_v_size < 0.03f ? 0.03f : new_v_size;
		float new_f_size = g(f_size, scale, begin_size, fu);
		new_f_size = new_f_size < 0.03f ? 0.03f : new_f_size;
		int size_v = new_v_size * v_size;
		int szie_f = new_f_size * f_size;
		this->vertices.reserve(size_v);
		this->faces.reserve(szie_f);*/
		int size_v = 1.8 * v_size;
		int szie_f = 1.8 * f_size;
		if (f_size > 1000000)
		{
			size_v = 1.5 * v_size;
			szie_f = 1.5 * f_size;
		}
		else if (f_size > 3000000)
		{
			size_v = 1.25 * v_size;
			szie_f = 1.25 * f_size;
		}
		this->vertices.reserve(size_v);
		this->faces.reserve(szie_f);
	}



	void MMeshT::getBoundingBox()
	{
		this->bbox = true;
		for (MMeshVertex& vi : this->vertices)
		{
			if (vi.p.x < this->boundingbox.min_x)
				this->boundingbox.min_x = vi.p.x;
			if (vi.p.x > this->boundingbox.max_x)
				this->boundingbox.max_x = vi.p.x;
			if (vi.p.y < this->boundingbox.min_y)
				this->boundingbox.min_y = vi.p.y;
			if (vi.p.y > this->boundingbox.max_y)
				this->boundingbox.max_y = vi.p.y;
			if (vi.p.z < this->boundingbox.min_z)
				this->boundingbox.min_z = vi.p.z;
			if (vi.p.z > this->boundingbox.max_z)
				this->boundingbox.max_z = vi.p.z;
		}
	}

	void MMeshT::mmesh2trimesh(trimesh::TriMesh* currentMesh)
	{
		shrinkMesh();
		currentMesh->clear();
		for (int i = 0; i < this->vertices.size(); i++)
		{
			if (this->vertices[i].IsD()) continue;
			currentMesh->vertices.push_back(this->vertices[i].p);
			if(this->is_VertexNormals())
				currentMesh->normals.push_back(this->vertices[i].normal);
			if (this->is_VVadjacent())
			{
				std::vector<int> vvadj;
				for (MMeshVertex* v : this->vertices[i].connected_vertex)
					vvadj.push_back(v->index);
				currentMesh->neighbors.push_back(vvadj);
			}
			if (this->is_VFadjacent())
			{
				std::vector<int> vfadj;
				for (MMeshFace* f : this->vertices[i].connected_face)
					vfadj.push_back(f->index);
				currentMesh->adjacentfaces.push_back(vfadj);
			}
		}
		for (int i = 0; i < this->faces.size(); i++)
		{
			if (this->faces[i].IsD()) continue;
			currentMesh->faces.push_back(trimesh::TriMesh::Face(this->faces[i].connect_vertex[0]->index,
				this->faces[i].connect_vertex[1]->index,
				this->faces[i].connect_vertex[2]->index));
			if (this->is_FFadjacent())
			{
				//.....
			}
		}
	}

	void MMeshT::quickTransform(trimesh::TriMesh* currentMesh)
	{
		currentMesh->clear();
		int deleteVNum = 0;
		int deleteFNum = 0;
		for (MMeshVertex& v : this->vertices)
		{
			if (v.IsD())
			{
				deleteVNum++; continue;
			}
			else
			{
				currentMesh->vertices.emplace_back(v.p);
			}
			v.index -= deleteVNum;
		}
		for (MMeshFace& f : this->faces)
		{
			if (!f.IsD())
			{
				currentMesh->faces.emplace_back(trimesh::TriMesh::Face(f.connect_vertex[0]->index,
					f.connect_vertex[1]->index,
					f.connect_vertex[2]->index));
			}
		
		}

	}

	void MMeshT::shrinkMesh()
	{
		if (this->is_HalfEdge())
		{
			int deleteHENum = 0;
			for (MMeshHalfEdge& he : this->half_edge)
			{
				if (he.IsD())
				{
					deleteHENum++; continue;
				}
				he.index -= deleteHENum;
			}
			for (MMeshHalfEdge& he : this->half_edge)
			{
				he.next = &this->half_edge[he.next->index];
				if (he.opposite != nullptr && !he.opposite->IsD())
				{
					he.opposite = &this->half_edge[he.opposite->index];
				}
			}
		}
		int deleteVNum = 0;
		int deleteFNum = 0;
		for (MMeshVertex& v : this->vertices)
		{
			if (v.IsD())
			{
				deleteVNum++; continue;
			}
			v.index -= deleteVNum;
		}
		for (MMeshFace& f : this->faces)
		{
			if (f.IsD())
			{
				deleteFNum++; continue;
			}
			f.index -= deleteFNum;
		}
		if(this->is_VVadjacent())
			for (int i = 0; i < this->vertices.size(); i++)
			{
				for (int j = 0; j < this->vertices[i].connected_vertex.size(); j++)
					this->vertices[i].connected_vertex[j] = &this->vertices[this->vertices[i].connected_vertex[j]->index];
				for (int j = 0; j < this->vertices[i].connected_face.size(); j++)
					this->vertices[i].connected_face[j] = &this->faces[this->vertices[i].connected_face[j]->index];
				if(this->is_HalfEdge())
					for (int j = 0; j < this->vertices[i].v_mhe.size(); j++)
					{
						if (this->vertices[i].v_mhe[j]->IsD())
							this->vertices[i].v_mhe.erase(this->vertices[i].v_mhe.begin() + j);
					}
			}
		if(this->is_VFadjacent())
			for (int i = 0; i < this->faces.size(); i++)
			{
				for (int j = 0; j < this->faces[i].connect_vertex.size(); j++)
					this->faces[i].connect_vertex[j] = &this->vertices[this->faces[i].connect_vertex[j]->index];
				for (int j = 0; j < this->faces[i].connect_face.size(); j++)
					this->faces[i].connect_face[j] = &this->faces[this->faces[i].connect_face[j]->index];
				if (this->is_HalfEdge())
				{
					this->faces[i].f_mhe = &this->half_edge[this->faces[i].f_mhe->index];	
				}
			}
		for (int i = 0; i < this->vertices.size(); i++)
			if (this->vertices[i].IsD())
			{
				this->vertices.erase(this->vertices.begin() + i);
				i--;
			}
		for (int i = 0; i < this->faces.size(); i++)
			if (this->faces[i].IsD())
			{
				if (this->is_HalfEdge())
				{
					this->faces[i].f_mhe->indication_face = nullptr;
					this->faces[i].f_mhe->next->indication_face = nullptr;
					this->faces[i].f_mhe->next->next->indication_face = nullptr;
				}
				this->faces.erase(this->faces.begin() + i);
				i--;
			}			
		for (int i = 0; i < this->half_edge.size(); i++)
		{
			if (this->half_edge[i].IsD())
			{
				this->half_edge.erase(this->half_edge.begin() + i);
				i--;
			}			
		}
		if (this->is_HalfEdge())
			for (int i = 0; i < this->faces.size(); i++)
			{
				this->faces[i].f_mhe->indication_face = &this->faces[i];
				this->faces[i].f_mhe->next->indication_face = &this->faces[i];
				this->faces[i].f_mhe->next->next->indication_face = &this->faces[i];
			}
	}

	void MMeshT::getMeshBoundary()
	{
		if (!this->is_VVadjacent()) return;
		for (MMeshVertex& v : this->vertices)if(!v.IsD())
		{
			for (int i = 0; i < v.connected_face.size(); i++)
			{
				v.connected_face[i]->V1(&v)->SetA();
				v.connected_face[i]->V2(&v)->SetA();
			}
			for (int i = 0; i < v.connected_vertex.size(); i++)
			{
				if (v.connected_vertex[i]->IsA(1))
				{
					v.connected_vertex[i]->SetB();
					v.connected_vertex[i]->ClearA();
				}
			}
		}
		for (MMeshVertex& v : this->vertices)
			v.ClearA();
	}

	void MMeshT::getMeshBoundaryFaces()
	{
		if (!this->is_FFadjacent()) return;
		for (MMeshFace& f : this->faces)if(!f.IsD())
		{
			int fn = 0;
			for (int i = 0; i < f.connect_face.size(); i++)
				if (!f.connect_face[i]->IsD())
					fn++;
			if (fn != 3)
				f.SetB();
		}
	}

	void MMeshT::init_halfedge()
	{
		if (!this->is_FFadjacent()||!this->is_VFadjacent()) return;
		this->set_HalfEdge(true);
		this->half_edge.clear();
		if (this->faces.size() < 4096)
			this->half_edge.reserve(4096 * 4);
		else		
			this->half_edge.reserve(4*this->faces.size());
		for (MMeshFace& f : this->faces)
		{
			this->half_edge.push_back(MMeshHalfEdge(f.V0(0), f.V1(0)));
			this->half_edge.back().index = this->half_edge.size() - 1;
			this->half_edge.back().indication_face = &f;
			f.f_mhe=&this->half_edge[this->half_edge.size() - 1];
			f.V0(0)->v_mhe.push_back(&this->half_edge[this->half_edge.size() - 1]);
			this->half_edge.push_back(MMeshHalfEdge(f.V1(0), f.V2(0)));
			this->half_edge.back().index = this->half_edge.size() - 1;
			this->half_edge.back().indication_face = &f;
			f.V1(0)->v_mhe.push_back(&this->half_edge[this->half_edge.size() - 1]);
			this->half_edge.push_back(MMeshHalfEdge(f.V2(0), f.V0(0)));
			this->half_edge.back().index = this->half_edge.size() - 1;
			this->half_edge.back().indication_face = &f;
			f.V2(0)->v_mhe.push_back(&this->half_edge[this->half_edge.size() - 1]);

			this->half_edge[this->half_edge.size() - 3].next = &this->half_edge[this->half_edge.size() - 2];
			this->half_edge[this->half_edge.size() - 2].next = &this->half_edge[this->half_edge.size() - 1];
			this->half_edge[this->half_edge.size() - 1].next = &this->half_edge[this->half_edge.size() - 3];
		}
		std::unordered_map<unsigned long long,std::vector<int>> map;
		map.rehash(this->half_edge.size());
		int scale = std::pow(10, 1 + (int)std::log10(this->vertices.size()));
		for (int i=0;i<this->half_edge.size();i++)
		{
			MMeshHalfEdge& he = this->half_edge[i];
			int min = std::min(he.edge_vertex.first->index, he.edge_vertex.second->index);
			int max = std::max(he.edge_vertex.first->index, he.edge_vertex.second->index);
			unsigned long long key = min + max * scale;
			map[key].push_back(i);
		}
		for (auto& value : map)
		{
			if (value.second.size() == 2)
			{
				this->half_edge[value.second[0]].opposite = &this->half_edge[value.second[1]];
				this->half_edge[value.second[1]].opposite = &this->half_edge[value.second[0]];
			}
		}
	}


	void MMeshT::getEdge(std::vector<trimesh::ivec2>& edge, bool is_select)
	{
		for (MMeshVertex& v : this->vertices)
		{
			if (is_select && !v.IsS()) continue;
			for (int i = 0; i < v.connected_vertex.size(); i++)
			{
				if (v.connected_vertex[i]->IsV()) continue;
				edge.push_back(trimesh::ivec2(v.index, v.connected_vertex[i]->index));				
			}
			v.SetV();
		}
		for (MMeshVertex& v : this->vertices)
		{
			v.ClearV();
		}
	}

	void MMeshT::getEdgeNumber()
	{
		this->en = 0;
		for (MMeshVertex& v : this->vertices)if(!v.IsD())
		{
			for (int i = 0; i < v.connected_vertex.size(); i++)
			{
				if (v.connected_vertex[i]->IsD()|| v.connected_vertex[i]->IsV()) continue;
				this->en++;
			}
			v.SetV();
		}
		for (MMeshVertex& v : this->vertices)
		{
			v.ClearV();
		}
	}

	void MMeshT::getVertexSDFBlockCoord()
	{
		if (!this->is_BoundingBox()) this->getBoundingBox();
		int base = 200;
		this->span_x = (this->boundingbox.max_x - this->boundingbox.min_x) / (base * 1.f);
		this->span_y = (this->boundingbox.max_y - this->boundingbox.min_y) / (base * 1.f);
		this->span_z = (this->boundingbox.max_z - this->boundingbox.min_z) / (base * 1.f);
		for (MMeshVertex& v : this->vertices)
		{
			if (!v.inner.empty()) v.inner.clear();
			int xi = (v.p.x - this->boundingbox.min_x) / this->span_x;
			int yi = (v.p.y - this->boundingbox.min_y) / this->span_y;
			int zi = (v.p.z - this->boundingbox.min_z) / this->span_z;
			v.inner.push_back(xi);
			v.inner.push_back(yi);
			v.inner.push_back(zi);
		}
	}

	void MMeshT::calculateCrossPoint(const std::vector<trimesh::ivec2>& edge,const std::pair<trimesh::point, trimesh::point>& line, std::vector<std::pair<float, trimesh::ivec2>>& tc)
	{		
		for (trimesh::ivec2 e : edge)
		{
			trimesh::point b1 = trimesh::point(this->vertices[e.x].p.x, this->vertices[e.x].p.y, 0);
			trimesh::point b2 = trimesh::point(this->vertices[e.y].p.x, this->vertices[e.y].p.y, 0);			
			trimesh::point a = line.first - line.second;//a2->a1
			trimesh::point b = b1 - b2;//b2->b1
			trimesh::point n1 = line.first - b1;//b1->a1
			trimesh::point n2 = line.first - b2;//b2->a1
			trimesh::point n3 = line.second - b1;//b1->a2								
			if (((-a % -n2) ^ (-a % -n1)) >= 0 || ((-b % n1) ^ (-b % n3)) >= 0) continue;
			trimesh::point m = a % b;
			trimesh::point n = n1 % a;
			float t = (m.x != 0 && n.x != 0) ? n.x / m.x : (m.y != 0 && n.y != 0) ? n.y / m.y : (m.z != 0 && n.z != 0) ? n.z / m.z : -1;
			if (t > 0 && t < 1)//b1-t*b							
			{
				tc.push_back(std::make_pair(t, trimesh::ivec2(e.x, e.y)));				
			}
		}		
	}


	void MMeshT::deleteVertex(MMeshVertex& v)
	{
		if (v.IsD()) return;
		v.SetD();
		if (this->is_VFadjacent() && this->is_VVadjacent())
		{
			std::vector<MMeshFace*> deleteface = v.connected_face;
			for (int i = 0; i < deleteface.size(); i++)
				deleteFace(*deleteface[i]);
		}
#pragma omp atomic
		this->vn--;
	}

	void MMeshT::deleteVertex(int i)
	{
		deleteVertex(this->vertices[i]);
	}

	void MMeshT::deleteFace(MMeshFace& f)
	{
		if (f.IsD()) return;
		f.SetD();
		if (this->is_VFadjacent())
		{
			for (int i = 0; i < f.connect_vertex.size(); i++)
			{
				for (int j = 0; j < f.connect_vertex[i]->connected_face.size(); j++)
				{
					if (f == f.connect_vertex[i]->connected_face[j])
					{
						f.connect_vertex[i]->connected_face.erase(f.connect_vertex[i]->connected_face.begin() + j);
						break;
					}
				}
			}
		}
		if (this->is_VVadjacent())
		{
			for (MMeshVertex* v : f.connect_vertex)
				v->connected_vertex.clear();
			for (MMeshVertex* v : f.connect_vertex)
			{
				for (MMeshFace* vf : v->connected_face)
				{
					if (vf->index == f.index) continue;
					if (!vf->V1(v)->IsL())
					{
						v->connected_vertex.push_back(vf->V1(v)); vf->V1(v)->SetL();
					}
					if (!vf->V2(v)->IsL())
					{
						v->connected_vertex.push_back(vf->V2(v)); vf->V2(v)->SetL();
					}
				}
				for (MMeshFace* vf : v->connected_face)
				{
					vf->V0(0)->ClearL(); vf->V1(0)->ClearL(); vf->V2(0)->ClearL();
				}
			}

			/*for (int i = 0; i < f.connect_vertex.size(); i++)
			{
				if (f.V0(i)->IsB() && f.V1(i)->IsB())
				{
					for (int j = 0; j < f.V0(i)->connected_vertex.size(); j++)
					{
						if (f.V1(i) == f.V0(i)->connected_vertex[j])
						{
							f.V0(i)->connected_vertex.erase(f.V0(i)->connected_vertex.begin() + j);
							break;
						}
					}
				}
				if (f.V0(i)->IsB() && f.V2(i)->IsB())
				{
					for (int j = 0; j < f.V0(i)->connected_vertex.size(); j++)
					{
						if (f.V2(i) == f.V0(i)->connected_vertex[j])
						{
							f.V0(i)->connected_vertex.erase(f.V0(i)->connected_vertex.begin() + j);
							break;
						}
					}
				}
			}*/
		}
		if (this->is_FFadjacent())
		{
			for (int i = 0; i < f.connect_face.size(); i++)
			{
				for (int j = 0; j < f.connect_face[i]->connect_face.size(); j++)
				{
					if (f == f.connect_face[i]->connect_face[j])
					{
						f.connect_face[i]->connect_face.erase(f.connect_face[i]->connect_face.begin() + j);
						break;
					}
				}
			}
		}
		if (this->is_HalfEdge())
		{
			MMeshHalfEdge* halfedge = f.f_mhe;
			do
			{
				halfedge->SetD();
				halfedge = halfedge->next;
			} while (halfedge!=f.f_mhe);
		}
#pragma omp atomic
		this->fn--;
	}

	void MMeshT::deleteFace(int i)
	{
		deleteFace(this->faces[i]);
	}

	void MMeshT::appendVertex(const MMeshVertex& v)
	{
		this->vertices.push_back(v);
		this->vertices.back().index = this->vertices.size()-1;
		this->vn++;
	}

	void MMeshT::appendVertex(const trimesh::point& v)
	{
		this->vertices.push_back(v);
		this->vertices.back().index = this->vertices.size()-1;
		this->vn++;
	}

	void MMeshT::appendFace(MMeshVertex& v0, MMeshVertex& v1, MMeshVertex& v2)
	{				
		bool invert = false;		
		this->faces.push_back(MMeshFace(&this->vertices[v0.index], &this->vertices[v1.index], &this->vertices[v2.index]));		
		this->faces.back().index = this->faces.size()-1;	
		
		if (this->is_FFadjacent())
		{
			if (this->is_HalfEdge())
			{
				this->half_edge.push_back(MMeshHalfEdge(this->faces.back().V0(0), this->faces.back().V1(0)));
				this->half_edge.back().indication_face = &this->faces.back();
				this->half_edge.back().index = this->half_edge.size() - 1;
				this->faces.back().f_mhe = &this->half_edge.back();
				this->half_edge.push_back(MMeshHalfEdge(this->faces.back().V1(0), this->faces.back().V2(0)));
				this->half_edge.back().indication_face = &this->faces.back();
				this->half_edge.back().index = this->half_edge.size() - 1;
				this->half_edge.push_back(MMeshHalfEdge(this->faces.back().V2(0), this->faces.back().V0(0)));
				this->half_edge.back().indication_face = &this->faces.back();
				this->half_edge.back().index = this->half_edge.size() - 1;

				this->half_edge[this->half_edge.size() - 3].next = &this->half_edge[this->half_edge.size() - 2];
				this->half_edge[this->half_edge.size() - 2].next = &this->half_edge[this->half_edge.size() - 1];
				this->half_edge[this->half_edge.size() - 1].next = &this->half_edge[this->half_edge.size() - 3];
			}
			for (int i = 0; i < v0.connected_face.size(); i++)
				v0.connected_face[i]->SetA();
			for (int i = 0; i < v1.connected_face.size(); i++)
				v1.connected_face[i]->SetA();
			for (int i = 0; i < v2.connected_face.size(); i++)
				v2.connected_face[i]->SetA();
			//---�����ظ�����connect_face			
			for (int i = 0; i < v0.connected_face.size(); i++)
				if (v0.connected_face[i]->IsA(2) && !v0.connected_face[i]->IsL())
				{
					v0.connected_face[i]->SetL();
					this->faces.back().connect_face.push_back(v0.connected_face[i]);
					v0.connected_face[i]->connect_face.push_back(&this->faces.back());
					if (v0.connected_face[i]->V1(&v0) == &v1 || v0.connected_face[i]->V2(&v0) == &v2)
						invert = true;					
				}
			
			for (int i = 0; i < v1.connected_face.size(); i++)
				if (v1.connected_face[i]->IsA(2) && !v1.connected_face[i]->IsL())
				{
					v1.connected_face[i]->SetL();
					this->faces.back().connect_face.push_back(v1.connected_face[i]);
					v1.connected_face[i]->connect_face.push_back(&this->faces.back());
					if (v1.connected_face[i]->V1(&v1) == &v2 && v1.connected_face[i]->V2(&v1) == &v0)
						invert = true;				
				}
			
			for (int i = 0; i < v2.connected_face.size(); i++)
				if (v2.connected_face[i]->IsA(2) && !v2.connected_face[i]->IsL())
				{
					v2.connected_face[i]->SetL();
					this->faces.back().connect_face.push_back(v2.connected_face[i]);
					v2.connected_face[i]->connect_face.push_back(&this->faces.back());
					if (v2.connected_face[i]->V1(&v2) != &v0 && v2.connected_face[i]->V2(&v2) != &v1)
						invert = true;
					
				}
			for (int i = 0; i < v0.connected_face.size(); i++) {
				v0.connected_face[i]->ClearL(); v0.connected_face[i]->ClearA();
			}
			for (int i = 0; i < v1.connected_face.size(); i++) {
				v1.connected_face[i]->ClearL(); v1.connected_face[i]->ClearA();
			}
			for (int i = 0; i < v2.connected_face.size(); i++) {
				v2.connected_face[i]->ClearL(); v2.connected_face[i]->ClearA();
			}
		}
		
		if (this->is_VFadjacent())
		{
			v0.connected_face.push_back(&this->faces.back());
			v1.connected_face.push_back(&this->faces.back());
			v2.connected_face.push_back(&this->faces.back());
		}
		
		if (this->is_VVadjacent())
		{
			bool edge1 = false, edge2 = false, edge3 = false;
			for (int i = 0; i < v0.connected_vertex.size(); i++)
			{
				if (v0.connected_vertex[i] == &v1)
					edge1 = true;
				if (v0.connected_vertex[i] == &v2)
					edge2 = true;
			}
			for (int i = 0; i < v1.connected_vertex.size(); i++)
				if (v1.connected_vertex[i] == &v2)
					edge3 = true;
			if (!edge1)
			{
				v0.connected_vertex.push_back(&v1); v1.connected_vertex.push_back(&v0);
			}
			if (!edge2)
			{
				v0.connected_vertex.push_back(&v2); v2.connected_vertex.push_back(&v0);
			}
			if (!edge3)
			{
				v1.connected_vertex.push_back(&v2); v2.connected_vertex.push_back(&v1);
			}
		}		
		if (invert)
			std::swap(this->faces.back().connect_vertex[1], this->faces.back().connect_vertex[2]);

		if (this->is_HalfEdge())
		{
			MMeshHalfEdge* he = this->faces.back().f_mhe;
			do
			{
				he->edge_vertex.first->SetL(); he->edge_vertex.second->SetL();
				for (MMeshFace* ff : this->faces.back().connect_face)
				{
					MMeshHalfEdge* ffhe = ff->f_mhe;
					bool pass = false;
					do
					{
						if (ffhe->edge_vertex.first->IsL() && ffhe->edge_vertex.second->IsL())
						{
							ffhe->opposite = he;
							he->opposite = ffhe;
							pass = true;
							break;
						}
						ffhe = ffhe->next;
					} while (ffhe!=ff->f_mhe);
					if (pass)
					{
						break;
					}
				}
				he->edge_vertex.first->ClearL(); he->edge_vertex.second->ClearL();
				he = he->next;
			} while (he!=this->faces.back().f_mhe);
		}
		
		this->fn++;
	}

	void MMeshT::appendFace(int i0, int i1, int i2)
	{
		appendFace(this->vertices[i0], this->vertices[i1], this->vertices[i2]);
	}

	double  getTotalArea(std::vector<trimesh::point>& inVertices)
	{
		double sum = 0.0f;
		for (int n = 1; n < inVertices.size() - 1; n++)
		{
			sum += MMeshT::det(inVertices[0], inVertices[n], inVertices[n + 1]);
		}
		return abs(sum);
	}

	void MMeshT::getFacesNormals()
	{
		for (MMeshFace& f : this->faces)
		{
			trimesh::point ave_normal;
			/*ave_normal += f.V0(0)->normal;
			ave_normal += f.V0(1)->normal;
			ave_normal += f.V0(2)->normal;
			ave_normal /= 3.0f;*/
			ave_normal = trimesh::normalized((f.V1(0)->p - f.V0(0)->p) % (f.V2(0)->p - f.V0(0)->p));
			f.normal = ave_normal;
		}
		this->FacesNormals = true;
	}
			
}