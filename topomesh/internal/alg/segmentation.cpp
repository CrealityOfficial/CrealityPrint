#include "segmentation.h"
#include "cmath"
#include "floyd.h"
#include "dijkstra.h"
#include "internal/clustering/k-means.h"
#include "laplacian.h"
#include "utils.h"
#include "queue"
#include "set"
#pragma once
#include "Eigen/Dense"
#include "Eigen/Sparse"
#include<Eigen/Eigenvalues>

struct Patch
{
	Patch() {};
	Patch(std::vector<int>& faces) { inner = faces; }
	int index;
	bool select=false;
	std::vector<int> inner;//对于基本块
	trimesh::point normal;
	std::set<int> neighbor;//对于本层块
};


namespace topomesh {
	SpectralClusteringCuts::SpectralClusteringCuts(MMeshT* mesh, float delte, float eta):_delta(delte),_eta(eta)
	{		
		if (!mesh->is_FaceNormals()) mesh->getFacesNormals();
		if (!mesh->is_HalfEdge()) mesh->init_halfedge();
		float ave_geod_dist;
		float ave_ang_dist;
		int edge = 0;
		for (MMeshFace& f : mesh->faces)
		{
			trimesh::point c = (f.V0(0)->p + f.V1(0)->p + f.V2(0)->p)/3.f;		
			MMeshHalfEdge* halfedge_ptr = f.f_mhe;
			do
			{
				if (halfedge_ptr->IsS())
				{
					halfedge_ptr = halfedge_ptr->next;
					continue;
				}
				if (halfedge_ptr->opposite == nullptr || halfedge_ptr->opposite->IsD())
				{
					halfedge_ptr->SetS();
					halfedge_ptr = halfedge_ptr->next;
					continue;
				}
				trimesh::point m = (halfedge_ptr->edge_vertex.first->p + halfedge_ptr->edge_vertex.second->p) / 2.f;
				trimesh::point cf = (halfedge_ptr->opposite->indication_face->V0(0)->p + halfedge_ptr->opposite->indication_face->V1(0)->p +
					halfedge_ptr->opposite->indication_face->V2(0)->p) / 3.f;
				float dist = trimesh::dist(cf, m) + trimesh::dist(c, m);
				float angle = f.dihedral(halfedge_ptr->opposite->indication_face);
				float alph = angle > M_PI/2.0 ? _eta : 1.0f;
				float ang_dist = alph * (1.f - std::cos(angle));
				edge++;
				ave_geod_dist += dist;
				ave_ang_dist += ang_dist;
				halfedge_ptr->attritube_vec.push_back(dist);
				halfedge_ptr->attritube_vec.push_back(ang_dist);
				halfedge_ptr->opposite->attritube_vec.push_back(dist);
				halfedge_ptr->opposite->attritube_vec.push_back(ang_dist);
				halfedge_ptr->SetS();
				halfedge_ptr->opposite->SetS();
				halfedge_ptr = halfedge_ptr->next;
			} while (halfedge_ptr!=f.f_mhe);			
		}

		ave_geod_dist = ave_geod_dist*1.f / edge * 1.f;
		ave_ang_dist = ave_ang_dist * 1.f / edge * 1.f;
		for (MMeshHalfEdge& he : mesh->half_edge)
			he.ClearS();		
		Eigen::SparseMatrix<float> D(mesh->faces.size(),mesh->faces.size());
		typedef Eigen::Triplet<float> Tr;
		std::vector<Tr> D_triplet;
		for (MMeshHalfEdge& he : mesh->half_edge)
		{
			if (!he.IsS()&&he.attritube_vec.size()==2)
			{
				/*std::cout << "face : " << he.indication_face->index << "to face : " << he.opposite->indication_face->index 
					<< " distance : " << he.attritube_vec[0] << " angle : " << he.attritube_vec[1] << "\n";*/
				/*float value = _delta * (he.attritube_vec[0] * 1.f / (ave_geod_dist * 1.f))
					+ (1.f - _delta) * (he.attritube_vec[1] * 1.f / (ave_ang_dist * 1.f));*/
				float value = he.attritube_vec[1]*50.f;
				D_triplet.push_back(Tr(he.indication_face->index, he.opposite->indication_face->index, value));
				he.SetS();
			}
		}
		D.setFromTriplets(D_triplet.begin(),D_triplet.end());
		//std::cout << "D : \n" << D << "\n";
		/*topomesh::Dijkstra<Eigen::SparseMatrix<float>> dijk(D);
		Eigen::SparseMatrix<float>  r = dijk.get_result();*/
		/*topomesh::floyd<Eigen::SparseMatrix<float>> floyd(D);
		Eigen::MatrixXf result = floyd.get_result();*/
		//------- ����̫�󣬼���ʱ�䳤
	/*	float sigma = D.sum() * 1.f / (1.f*std::pow(edge,2));
		Eigen::SparseMatrix<float> W(D.rows(), D.cols());
		std::vector<Tr> W_triplet;
		std::vector<Tr> diag_triplet;
		for (int k = 0; k < D.outerSize(); ++k)
			for (Eigen::SparseMatrix<float>::InnerIterator it(D, k); it; ++it)
			{
				float value = it.value();
				std::cout << "before value : " << value << "\n";
				value = std::exp((-1.f * value) / (2.f * std::pow(sigma, 2)));
				std::cout << "value : " << value <<" row :"<<it.row()<<" col :"<< it.col() << "\n";
				W_triplet.push_back(Tr(it.row(), it.col(), value));
			}

		W.setFromTriplets(W_triplet.begin(), W_triplet.end());		
		std::cout << "W : \n" << W << "\n";*/
		Eigen::SparseMatrix<float> LD(D.rows(), D.cols());
		Eigen::VectorXf rowsum(D.rows());
		rowsum.setZero();
		std::vector<Tr> LD_triplet;
		for (int i = 0; i < D.outerSize(); i++)
		{
			for (Eigen::SparseMatrix<float>::InnerIterator it(D, i); it; ++it) {
				rowsum(it.row()) += it.value();
				/*std::cout << "value : " << it.value() << " row :" << it.row() << " col :" << it.col() << "\n";
				std::cout << "rowsum : " << rowsum(it.row()) << "\n";*/
			}
		}
		for (int i = 0; i < rowsum.size(); i++)
			LD_triplet.push_back(Tr(i,i,rowsum(i)));
		LD.setFromTriplets(LD_triplet.begin(), LD_triplet.end());
		//std::cout << "LD : \n" << LD << "\n";
		Eigen::SparseMatrix<float> L(D.rows(), D.cols());
		L = LD - D;
		//L = ((((W*LD).pruned()).transpose()*LD).pruned()).transpose();
		
		Eigen::SelfAdjointEigenSolver<Eigen::SparseMatrix<float>> es(L);
		//std::cout << "L : \n" << L << "\n";
		//std::cout << "value : \n" << es.eigenvalues() << "\n";
		//std::cout<<"vec : \n"<<es.eigenvectors()<<"\n";
		//std::cout << "block : \n" << es.eigenvectors().block(0,1,L.rows(),3)<<"\n";
		Eigen::MatrixXf block = es.eigenvectors().block(0, 1, L.rows(), 4);
		result.resize(16);
		for (int i = 0; i < block.rows(); i++)
		{
			int index = 0;
			for (int j = 0; j < block.cols(); j++)
			{
				if (block(i, j) > 0)
					index += std::pow(2, j);
			}
			result[index].push_back(i);
		}
		for (int i = 0; i < result.size(); i++)
		{
			if (result[i].empty())
			{
				result.erase(result.begin() + i); i--;
			}
		}
		/*for (int i = 0; i < result.size(); i++)
		{
			for (int j = 0; j < result[i].size(); j++)
				std::cout << "i : " << i << " j : " << result[i][j] << "\n";
		}*/

		/*topomesh::kmeansClustering<Eigen::MatrixXf> kmeans(block, 9);
		this->result.resize(kmeans.get_result()->size());
		for (int i = 0; i < kmeans.get_result()->size(); i++)
		{
			for (int j = 0; j < kmeans.get_result()->at(i).size(); j++)
			{
				std::cout << "i : " << i << " j : " << kmeans.get_result()->at(i).at(j) << "\n";
				this->result[i].push_back(kmeans.get_result()->at(i).at(j));
			}
		}*/
		
	}
	
