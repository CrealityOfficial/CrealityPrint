#include "laplacian.h"
#include<Eigen/Eigenvalues>

namespace topomesh {
	LaplacianMatrix::LaplacianMatrix(MMeshT* mesh, bool weight)
	{
		if (mesh->VN() != mesh->vertices.size()) mesh->shrinkMesh();
		if (weight && !mesh->is_HalfEdge()) return;
		this->row = mesh->VN();
		this->col = mesh->VN();
		Eigen::SparseMatrix<float>  D;
		Eigen::SparseMatrix<float>  W;
		Eigen::SparseMatrix<float>  D_sqrt;
		D.resize(this->row, this->col);
		W.resize(this->row, this->col);
		L.resize(this->row, this->col);
		typedef Eigen::Triplet<float> Tr;
		std::vector<Tr> D_tripletList;
		std::vector<Tr> Dsqrt_tripletList;
		std::vector<Tr> W_tripletList;
		for (MMeshVertex& v : mesh->vertices)if(!v.IsD())
		{
			float dre = v.connected_vertex.size();
			D_tripletList.push_back(Tr(v.index,v.index, dre));
			Dsqrt_tripletList.push_back(Tr(v.index, v.index, 1.f / std::sqrt(dre) * 1.f));
			if(weight)
				for (MMeshHalfEdge* he : v.v_mhe)
					W_tripletList.push_back(Tr(v.index, he->edge_vertex.second->index, he->attritube_f));
			else
				for (MMeshVertex* vv : v.connected_vertex)
					W_tripletList.push_back(Tr(v.index, vv->index, 1));
		}
		D.setFromTriplets(D_tripletList.begin(), D_tripletList.end());
		W.setFromTriplets(W_tripletList.begin(), W_tripletList.end());
		D_sqrt.setFromTriplets(Dsqrt_tripletList.begin(), Dsqrt_tripletList.end());
		L = D - W;
		NL = D_sqrt * L * D_sqrt;
	};


	LaplacianMatrix::LaplacianMatrix(const std::vector<std::vector<std::pair<int, float>>>& data)
	{
		this->row = data.size();
		this->col = data.size();
		L.resize(this->row, this->col);
		typedef Eigen::Triplet<float> Tr;
		std::vector<Tr> D_tripletList;
		std::vector<Tr> W_tripletList;
		for (int i = 0; i < data.size(); i++)
		{
			float diag = 0.f;
			for (int j = 0; j < data[i].size(); j++)
			{
				W_tripletList.push_back(Tr(i, data[i][j].first,data[i][j].second));
				diag += data[i][j].second;
			}
			D_tripletList.push_back(Tr(i,i,diag));
		}
		Eigen::SparseMatrix<float>  D(this->row, this->col);
		Eigen::SparseMatrix<float>  W(this->row, this->col);

		D.setFromTriplets(D_tripletList.begin(), D_tripletList.end());
		W.setFromTriplets(W_tripletList.begin(), W_tripletList.end());
		L = D - W;
	}


	void LaplacianMatrix::EigenvectorSolver()
	{
		if (L.size() == 0) return;
		Eigen::SelfAdjointEigenSolver<Eigen::SparseMatrix<float>> es(L);
		EigenVector = es.eigenvectors().block(0, 1, L.rows(), 30);
	}

	void LaplacianMatrix::selectEigenVector(int indication, std::vector<int>& result)
	{
		if (EigenVector.size() == 0) return;
		Eigen::VectorXf indica= EigenVector.row(indication);
		std::vector<std::pair<int, float>> distance;
		for (int i = 0; i < EigenVector.rows(); i++)
		{
			Eigen::VectorXf c = indica - EigenVector.row(i);
			distance.push_back(std::make_pair(i, c.norm()));
		}
		std::sort(distance.begin(), distance.end(), [](std::pair<int, float> a, std::pair<int, float> b) {
			return a.second < b.second;
			});
		for (int i = 0; i < 4; i++)
			result.push_back(distance[i].first);
	}

	void LaplacianMatrix::normalzationLaplacian()
	{
		
	}

	
}