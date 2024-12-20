#pragma once
#include <stdlib.h>
#include <numeric>
#include <iostream>

#include "Eigen/Dense"
#include "Eigen/Sparse"


namespace topomesh {
	template <typename T>
	class kmeansClustering {
	public:
		kmeansClustering() {};
		//template <typename T>
		kmeansClustering(T data,int centre_num = 2)
		{
			int data_num = data.rows();
			int data_dim = data.cols();
			//Eigen::SparseMatrix<float> centre(data_num, data_dim);
			std::vector<int> random_vec(data_num);
			std::vector<int> seed;
			std::iota(random_vec.begin(), random_vec.end(), 0);
			int randsize = data_num;
			for (int i = 0; i < centre_num; i++)
			{
				int index = rand() % randsize;
				seed.push_back(random_vec[index]);
				random_vec.erase(random_vec.begin() + index);
				randsize--;
			}
			std::vector<int>().swap(random_vec);

			Eigen::MatrixXf centre_data(centre_num, data_dim);
			for (int i = 0; i < centre_num; i++)
			{
				centre_data.row(i) = data.row(10*i);
				std::cout << "seed[i] : " << seed[i] << "\n";
			}
			

			Eigen::MatrixXd k_mark(data_num, centre_num);
			for (int i = 0; i < 10; i++)
			{
				k_mark.setZero();
				Eigen::MatrixXf distance(data_num, centre_num);
				for (int c = 0; c < centre_num; c++)
				{
					for (int n = 0; n < data_num; n++)
					{
						distance(n, c) = (data.row(n) - centre_data.row(c)).squaredNorm();
					}
				}
				//std::cout <<"i :"<<i << " centre_data : \n" << centre_data << "\n";
				//std::cout << "i :" << i << " distance : \n" << distance << "\n";
				//std::vector<int> k_mark(data_num);
				for (int n = 0; n < data_num; n++)
				{
					int r, c;
					distance.row(n).minCoeff(&r, &c);
					//k_mark[n] = c;
					k_mark(n, c) = 1;
				}
				//std::cout << "i :" << i << " k_mark : \n" << k_mark << "\n";
				Eigen::VectorXf cluster = k_mark.colwise().sum().cast<float>();
				for (int c = 0; c < centre_num; c++)
				{
					if (cluster[c] > 0)
					{
						Eigen::MatrixXf t = Eigen::MatrixXf::Zero(1, data_dim);
						for (int n = 0; n < data_num; n++)
						{
							if (k_mark(n, c) == 1)
							{
								t += data.row(n);
							}
						}
						centre_data.row(c) = t / cluster[c];
					}
				}
			}

			this->result.resize(centre_num);
			for (int i = 0; i < centre_num; i++)
			{
				for (int n = 0; n < data_num; n++)
				{
					if (k_mark(n, i) == 1)
					{
						this->result[i].push_back(n);
					}
				}
			}
		};//Eigen::SparseMatrix<float>
		~kmeansClustering() { std::vector<std::vector<float>>().swap(result); };
		std::vector<std::vector<float>>* get_result() { return &result; }
	private:
		std::vector<std::vector<float>> result;
	};
}