	void SpectralClusteringCuts::BlockSpectralClusteringCuts(MMeshT* mesh)
	{				
		if (!mesh->is_FaceNormals()) mesh->getFacesNormals();
		//if (!mesh->is_HalfEdge()) mesh->init_halfedge();
#if 1				
		std::vector<std::vector<int>> container;
		int index = 1;
		int threshold = mesh->faces.size() / 1000;		
		std::vector<int> other_faces;
		other_faces.reserve(mesh->faces.size());
		std::vector<float> faces_area(mesh->faces.size());
		float surface_area = 0.f;
		for (MMeshFace& f : mesh->faces)
		{					
			/*float area=mesh->det(f.index);
			surface_area += area;
			faces_area[f.index] = area;*/
			if (f.IsV()) continue;
			std::vector<int> patch;
			findNeighborFacesOfConsecutive(mesh,f.index, patch,30.f,true);	
			for (int i : patch)
			{
				mesh->faces[i].SetV();
			}
			//if(false)
			//if (patch.size() < threshold)
			//{								
			//	for (int i : patch)
			//	{
			//		mesh->faces[i].SetV();					
			//		//other_faces.emplace_back(i);					
			//	}	
			//	container.push_back(patch);
			//	//other_faces.insert(other_faces.end(), patch.begin(), patch.end());			
			//}
			//else
			//{
			//	for (int i : patch)
			//	{
			//		mesh->faces[i].SetV();
			//		mesh->faces[i].SetU(index);	
			//	}
			//	index++;
			//}		
			container.push_back(patch);
			//result.push_back(patch);
		}		
		//mesh->getVertexSDFBlockCoord();
	

		result.swap(container);

		return;
		float ave_area = surface_area / 1000.f;
		for (int i = 0; i < container.size(); i++)
		{
			float block_area = 0.0f;
			for (int j = 0; j < container[i].size(); j++)
			{
				block_area += faces_area[container[i][j]];
			}
			if (block_area > ave_area)
			{
				for (int j = 0; j < container[i].size(); j++)
					mesh->faces[container[i][j]].SetU(index);
				index++;
			}
			else
			{
				other_faces.insert(other_faces.end(), container[i].begin(), container[i].end());
			}
		}

		std::vector<int> mark(other_faces.size(),0);
		int mark_all = 0;
		int remark = 0;
		while (mark_all<other_faces.size())
		{
			for (int i = 0; i < other_faces.size(); i++)
			{
				mesh->faces[other_faces[i]].ClearV();
				if (mark[i]) continue;
				for (MMeshFace* f : mesh->faces[other_faces[i]].connect_face)
				{
					if (f->GetU() > 0)
					{
						float ang = trimesh::normalized(mesh->faces[other_faces[i]].normal) ^ trimesh::normalized(f->normal);
						ang = ang >= 1.f ? 1.f : ang;
						ang = ang <= -1.f ? -1.f : ang;
						float arc = (std::acos(ang) * 180 / M_PI);
						if (arc<10.f+remark*2.5f)
						{
							mesh->faces[other_faces[i]].SetU(f->GetU());
							mark_all++;
							break;
						}
					}
				}
			}
			remark++;
		}
		for (MMeshFace& f : mesh->faces)
		{
			if (f.GetU() == 0&&!f.IsV())
			{
				std::vector<int> patch;
				findNeighborFacesOfConsecutive(mesh, f.index, patch, 30.f, true);
				if (patch.size() > threshold)
				{
					for (int i : patch)
					{
						mesh->faces[i].SetV();
						mesh->faces[i].SetU(index);
					}
					index++;
				}
			}
		}		
		for (MMeshFace& f : mesh->faces)
		{
			if (f.GetU() == 0 && !f.IsV())
				for (MMeshFace* ff : f.connect_face)
					if(ff->GetU()!=0)
						f.SetU(ff->GetU());
		}
		container.clear();
		container.resize(index - 1);
		//result.resize(index-1);	
		for (MMeshFace& f : mesh->faces)
		{			
			if (f.GetU() != 0)
			{//result[f.GetU()-1].push_back(f.index);
				container[f.GetU() - 1].push_back(f.index);
			}
		}
		//return;
		std::vector<std::set<int>> patch_neighbor(container.size());
		std::vector<trimesh::point> patch_normal(container.size());
		for (int i = 0; i < container.size(); i++)
		{
			trimesh::point normal(0,0,0);
			for (int j = 0; j < container[i].size(); j++)
			{
				normal += mesh->faces[container[i][j]].normal;
				for(MMeshFace* f :mesh->faces[container[i][j]].connect_face)
					if (f->GetU() != i + 1&&f->GetU()!=0)
					{
						patch_neighbor[i].emplace(f->GetU() - 1);
					}
			}
			normal /= (container[i].size()*1.f);
			patch_normal[i] = normal;
		}
		std::vector<std::vector<std::pair<int, float>>> data;
		for (int i = 0; i < patch_neighbor.size(); i++)
		{
			std::vector<std::pair<int, float>> node;
			for (std::set<int>::iterator itr = patch_neighbor[i].begin(); itr != patch_neighbor[i].end(); itr++)
			{
				float ang = trimesh::normalized(patch_normal[i]) ^ trimesh::normalized(patch_normal[*itr]);
				ang = ang >= 1.f ? 1.f : ang;
				ang = ang <= -1.f ? -1.f : ang;
				//float arc = (std::acos(ang) * 180 / M_PI);
				float arc = 100.f * (1.f - std::cos(ang));
				node.push_back(std::make_pair(*itr, arc));
			}
			data.push_back(node);
		}
		int indica = mesh->faces[304832].GetU() - 1;
		LaplacianMatrix lapMatrix(data);
		lapMatrix.EigenvectorSolver();
		
		std::vector<int> data_result;
		lapMatrix.selectEigenVector(indica, data_result);

		result.resize(1);
		if (!data_result.empty())
		{
			for (int i = 0; i < data_result.size(); i++)
				result[0].insert(result[0].end(), container[data_result[i]].begin(), container[data_result[i]].end());
		}
		return;
		
		std::vector<Patch> Patchcontainer;
		for (int i = 0; i < container.size(); i++)
		{						
			Patch ph;
			ph.index = i;
			ph.inner.push_back(i);
			trimesh::point normal(0, 0, 0);
			for (int j = 0; j < container[i].size(); j++)
			{
				MMeshFace& f = mesh->faces[container[i][j]];
				normal += f.normal;
				for (MMeshFace* ff : f.connect_face)
				{
					if (ff->GetU() != i)
						ph.neighbor.emplace(ff->GetU());
				}
			}
			normal /= (container[i].size() * 1.f);
			ph.normal = normal;
			Patchcontainer.push_back(ph);
		}

		while (Patchcontainer.size()>5000)
		{
			std::vector<Patch> new_patchcontainer;
			int index = 0;
			for (int i = 0; i < Patchcontainer.size(); i++)
			{
				Patch& ph = Patchcontainer[i];
				if (ph.select) continue;
				ph.select = true;
				Patch np;
				np.index = index++;
				for (int j = 0; j < ph.inner.size(); j++)
					np.inner.push_back(ph.inner[j]);
				trimesh::point normal=ph.normal;
				int is_neig = 1;
				for (std::set<int>::iterator itr = ph.neighbor.begin(); itr != ph.neighbor.end(); itr++)
				{
					float ang = trimesh::normalized(ph.normal)^trimesh::normalized(Patchcontainer[*itr].normal);
					ang = ang >= 1.f ? 1.f : ang;
					ang = ang <= -1.f ? -1.f : ang;
					if ((std::acos(ang) * 180 / M_PI) < (10.f))
					{
						Patchcontainer[*itr].select = true;
						normal += Patchcontainer[*itr].normal;
						is_neig++;
						for (int j = 0; j < Patchcontainer[*itr].inner.size(); j++)
						{
							np.inner.push_back(Patchcontainer[*itr].inner[j]);
						}
					}
				}
				normal /= (is_neig * 1.f);
				np.normal = normal;
				new_patchcontainer.push_back(np);
			}

		}
		std::vector<std::set<unsigned int>> block_neighbor(container.size());
		std::vector<trimesh::point> block_ave_normal(container.size());
		for (int i = 0; i < container.size(); i++)
		{
			trimesh::point ave_normal(0, 0, 0);
			for (int j = 0; j < container[i].size(); j++)
			{
				MMeshFace& f = mesh->faces[container[i][j]];
				ave_normal += f.normal;
				for (MMeshFace* ff : f.connect_face)
				{
					unsigned int user = (unsigned int)ff->GetU();
					if (user != i)
					{
						block_neighbor[i].emplace(user);
					}
				}
			}
			ave_normal = (ave_normal * 1.f) / (container[i].size() * 1.f);
			block_ave_normal[i] = ave_normal;
		}		
	
		int scale = 1;
		while (container.size() > 5000)
		{
			scale++;
			std::vector<trimesh::point> block_normal(container.size());
			for (int i = 0; i < container.size(); i++)
			{
				trimesh::point ave_normal(0, 0, 0);				
				for (int j = 0; j < container[i].size(); j++)
				{					
					ave_normal += block_ave_normal[container[i][j]];
				}
				ave_normal /= (container[i].size() * 1.f);
				block_normal[i] = ave_normal;
			}
			std::vector<std::vector<int>> temp_container;
			std::vector<int> is_pass(container.size(), 0);
			for (int i = 0; i < container.size(); i++)
			{
				if (is_pass[i]) continue;

			}


			for (int i = 0; i < container.size(); i++)
			{
				if (is_pass[i]) continue;
				is_pass[i] = 1;
				std::vector<int> block = { i };
				trimesh::point this_normal = block_ave_normal[i];
				for (std::set<unsigned int>::iterator itr = block_neighbor[i].begin(); itr != block_neighbor[i].end(); itr++)
				{
					trimesh::point neighbor_normal = block_ave_normal[*itr];
					float ang = trimesh::normalized(this_normal) ^ trimesh::normalized(neighbor_normal);
					ang = ang >= 1.f ? 1.f : ang;
					ang = ang <= -1.f ? -1.f : ang;
					if ((std::acos(ang) * 180 / M_PI) < (8.f+ scale*2.0f))
					{
						block.push_back(*itr);
						is_pass[*itr] = 1;
					}
				}
				std::vector<int> new_block(container[i]);
				for (int j = 0; j < block.size(); j++)
				{
					std::move(container[block[j]].begin(), container[block[j]].end(), std::back_inserter(new_block));
				}
				container.push_back(new_block);
			}
			container.swap(container);
		}
		result.swap(container);
		/*for (int i = 0; i < container.size(); i++)
			if (container[i].size() == 1)
				mesh->faces[container[i][0]].SetM();
		for (int i = 0; i < container.size(); i++)
			if (container[i].size() == 1)
				for (MMeshFace* ff : mesh->faces[container[i][0]].connect_face)
					if (!ff->IsM())
						mesh->faces[container[i][0]].SetU((unsigned int)ff->GetU());
		result.resize(make_user--);
		for (MMeshFace& f : mesh->faces)
		{			
			result[(unsigned int)f.GetU()].push_back(f.index);
			f.ClearALL();
		}
		for(int i=0;i< result.size();i++)
			if (result[i].empty())
			{
				result.erase(result.begin() + i); i--;
			}*/
#else
		for (topomesh::MMeshHalfEdge& he : mesh->half_edge)
		{
			if (he.IsS() || he.opposite==nullptr) continue;
			he.SetS(); he.opposite->SetS();
			float angle = he.indication_face->dihedral(he.opposite->indication_face);
			if (angle < 170.f)
			{
				he.indication_face->SetA();
				he.opposite->indication_face->SetA();
			}
		}
		std::vector<int> faceindex;
		for (topomesh::MMeshFace& f : mesh->faces)
		{
			int a = f.getA();
			if (a>0)
				faceindex.push_back(f.index);
		}
		result.push_back(faceindex);
#endif // 0
	}

