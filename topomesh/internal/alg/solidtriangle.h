#pragma once
#include "trimesh2/TriMesh.h"


/*��ɾ��������ʹ�������ڻ�����Ӧ�����ص�*/

namespace topomesh {
	class SolidTriangle {
	public :
		SolidTriangle() {};
		//��ʼ�� ������Ҫ��ɾ������������飬�ֱ��ʾ��Ⱥ�������Ķ�άboundingbox
		SolidTriangle(const std::vector<std::tuple<trimesh::point,trimesh::point,trimesh::point>>*,int,int,float,float,float,float);
		~SolidTriangle() {};


		void work();
		const std::vector<std::vector<std::vector<float>>>& getResult() { return _result; };
		/*float getData(int r, int c) { if (r >= _row || c >= _col) return std::numeric_limits<float>::max(); return _result[r][c]; };
		float getCoordData(float x, float y) { int xi = (x - _bbox_min_x) / (_col * 1.f); int yi = (y - _bbox_min_y) / (_row * 1.f); return _result[xi][yi]; };*/
		float getDataMinZ(float x, float y);
		float getDataMaxZ(float x, float y);
		float getDataMinZCoord(int xi,int yi);
		float getDataMaxZCoord(int xi,int yi);
		float getDataMinZInterpolation(float x, float y);
		float getDataMaxZInterpolation(float x, float y);
		float getDataMinZInterpolation(int xi, int yi);
		const std::vector<float> getResult(float x,float y);
		const std::vector<float> getResult(int xi, int yi);
	private:
		const std::vector<std::tuple<trimesh::point, trimesh::point, trimesh::point>>* _data=nullptr;
		int _row;
		int _col;
		float _bbox_min_x;
		float _bbox_min_y;
		float _bbox_max_x;
		float _bbox_max_y;
		float _length_x;
		float _length_y;
		std::vector<std::vector<std::vector<float>>> _result;
	};
}