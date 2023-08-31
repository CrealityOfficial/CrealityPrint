#ifndef _NULLSPACE_TRIMESHINPUT_1589465815718_H
#define _NULLSPACE_TRIMESHINPUT_1589465815718_H
#include "modelinput.h"
#include "trimesh2/TriMesh.h"
#include "trimesh2/XForm.h"

namespace creative_kernel
{
	class ModelN;
	class TrimeshInput : public ModelInput
	{
	public:
		TrimeshInput(QObject* parent = nullptr);
		virtual ~TrimeshInput();

		void setModel(creative_kernel::ModelN* model);
		void build();
		void beltSet();
		void tiltSliceSet(trimesh::vec axis, float angle);

		const creative_kernel::TriMeshPtr ptr() const;
		const creative_kernel::TriMeshPtr beltMesh() const;

		void setBeltOffset(const trimesh::vec3& offset);
		trimesh::TriMesh* generateBeltSupport(trimesh::TriMesh* mesh, float angle);
		trimesh::TriMesh* generateSlopeSupport(trimesh::TriMesh* mesh, float rotation_angle, trimesh::vec axis, float support_angle, bool bottomBigger);

		int vertexNum() const override;
		float* position() const override;
		float* normal() const override;

		int triangleNum() const override;
		int* indices() const override;
	protected:
		creative_kernel::ModelN* m_model;
		trimesh::TriMesh* m_mesh;
		trimesh::vec3 m_offset;
	};
}
#endif // _NULLSPACE_TRIMESHINPUT_1589465815718_H