	void SpectralClusteringCuts::test()
	{
		Eigen::MatrixXf L(12,3);
		L << -0.291691, -0.173686, 0.367083,
			-0.286053, -0.175397, 0.370689,
			-0.291229, 0.40508, -0.0331007,
			-0.285586, 0.409053, -0.0334231,
			-0.291525, -0.230866, -0.334235,
			-0.285886, -0.233138, -0.337515,
			0.291694, 0.173695, -0.367075,
			0.286056, 0.175406, -0.370682,
			0.291523, 0.230861, 0.334242,
			0.285884, 0.233132, 0.337521,
			0.291226, -0.405083, 0.0330848,
			0.285583, -0.409055, 0.0334072;
		topomesh::kmeansClustering<Eigen::MatrixXf> kmean(L, 6);
		for (int i = 0; i < kmean.get_result()->size(); i++)
		{
			for (int j = 0; j < kmean.get_result()->at(i).size(); j++)
			{
				std::cout << "i : " << i << " j : " << kmean.get_result()->at(i).at(j) << "\n";
			}
		}
		/*Eigen::SelfAdjointEigenSolver<Eigen::MatrixXf> es(L);
		std::cout << "v : \n" << es.eigenvalues() << "\n";
		std::cout << "vec : \n" << es.eigenvectors() << "\n";*/
	}

	
	InteractionCuts::InteractionCuts(MMeshT* mesh, std::vector<int> indication, float alpth, float delta) :_alpth(alpth), _delta(delta)
	{
		if (!mesh->is_FaceNormals()) mesh->getFacesNormals();
		for (int i = 0; i < indication.size(); i++)
		{
			std::vector<int> container = { indication[i] };
			std::vector<int> next_container;
			int score = 18000;
			mesh->faces[container[0]].SetU(score);
			while (!container.empty())
			{
				for (int j = 0; j < container.size(); j++)
				{
					MMeshFace& f = mesh->faces[container[j]];
					f.SetS();
					for (MMeshFace* ff : f.connect_face)if(!ff->IsS())
					{
						ff->SetS();
						float angle = f.dihedral(ff);
						int new_score = f.GetU()-10;
						if (angle >= 180)
							new_score = new_score - _alpth*(angle - 180.f)*1.f/180.f ;
						else
							new_score = new_score - _delta * (angle - 180.f) * 1.f / 180.f;
						ff->SetU(new_score);
						next_container.push_back(ff->index);
					}
				}
				container.clear();
				container.swap(next_container);
			}
			for (MMeshFace& f : mesh->faces)
				f.ClearS();
		}
		for (MMeshFace& f : mesh->faces)
		{
			if (f.GetU() > 8000)
				result.push_back(f.index);
		}
	}

