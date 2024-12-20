#include "crslice2/matrix.h"
#include "libslic3r/Geometry.hpp"

namespace crslice2
{
	inline trimesh::dvec3 convert(const Slic3r::Vec3d& v)
	{
		return trimesh::dvec3(v.x(), v.y(), v.z());
	}

	inline Slic3r::Vec3d convert(const trimesh::dvec3& v)
	{
		return Slic3r::Vec3d(v.x, v.y, v.z);
	}

	class MatrixImpl
	{
	public:
		MatrixImpl()
		{

		}

		~MatrixImpl(){

		}

		Slic3r::Geometry::Transformation transform;
	};

	Matrix::Matrix()
		:impl(new MatrixImpl())
	{

	}

	Matrix::~Matrix()
	{

	}

	trimesh::dvec3 Matrix::get_offset() const
	{
		return convert(impl->transform.get_offset());
	}

	void Matrix::set_offset(const trimesh::dvec3& offset)
	{
		impl->transform.set_offset(convert(offset));
	}

	void Matrix::set_scaling_factor(const trimesh::dvec3& scaling_factor)
	{
		impl->transform.set_scaling_factor(convert(scaling_factor));
	}

	trimesh::dvec3 Matrix::get_rotation() const
	{
		return convert(impl->transform.get_rotation());
	}

	trimesh::dvec3 Matrix::get_scaling_factor() const
	{
		return convert(impl->transform.get_scaling_factor());
	}

	trimesh::xform Matrix::get_matrix() const
	{
		trimesh::xform x = trimesh::xform::identity();
		const Slic3r::Transform3d& d = impl->transform.get_matrix();
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				x(i, j) = d(i, j);
		return x;
	}

	void Matrix::set_matrix(const trimesh::xform& m)
	{
		Slic3r::Transform3d d;
		for (int i = 0; i < 4; ++i)
			for(int j = 0; j < 4; ++j)
				d(i, j) = m(i, j);
		impl->transform.set_matrix(d);
	}
}
