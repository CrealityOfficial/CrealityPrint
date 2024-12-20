#ifndef CRSLICE_MATRIX_H_2
#define CRSLICE_MATRIX_H_2
#include "crslice2/header.h"

namespace crslice2
{
	class MatrixImpl;
	class CRSLICE2_API Matrix
	{
	public:
		Matrix();
		~Matrix();

		trimesh::dvec3 get_offset() const ;
		void set_offset(const trimesh::dvec3& offset);
		void set_scaling_factor(const trimesh::dvec3& scaling_factor);

		trimesh::dvec3 get_rotation() const;
		trimesh::dvec3 get_scaling_factor() const;
		trimesh::xform get_matrix() const;
		void set_matrix(const trimesh::xform& m);
	protected:
		MatrixImpl* impl;
	};
}
#endif  // MSIMPLIFY_SIMPLIFY_H