	SegmentationGroup::SegmentationGroup(int id, ManualSegmentation* segmentation)
			: m_segmentation(segmentation)
			, m_id(id)
	{
		
	}	


	/*Segmentation::Segmentation(trimesh::TriMesh* mesh)
	{

	}*/

	SegmentationGroup::~SegmentationGroup()
	{

	}

	bool SegmentationGroup::addSeed(int index)
	{
		std::vector<int> indices;
		indices.push_back(index);

		return addSeeds(indices);
	}

	bool SegmentationGroup::addSeeds(const std::vector<int>& indices)
	{
		for (int index : indices)
		{
			assert(m_segmentation->checkValid(index)
				&& m_segmentation->checkEmpty(index));
		}

		m_segmentation->replaceSeeds(indices, m_id);
		return true;
	}

	bool SegmentationGroup::removeSeed(int index)
	{
		assert(m_segmentation->checkValid(index)
			&& m_segmentation->checkIn(index, m_id));

		m_segmentation->removeSeed(index, m_id);
		return true;
	}

	ManualSegmentation::ManualSegmentation(TopoTriMeshPtr mesh)
		: m_mesh(mesh)
		, m_debugger(nullptr)
		, m_usedGroupId(0)
	{
		m_faceSize = (int)m_mesh->faces.size();
		m_groupMap.resize(m_faceSize, -1);
	}

