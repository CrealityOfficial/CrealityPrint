#pragma once
#include "internal/data/mmesht.h"
#include "Eigen/Dense"
#include "Eigen/Sparse"
//#include "Eigen/src/Cholesky/LLT.h"

namespace topomesh {

	class LaplacianMatrix
	{
	public:
		LaplacianMatrix() {};
		LaplacianMatrix(const std::vector<std::vector<std::pair<int,float>>>& data);
		LaplacianMatrix(MMeshT* mesh,bool weight=false);
		~LaplacianMatrix() {};

		
		void normalzationLaplacian();
		void EigenvectorSolver();
		void selectEigenVector(int indication ,std::vector<int>& result);

		inline const Eigen::SparseMatrix<float>* getLaplacianPtr() { return &L; };
		inline const Eigen::SparseMatrix<float>* getNormalizationLaplacianPtr() { return &NL; };

		inline const int getRow() { return row; }
		inline const int getCol() { return col; }
	private:
		int row;
		int col;
		Eigen::SparseMatrix<float>  L;
		Eigen::SparseMatrix<float>  NL;
		Eigen::SparseMatrix<float>  Lsym;
		Eigen::SparseMatrix<float>  Lrw;
		Eigen::MatrixXf EigenVector;
	};
}