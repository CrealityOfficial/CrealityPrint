#pragma once
#include "Eigen/Dense"
#include "Eigen/Sparse"
#include "internal/data/mmesht.h"

namespace topomesh {
	template <typename T>
	class Dijkstra {
	public:
		Dijkstra() {};
		Dijkstra(T data)
		{

		}
		~Dijkstra() {};
		T& get_result() { return result; }
	private:
		T result;
	};


}