	ManualSegmentation::~ManualSegmentation()
	{
		for (SegmentationGroup* group : m_groups)
			delete group;

		m_groups.clear();
	}

	void ManualSegmentation::setDebugger(ManualSegmentationDebugger* debugger)
	{
		m_debugger = debugger;
	}

	//
	SegmentationGroup* ManualSegmentation::addGroup()
	{
		SegmentationGroup* group = new SegmentationGroup(m_usedGroupId, this);
		m_groups.push_back(group);

		++m_usedGroupId;
		return group;
	}

	void ManualSegmentation::removeGroup(SegmentationGroup* group)
	{
		if (!group)
			return;

		std::vector<SegmentationGroup*>::iterator it = std::find(m_groups.begin(), m_groups.end(), group);
		if (it != m_groups.end())
		{
			int id = (*it)->m_id;
			for (int& index : m_groupMap)
			{
				if (index == id)
					index = -1;
			}
			m_groups.erase(it);
		}

		delete group;
	}

	SegmentationGroup* ManualSegmentation::checkIndex(int index)
	{
		if (!checkValid(index))
			return nullptr;

		int id = m_groupMap.at(index);
		for (SegmentationGroup* group : m_groups)
		{
			if (group->m_id == id)
				return group;
		}
		return nullptr;
	}

	void ManualSegmentation::segment()
	{

	}

	bool ManualSegmentation::checkOther(int index, int groupID)
	{
		return m_groupMap.at(index) == -1 || m_groupMap.at(index) == groupID;
	}

	bool ManualSegmentation::checkEmpty(int index)
	{
		return m_groupMap.at(index) == -1;
	}

	bool ManualSegmentation::checkIn(int index, int groupID)
	{
		return m_groupMap.at(index) == groupID;
	}

	bool ManualSegmentation::checkValid(int index)
	{
		return index >= 0 && index < m_faceSize;
	}

	void ManualSegmentation::replaceSeeds(const std::vector<int>& indices, int groupID)
	{
		for (int index : indices)
		{
			m_groupMap.at(index) = groupID;
		}
	}

	void ManualSegmentation::removeSeed(int index, int groupID)
	{
		m_groupMap.at(index) = -1;
	}
}