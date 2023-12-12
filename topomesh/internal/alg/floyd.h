#pragma once
#include "Eigen/Dense"
#include "Eigen/Sparse"
#include "internal/data/mmesht.h"


#define FLOYD_INF 0x3f3f3f3f

namespace topomesh {
	template<class T = Eigen::SparseMatrix<float>>
	class floyd 
	{
	public:
		floyd() {};
		floyd(const T& data):_data(data)
		{			
			_path.resize(data.rows(), data.cols());
			auto trans = [&](int a) ->float {
				float r = (a == 0) ? FLOYD_INF : a;
				r = (r == -1) ? 0 : r;
				return r;
			};
			//-----bottom triangle-----
			//if (std::is_same<typename T, Eigen::SparseMatrix<float>>::value)
			//{
				for (int r = 0; r < _data.rows(); r++)
				{
					for (int i = 0; i < _data.rows(); i++)
					{
						if (r != i)
						{
							float distance1 = _data.coeffRef(i, r);
							if (distance1 == 0.0f) continue;
							distance1 = trans(distance1);
							for (int j = 0; j <= i; j++)
							{
								float value = _data.coeffRef(i, j);
								value = trans(value);
								float distance2 = _data.coeffRef(r, j);
								distance2 = trans(distance2);
								if (value > distance1 + distance2)
								{
									_result.coeffRef(i, j) = distance1 + distance2;
									_path.coeffRef(i, j) = r + 1;
								}
							}
						}
					}
				}
			//}
			
		};
		~floyd(){};
		Eigen::MatrixXf& get_result() { return _result; }
		Eigen::MatrixXf& get_path()  { return _path; }
	private:
		Eigen::SparseMatrix<float> _data;
		Eigen::MatrixXf _result;
		Eigen::MatrixXf _path;
	};
}